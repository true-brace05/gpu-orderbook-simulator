#pragma once

/**
 * @brief Host-side wrapper function to launch a simple verification CUDA kernel.
 * 
 * Launches the helloKernel on the GPU runtime default stream and synchronizes device execution
 * using cudaDeviceSynchronize(). Throws std::runtime_error if kernel launch or synchronization fails.
 */
void launchHelloKernel();
