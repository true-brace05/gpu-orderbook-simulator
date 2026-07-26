#pragma once

#include "cuda/CUDAContext.h"
#include "cuda/ReplayBuffer.h"
#include "cuda/DecodedEventBuffer.h"

#include <cuda_runtime.h>

/**
 * @brief Decodes AoS Event objects stored in ReplayBuffer into SoA DecodedEventBuffer on GPU.
 *
 * Launches a CUDA kernel where each thread converts one Event (AoS) into SoA attribute streams.
 *
 * @param inputBuffer Input ReplayBuffer containing AoS Event objects.
 * @param outputBuffer Output DecodedEventBuffer to receive SoA attribute streams.
 * @param context Active CUDAContext for CUDA stream and synchronization.
 */
void decodeEvents(
    const ReplayBuffer& inputBuffer,
    DecodedEventBuffer& outputBuffer,
    CUDAContext& context);

/**
 * @brief Asynchronously decodes AoS Event objects into SoA layout on specified CUDA stream.
 *
 * @param inputBuffer Input ReplayBuffer containing AoS Event objects.
 * @param outputBuffer Output DecodedEventBuffer to receive SoA attribute streams.
 * @param stream CUDA stream handle for asynchronous kernel launch.
 */
void decodeEventsAsync(
    const ReplayBuffer& inputBuffer,
    DecodedEventBuffer& outputBuffer,
    cudaStream_t stream = nullptr);
