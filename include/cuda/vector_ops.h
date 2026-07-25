#pragma once

#include "cuda/CUDAContext.h"
#include "cuda/DeviceBuffer.h"

/**
 * @brief Synchronously adds two GPU device buffers element-wise: C = A + B.
 *
 * @param A Source device buffer A containing N float elements.
 * @param B Source device buffer B containing N float elements.
 * @param C Destination device buffer C containing N float elements.
 * @param context Active CUDAContext managing CUDA stream and device execution.
 *
 * Throws std::invalid_argument if buffer sizes mismatch or if buffers are invalid.
 */
void vectorAdd(
    const DeviceBuffer<float>& A,
    const DeviceBuffer<float>& B,
    DeviceBuffer<float>& C,
    CUDAContext& context);
