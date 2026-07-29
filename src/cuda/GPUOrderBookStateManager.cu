#include "cuda/GPUOrderBookStateManager.h"
#include "cuda/CUDAContext.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------------
// Device Helper & Hash Functions
// -----------------------------------------------------------------------------

__device__ inline uint32_t hashOrderId(int orderId, uint32_t capacity)
{
    uint32_t x = static_cast<uint32_t>(orderId);
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x % capacity;
}

__device__ inline bool insertHashEntry(HashEntry* table, uint32_t capacity, int orderId, int slotIndex)
{
    uint32_t hash = hashOrderId(orderId, capacity);
    for (uint32_t i = 0; i < capacity; ++i)
    {
        uint32_t idx = (hash + i) % capacity;
        int prevKey = atomicCAS(&table[idx].orderId, -1, orderId);
        if (prevKey == -1 || prevKey == orderId)
        {
            table[idx].slotIndex = slotIndex;
            return true;
        }
    }
    return false;
}

__device__ inline int lookupHashEntry(const HashEntry* table, uint32_t capacity, int orderId)
{
    uint32_t hash = hashOrderId(orderId, capacity);
    for (uint32_t i = 0; i < capacity; ++i)
    {
        uint32_t idx = (hash + i) % capacity;
        int key = table[idx].orderId;
        if (key == orderId)
        {
            return table[idx].slotIndex;
        }
        if (key == -1)
        {
            return -1;
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// CUDA Kernels
// -----------------------------------------------------------------------------

__global__ void processAddEventsKernel(
    const int* addIndices,
    std::size_t numAddEvents,
    ConstDecodedEventSoAView decodedSoA,
    GPUOrderState* orders,
    int* activeOrderCount,
    std::size_t maxOrders,
    HashEntry* hashMap,
    std::size_t hashCapacity,
    double tickSize)
{
    std::size_t tid = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (tid >= numAddEvents)
    {
        return;
    }

    int addIdx = addIndices[tid];
    int orderId = decodedSoA.orderIds[addIdx];
    uint8_t side = decodedSoA.sides[addIdx];
    uint8_t orderType = decodedSoA.orderTypes[addIdx];
    double price = decodedSoA.prices[addIdx];
    uint32_t priceTick = static_cast<uint32_t>(llround(price / tickSize));
    int quantity = decodedSoA.quantities[addIdx];
    int displayQty = decodedSoA.displayQuantities ? decodedSoA.displayQuantities[addIdx] : quantity;
    int reserveQty = decodedSoA.reserveQuantities ? decodedSoA.reserveQuantities[addIdx] : 0;
    uint64_t timestamp = decodedSoA.timestamps[addIdx];

    if (quantity <= 0)
    {
        return;
    }

    int slotIndex = atomicAdd(activeOrderCount, 1);
    if (static_cast<std::size_t>(slotIndex) < maxOrders)
    {
        GPUOrderState state;
        state.orderId = orderId;
        state.priceTick = priceTick;
        state.quantity = quantity;
        state.displayQuantity = (displayQty > 0) ? displayQty : quantity;
        state.reserveQuantity = reserveQty;
        state.side = side;
        state.orderType = orderType;
        state.timestamp = timestamp;
        state.status = 0; // Active

        orders[slotIndex] = state;
        insertHashEntry(hashMap, static_cast<uint32_t>(hashCapacity), orderId, slotIndex);
    }
}

__global__ void processCancelEventsKernel(
    const int* cancelIndices,
    std::size_t numCancelEvents,
    ConstDecodedEventSoAView decodedSoA,
    GPUOrderState* orders,
    const HashEntry* hashMap,
    std::size_t hashCapacity)
{
    std::size_t tid = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (tid >= numCancelEvents)
    {
        return;
    }

    int cancelIdx = cancelIndices[tid];
    int orderId = decodedSoA.orderIds[cancelIdx];
    int cancelQty = decodedSoA.quantities[cancelIdx];

    if (cancelQty <= 0)
    {
        return;
    }

    int slotIndex = lookupHashEntry(hashMap, static_cast<uint32_t>(hashCapacity), orderId);
    if (slotIndex >= 0)
    {
        GPUOrderState& order = orders[slotIndex];
        int* qtyPtr = &order.quantity;

        int oldQty = atomicSub(qtyPtr, cancelQty);
        if (oldQty <= cancelQty)
        {
            atomicExch(qtyPtr, 0);
            atomicExch(&order.displayQuantity, 0);
            atomicExch(&order.reserveQuantity, 0);
            atomicCAS(&order.status, 0, 1); // Canceled (1)
        }
    }
}

__global__ void resetStateManagerKernel(
    GPUOrderState* orders,
    std::size_t orderCapacity,
    HashEntry* hashMap,
    std::size_t hashCapacity,
    int* activeOrderCount)
{
    std::size_t tid = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

    if (tid < orderCapacity)
    {
        orders[tid] = GPUOrderState{};
    }

    if (tid < hashCapacity)
    {
        hashMap[tid].orderId = -1;
        hashMap[tid].slotIndex = -1;
    }

    if (tid == 0)
    {
        *activeOrderCount = 0;
    }
}

// -----------------------------------------------------------------------------
// Host API Implementation
// -----------------------------------------------------------------------------

GPUOrderBookStateManager::GPUOrderBookStateManager(std::size_t maxOrders, std::size_t maxLevels)
    : orderCapacity(maxOrders),
      hashCapacity(maxOrders * 2), // 50% load factor
      levelCapacity(maxLevels)
{
    if (maxOrders == 0)
    {
        throw std::invalid_argument("GPUOrderBookStateManager: maxOrders must be > 0");
    }

    ordersBuf.allocate(orderCapacity);
    hashMapBuf.allocate(hashCapacity);
    levelAggregatesBuf.allocate(levelCapacity);
    activeOrderCountBuf.allocate(1);

    reset();
}

void GPUOrderBookStateManager::reset(cudaStream_t stream)
{
    if (orderCapacity == 0)
    {
        return;
    }

    std::size_t maxCap = (orderCapacity > hashCapacity) ? orderCapacity : hashCapacity;
    int blockSize = 256;
    int numBlocks = static_cast<int>((maxCap + blockSize - 1) / blockSize);

    resetStateManagerKernel<<<numBlocks, blockSize, 0, stream>>>(
        ordersBuf.data(),
        orderCapacity,
        hashMapBuf.data(),
        hashCapacity,
        activeOrderCountBuf.data());

    if (stream == nullptr)
    {
        cudaStreamSynchronize(nullptr);
    }
}

void GPUOrderBookStateManager::processEventsShadow(
    const ClassifiedEventBuffer& classifiedEvents,
    const DecodedEventBuffer& decodedEvents,
    double tickSize,
    cudaStream_t stream)
{
    std::size_t numAdd = classifiedEvents.getCount(EventType::Add);
    std::size_t numCancel = classifiedEvents.getCount(EventType::Cancel);

    int blockSize = 256;

    // 1. Process Add events
    if (numAdd > 0)
    {
        int addBlocks = static_cast<int>((numAdd + blockSize - 1) / blockSize);
        processAddEventsKernel<<<addBlocks, blockSize, 0, stream>>>(
            classifiedEvents.getIndices(EventType::Add),
            numAdd,
            decodedEvents.getDeviceView(),
            ordersBuf.data(),
            activeOrderCountBuf.data(),
            orderCapacity,
            hashMapBuf.data(),
            hashCapacity,
            tickSize);
    }

    // 2. Process Cancel events
    if (numCancel > 0)
    {
        int cancelBlocks = static_cast<int>((numCancel + blockSize - 1) / blockSize);
        processCancelEventsKernel<<<cancelBlocks, blockSize, 0, stream>>>(
            classifiedEvents.getIndices(EventType::Cancel),
            numCancel,
            decodedEvents.getDeviceView(),
            ordersBuf.data(),
            hashMapBuf.data(),
            hashCapacity);
    }

    if (stream == nullptr)
    {
        cudaStreamSynchronize(nullptr);
    }
}

std::size_t GPUOrderBookStateManager::getActiveOrderCount(cudaStream_t stream) const
{
    int hostCount = 0;
    activeOrderCountBuf.copyToHost(&hostCount, 1);
    return static_cast<std::size_t>(hostCount);
}

bool GPUOrderBookStateManager::verifyBatch(
    const DecodedEventBuffer& decodedEvents,
    double tickSize,
    std::string* outErrorMessage)
{
    std::size_t gpuOrderCount = getActiveOrderCount();
    if (gpuOrderCount > orderCapacity)
    {
        if (outErrorMessage != nullptr)
        {
            std::ostringstream oss;
            oss << "[Invariant Failure] GPU active order count (" << gpuOrderCount
                << ") exceeds capacity (" << orderCapacity << ")";
            *outErrorMessage = oss.str();
        }
        return false;
    }

    // Download GPU order state array for host verification
    std::vector<GPUOrderState> hostOrders(gpuOrderCount);
    if (gpuOrderCount > 0)
    {
        ordersBuf.copyToHost(hostOrders.data(), gpuOrderCount);
    }

    // CPU Shadow Reference Verification
    struct HostOrder
    {
        int orderId;
        uint32_t priceTick;
        int quantity;
        uint8_t side;
    };
    std::unordered_map<int, HostOrder> cpuBook;

    std::size_t totalEvents = decodedEvents.size();
    std::vector<Event> cpuEvents(totalEvents);
    if (totalEvents > 0)
    {
        // Copy decoded attributes back from GPU DecodedEventBuffer view
        ConstDecodedEventSoAView deviceView = decodedEvents.getDeviceView();
        std::vector<int> hostOrderIds(totalEvents);
        std::vector<uint8_t> hostSides(totalEvents);
        std::vector<double> hostPrices(totalEvents);
        std::vector<int> hostQuantities(totalEvents);
        std::vector<uint8_t> hostEventTypes(totalEvents);

        decodedEvents.orderIdsBuffer().copyToHost(hostOrderIds.data(), totalEvents);
        decodedEvents.sidesBuffer().copyToHost(hostSides.data(), totalEvents);
        decodedEvents.pricesBuffer().copyToHost(hostPrices.data(), totalEvents);
        decodedEvents.quantitiesBuffer().copyToHost(hostQuantities.data(), totalEvents);
        decodedEvents.eventTypesBuffer().copyToHost(hostEventTypes.data(), totalEvents);

        for (std::size_t i = 0; i < totalEvents; ++i)
        {
            EventType evType = static_cast<EventType>(hostEventTypes[i]);
            if (evType == EventType::Add)
            {
                uint32_t priceTick = static_cast<uint32_t>(llround(hostPrices[i] / tickSize));
                cpuBook[hostOrderIds[i]] = HostOrder{hostOrderIds[i], priceTick, hostQuantities[i], hostSides[i]};
            }
            else if (evType == EventType::Cancel)
            {
                auto it = cpuBook.find(hostOrderIds[i]);
                if (it != cpuBook.end())
                {
                    it->second.quantity -= hostQuantities[i];
                    if (it->second.quantity <= 0)
                    {
                        it->second.quantity = 0;
                    }
                }
            }
        }
    }

    // Invariant Check 1: Verify order quantity equivalence
    std::unordered_map<int, GPUOrderState> gpuOrderMap;
    for (const auto& o : hostOrders)
    {
        if (o.orderId > 0)
        {
            gpuOrderMap[o.orderId] = o;
        }
    }

    for (const auto& pair : cpuBook)
    {
        int cpuId = pair.first;
        const auto& cpuOrd = pair.second;

        auto git = gpuOrderMap.find(cpuId);
        if (git == gpuOrderMap.end())
        {
            if (outErrorMessage != nullptr)
            {
                std::ostringstream oss;
                oss << "[Equivalence Failure] Order ID " << cpuId << " present on CPU but missing on GPU";
                *outErrorMessage = oss.str();
            }
            return false;
        }

        if (git->second.quantity != cpuOrd.quantity)
        {
            if (outErrorMessage != nullptr)
            {
                std::ostringstream oss;
                oss << "[Equivalence Failure] Order ID " << cpuId << " quantity mismatch: CPU = "
                    << cpuOrd.quantity << ", GPU = " << git->second.quantity;
                *outErrorMessage = oss.str();
            }
            return false;
        }
    }

    return true;
}
