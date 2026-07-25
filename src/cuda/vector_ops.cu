#include "cuda/vector_ops.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// CUDA Kernels
// -----------------------------------------------------------------------------

/**
 * @brief Element-wise float vector addition kernel: C[idx] = A[idx] + B[idx].
 *
 * @param A Pointer to input array A in GPU memory.
 * @param B Pointer to input array B in GPU memory.
 * @param C Pointer to output array C in GPU memory.
 * @param N Total number of elements.
 */
__global__ void vectorAddKernel(const float* A, const float* B, float* C, std::size_t N)
{
    std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx < N)
    {
        C[idx] = A[idx] + B[idx];
    }
}

// -----------------------------------------------------------------------------
// Host API Launchers
// -----------------------------------------------------------------------------

void vectorAdd(
    const DeviceBuffer<float>& A,
    const DeviceBuffer<float>& B,
    DeviceBuffer<float>& C,
    CUDAContext& context)
{
    const std::size_t nA = A.size();
    const std::size_t nB = B.size();
    const std::size_t nC = C.size();

    if (nA != nB || nA != nC)
    {
        throw std::invalid_argument(
            "vectorAdd: Vector size mismatch (A: " + std::to_string(nA) +
            ", B: " + std::to_string(nB) + ", C: " + std::to_string(nC) + ")"
        );
    }

    if (nA == 0)
    {
        return;
    }

    if (!A.isValid() || !B.isValid() || !C.isValid())
    {
        throw std::invalid_argument("vectorAdd: Device buffers must be allocated and valid");
    }

    constexpr int blockSize = 256;
    const int gridSize = static_cast<int>((nA + blockSize - 1) / blockSize);

    vectorAddKernel<<<gridSize, blockSize, 0, context.getStream()>>>(
        A.data(),
        B.data(),
        C.data(),
        nA
    );

    detail::checkDeviceBufferCudaError(
        cudaGetLastError(),
        "Failed to launch vectorAddKernel"
    );

    context.synchronize();
}
