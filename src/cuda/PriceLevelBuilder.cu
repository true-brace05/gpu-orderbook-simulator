#include "cuda/PriceLevelBuilder.h"
#include "cuda/DeviceBuffer.h"

#include <cuda_runtime.h>
#include <thrust/sort.h>
#include <thrust/device_ptr.h>
#include <thrust/execution_policy.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// Structs & CUDA Kernels
// -----------------------------------------------------------------------------

struct OrderKey
{
    uint32_t priceTick;
    uint8_t side;
    int addIndex;
    int quantity;

    // Sorting functor: Sort by (side, priceTick)
    __host__ __device__ bool operator<(const OrderKey& other) const
    {
        if (side != other.side)
        {
            return side < other.side;
        }
        return priceTick < other.priceTick;
    }
};

/**
 * @brief CUDA kernel extracting OrderKey entries from DecodedEventBuffer for classified Add events.
 */
__global__ void extractOrderKeysKernel(
    const int* addIndices,
    std::size_t numAddEvents,
    ConstDecodedEventSoAView decodedSoA,
    OrderKey* keys,
    double tickSize)
{
    std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx < numAddEvents)
    {
        int addIdx = addIndices[idx];
        double p = decodedSoA.prices[addIdx];
        uint32_t tick = static_cast<uint32_t>(llround(p / tickSize));

        OrderKey key;
        key.priceTick = tick;
        key.side = decodedSoA.sides[addIdx];
        key.addIndex = addIdx;
        key.quantity = decodedSoA.quantities[addIdx];

        keys[idx] = key;
    }
}

/**
 * @brief CUDA kernel building CSR PriceLevel structs and order index mappings from sorted OrderKeys.
 */
__global__ void buildPriceLevelsFromSortedKernel(
    const OrderKey* keys,
    std::size_t numAddEvents,
    PriceLevel* outputLevels,
    int* outputOrderIndices,
    int* outputLevelCount)
{
    std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx < numAddEvents)
    {
        bool isStart = (idx == 0) ||
                       (keys[idx].side != keys[idx - 1].side) ||
                       (keys[idx].priceTick != keys[idx - 1].priceTick);

        if (isStart)
        {
            int levelIdx = atomicAdd(outputLevelCount, 1);

            int totalQty = 0;
            int count = 0;
            std::size_t j = idx;

            while (j < numAddEvents &&
                   keys[j].side == keys[idx].side &&
                   keys[j].priceTick == keys[idx].priceTick)
            {
                totalQty += keys[j].quantity;
                outputOrderIndices[j] = keys[j].addIndex;
                count++;
                j++;
            }

            PriceLevel level;
            level.priceTick = keys[idx].priceTick;
            level.side = keys[idx].side;
            level.totalQuantity = totalQty;
            level.firstOrder = static_cast<int>(idx);
            level.orderCount = count;

            outputLevels[levelIdx] = level;
        }
    }
}

// -----------------------------------------------------------------------------
// Host API Implementation
// -----------------------------------------------------------------------------

void buildPriceLevelsAsync(
    const DecodedEventBuffer& decodedBuffer,
    const ClassifiedEventBuffer& classifiedBuffer,
    PriceLevelBuffer& priceLevelBuffer,
    cudaStream_t stream,
    double tickSize)
{
    if (tickSize <= 0.0)
    {
        throw std::invalid_argument("buildPriceLevelsAsync: tickSize must be greater than zero");
    }

    const std::size_t addCount = classifiedBuffer.getCount(EventType::Add);
    if (addCount == 0)
    {
        priceLevelBuffer.clear();
        return;
    }

    if (!decodedBuffer.isValid())
    {
        throw std::invalid_argument("buildPriceLevelsAsync: Input DecodedEventBuffer is not valid");
    }

    if (!classifiedBuffer.isValid())
    {
        throw std::invalid_argument("buildPriceLevelsAsync: Input ClassifiedEventBuffer is not valid");
    }

    if (priceLevelBuffer.capacity() < addCount)
    {
        priceLevelBuffer.reserve(addCount);
    }

    priceLevelBuffer.clear();

    if (!priceLevelBuffer.isValid())
    {
        throw std::invalid_argument("buildPriceLevelsAsync: Output PriceLevelBuffer is not valid");
    }

    // Allocate temporary GPU buffer for order keys
    DeviceBuffer<OrderKey> keysBuffer(addCount);

    constexpr int blockSize = 256;
    const int gridSize = static_cast<int>((addCount + blockSize - 1) / blockSize);

    // 1. Extract keys (priceTick, side, addIndex, quantity)
    extractOrderKeysKernel<<<gridSize, blockSize, 0, stream>>>(
        classifiedBuffer.getIndices(EventType::Add),
        addCount,
        decodedBuffer.getDeviceView(),
        keysBuffer.data(),
        tickSize
    );

    detail::checkDeviceBufferCudaError(
        cudaGetLastError(),
        "Failed to launch extractOrderKeysKernel"
    );

    // 2. Sort keys by (side, priceTick) using Thrust on CUDA stream
    thrust::device_ptr<OrderKey> d_keys_start(keysBuffer.data());
    thrust::device_ptr<OrderKey> d_keys_end(keysBuffer.data() + addCount);

    if (stream != nullptr)
    {
        thrust::sort(thrust::cuda::par.on(stream), d_keys_start, d_keys_end);
    }
    else
    {
        thrust::sort(thrust::device, d_keys_start, d_keys_end);
    }

    // 3. Build CSR price levels and order indices
    buildPriceLevelsFromSortedKernel<<<gridSize, blockSize, 0, stream>>>(
        keysBuffer.data(),
        addCount,
        priceLevelBuffer.data(),
        priceLevelBuffer.orderIndicesData(),
        priceLevelBuffer.levelCountDevicePtr()
    );

    detail::checkDeviceBufferCudaError(
        cudaGetLastError(),
        "Failed to launch buildPriceLevelsFromSortedKernel"
    );

    priceLevelBuffer.updateHostCount(stream);
    priceLevelBuffer.setOrderCount(addCount);
}

void buildPriceLevels(
    const DecodedEventBuffer& decodedBuffer,
    const ClassifiedEventBuffer& classifiedBuffer,
    PriceLevelBuffer& priceLevelBuffer,
    CUDAContext& context,
    double tickSize)
{
    buildPriceLevelsAsync(decodedBuffer, classifiedBuffer, priceLevelBuffer, context.getStream(), tickSize);
    context.synchronize();
}
