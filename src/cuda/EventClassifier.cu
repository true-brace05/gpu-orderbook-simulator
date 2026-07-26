#include "cuda/EventClassifier.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// CUDA Kernel
// -----------------------------------------------------------------------------

/**
 * @brief CUDA kernel for classifying events into 7 category index streams using atomic GPU operations.
 * One thread per event index.
 *
 * @param eventTypes Pointer to eventTypes array (uint8_t) in GPU device memory.
 * @param outputView View containing target index array pointers and categoryCounts counter array.
 * @param count Total number of decoded events.
 */
__global__ void classifyEventsKernel(
    const uint8_t* eventTypes,
    ClassifiedEventSoAView outputView,
    std::size_t count)
{
    std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx < count)
    {
        uint8_t typeVal = eventTypes[idx];
        if (typeVal < ClassificationCounters::NUM_CATEGORIES)
        {
            int* targetArray = nullptr;
            switch (typeVal)
            {
                case 0: targetArray = outputView.addIndices; break;
                case 1: targetArray = outputView.cancelIndices; break;
                case 2: targetArray = outputView.modifyIndices; break;
                case 3: targetArray = outputView.deleteIndices; break;
                case 4: targetArray = outputView.executeVisibleIndices; break;
                case 5: targetArray = outputView.executeHiddenIndices; break;
                case 6: targetArray = outputView.tradingHaltIndices; break;
                default: break;
            }

            if (targetArray != nullptr)
            {
                int writeOffset = atomicAdd(&outputView.categoryCounts[typeVal], 1);
                targetArray[writeOffset] = static_cast<int>(idx);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Host API Implementation
// -----------------------------------------------------------------------------

void classifyEventsAsync(
    const DecodedEventBuffer& decodedBuffer,
    ClassifiedEventBuffer& classifiedBuffer,
    cudaStream_t stream)
{
    const std::size_t count = decodedBuffer.size();
    if (count == 0)
    {
        classifiedBuffer.clear();
        return;
    }

    if (!decodedBuffer.isValid())
    {
        throw std::invalid_argument("classifyEventsAsync: Input DecodedEventBuffer is not allocated or valid");
    }

    if (classifiedBuffer.capacity() < count)
    {
        classifiedBuffer.reserve(count);
    }

    classifiedBuffer.clear();

    if (!classifiedBuffer.isValid())
    {
        throw std::invalid_argument("classifyEventsAsync: Output ClassifiedEventBuffer is not valid");
    }

    constexpr int blockSize = 256;
    const int gridSize = static_cast<int>((count + blockSize - 1) / blockSize);

    classifyEventsKernel<<<gridSize, blockSize, 0, stream>>>(
        decodedBuffer.eventTypes(),
        classifiedBuffer.getDeviceView(),
        count
    );

    detail::checkDeviceBufferCudaError(
        cudaGetLastError(),
        "Failed to launch classifyEventsKernel"
    );

    classifiedBuffer.updateHostCounters(stream);
}

void classifyEvents(
    const DecodedEventBuffer& decodedBuffer,
    ClassifiedEventBuffer& classifiedBuffer,
    CUDAContext& context)
{
    classifyEventsAsync(decodedBuffer, classifiedBuffer, context.getStream());
    context.synchronize();
}
