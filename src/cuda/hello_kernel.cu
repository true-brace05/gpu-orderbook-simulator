#include "cuda/hello_kernel.h"

#include <cuda_runtime.h>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace
{
    /**
     * @brief Minimal verification CUDA kernel printing a device message.
     */
    __global__ void helloKernel()
    {
        printf("Hello from GPU thread [%d, %d]!\n", threadIdx.x, blockIdx.x);
    }
} // anonymous namespace

void launchHelloKernel()
{
    helloKernel<<<1, 1>>>();

    cudaError_t launchErr = cudaGetLastError();
    if (launchErr != cudaSuccess)
    {
        throw std::runtime_error(
            std::string("CUDA kernel launch failed: ") + cudaGetErrorString(launchErr)
        );
    }

    cudaError_t syncErr = cudaDeviceSynchronize();
    if (syncErr != cudaSuccess)
    {
        throw std::runtime_error(
            std::string("CUDA device synchronization failed: ") + cudaGetErrorString(syncErr)
        );
    }
}
