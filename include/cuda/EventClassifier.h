#pragma once

#include "cuda/CUDAContext.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/ClassifiedEventBuffer.h"

#include <cuda_runtime.h>

/**
 * @brief Classifies decoded GPU event attributes (DecodedEventBuffer) into 7 category index streams using atomic GPU operations.
 *
 * Each thread handles one event from DecodedEventBuffer, atomically incrementing the category counter and appending
 * the event index into the target category index array in ClassifiedEventBuffer.
 *
 * @param decodedBuffer Input DecodedEventBuffer containing SoA event attribute streams.
 * @param classifiedBuffer Output ClassifiedEventBuffer receiving classified index streams.
 * @param context Active CUDAContext for stream execution and synchronization.
 */
void classifyEvents(
    const DecodedEventBuffer& decodedBuffer,
    ClassifiedEventBuffer& classifiedBuffer,
    CUDAContext& context);

/**
 * @brief Asynchronously classifies decoded GPU events on the specified CUDA stream.
 *
 * @param decodedBuffer Input DecodedEventBuffer containing SoA event attribute streams.
 * @param classifiedBuffer Output ClassifiedEventBuffer receiving classified index streams.
 * @param stream CUDA stream handle for asynchronous kernel launch.
 */
void classifyEventsAsync(
    const DecodedEventBuffer& decodedBuffer,
    ClassifiedEventBuffer& classifiedBuffer,
    cudaStream_t stream = nullptr);
