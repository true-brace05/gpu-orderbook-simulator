#include "cuda/EventDecoder.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// CUDA Kernel
// -----------------------------------------------------------------------------

/**
 * @brief CUDA kernel for converting Array of Structures (AoS) Event objects to Structure of Arrays (SoA).
 * Each thread handles decoding one Event index.
 *
 * @param inputEvents Pointer to input array of Event objects in GPU memory.
 * @param outputSoA View struct containing raw destination GPU pointers for each attribute stream.
 * @param count Total number of events to decode.
 */
__global__ void decodeEventsKernel(
    const Event* inputEvents,
    DecodedEventSoAView outputSoA,
    std::size_t count)
{
    std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx < count)
    {
        const Event& ev = inputEvents[idx];
        outputSoA.timestamps[idx] = ev.timestamp;
        outputSoA.eventTypes[idx] = static_cast<uint8_t>(ev.type);
        outputSoA.orderIds[idx] = ev.orderId;
        outputSoA.sides[idx] = static_cast<uint8_t>(ev.order.side);
        outputSoA.orderTypes[idx] = static_cast<uint8_t>(ev.order.type);
        outputSoA.prices[idx] = ev.order.price;
        outputSoA.quantities[idx] = ev.order.quantity;
        outputSoA.displayQuantities[idx] = ev.order.displayQuantity;
        outputSoA.reserveQuantities[idx] = ev.order.reserveQuantity;
    }
}

// -----------------------------------------------------------------------------
// Host API Implementation
// -----------------------------------------------------------------------------

void decodeEventsAsync(
    const ReplayBuffer& inputBuffer,
    DecodedEventBuffer& outputBuffer,
    cudaStream_t stream)
{
    const std::size_t count = inputBuffer.size();
    if (count == 0)
    {
        outputBuffer.clear();
        return;
    }

    if (!inputBuffer.isValid())
    {
        throw std::invalid_argument("decodeEventsAsync: Input ReplayBuffer is not allocated or valid");
    }

    if (outputBuffer.capacity() < count)
    {
        outputBuffer.reserve(count);
    }

    outputBuffer.resize(count);

    if (!outputBuffer.isValid())
    {
        throw std::invalid_argument("decodeEventsAsync: Output DecodedEventBuffer is not valid");
    }

    constexpr int blockSize = 256;
    const int gridSize = static_cast<int>((count + blockSize - 1) / blockSize);

    decodeEventsKernel<<<gridSize, blockSize, 0, stream>>>(
        inputBuffer.data(),
        outputBuffer.getDeviceView(),
        count
    );

    detail::checkDeviceBufferCudaError(
        cudaGetLastError(),
        "Failed to launch decodeEventsKernel"
    );
}

void decodeEvents(
    const ReplayBuffer& inputBuffer,
    DecodedEventBuffer& outputBuffer,
    CUDAContext& context)
{
    decodeEventsAsync(inputBuffer, outputBuffer, context.getStream());
    context.synchronize();
}
