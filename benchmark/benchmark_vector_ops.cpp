#include "cuda/vector_ops.h"
#include "cuda/CUDAContext.h"
#include "cuda/DeviceBuffer.h"

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <stdexcept>

/**
 * @brief CPU reference implementation for element-wise vector addition.
 */
void cpuVectorAdd(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C)
{
    const std::size_t n = A.size();
    C.resize(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        C[i] = A[i] + B[i];
    }
}

/**
 * @brief Helper to validate element-by-element match between GPU result and CPU reference,
 * with detailed diagnostics printed on the first mismatch.
 */
bool verifyResults(
    const std::vector<float>& gpuResult,
    const std::vector<float>& cpuReference,
    const std::vector<float>& h_A,
    const std::vector<float>& h_B,
    float tolerance = 1e-5f)
{
    if (gpuResult.size() != cpuReference.size())
    {
        std::cerr << "  Failure: Size mismatch between GPU result (" << gpuResult.size()
                  << ") and CPU reference (" << cpuReference.size() << ")\n";
        return false;
    }

    for (std::size_t i = 0; i < gpuResult.size(); ++i)
    {
        float diff = std::abs(gpuResult[i] - cpuReference[i]);
        if (diff > tolerance)
        {
            std::cerr << "\n  [BENCHMARK MISMATCH DETECTED]\n"
                      << "    First mismatching index : " << i << '\n'
                      << "    Expected value (CPU)   : " << cpuReference[i] << '\n'
                      << "    Actual value (GPU)     : " << gpuResult[i] << '\n'
                      << "    Input A[" << i << "]             : " << (i < h_A.size() ? std::to_string(h_A[i]) : "N/A") << '\n'
                      << "    Input B[" << i << "]             : " << (i < h_B.size() ? std::to_string(h_B[i]) : "N/A") << '\n'
                      << "    Absolute Difference    : " << diff << '\n';
            return false;
        }
    }

    return true;
}

int main()
{
    std::cout << "========================================================================================\n";
    std::cout << "                        GPU vs CPU Vector Addition Benchmark                            \n";
    std::cout << "========================================================================================\n";

    try
    {
        CUDAContext context;

        const std::vector<std::size_t> vectorSizes = {
            1000,         // 1K
            10000,        // 10K
            100000,       // 100K
            1000000,      // 1M
            10000000      // 10M
        };

        std::cout << std::left
                  << std::setw(12) << "Elements"
                  << std::setw(15) << "CPU Time(ms)"
                  << std::setw(18) << "GPU Kernel(ms)"
                  << std::setw(18) << "Total GPU(ms)"
                  << std::setw(15) << "Kernel Speedup"
                  << std::setw(12) << "Verified"
                  << '\n';
        std::cout << "----------------------------------------------------------------------------------------\n";

        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

        for (std::size_t N : vectorSizes)
        {
            std::vector<float> h_A(N);
            std::vector<float> h_B(N);

            for (std::size_t i = 0; i < N; ++i)
            {
                h_A[i] = dist(rng);
                h_B[i] = dist(rng);
            }

            // --- CPU Execution ---
            std::vector<float> h_C_cpu(N);
            auto t0_cpu = std::chrono::high_resolution_clock::now();
            cpuVectorAdd(h_A, h_B, h_C_cpu);
            auto t1_cpu = std::chrono::high_resolution_clock::now();
            double cpuTimeMs = std::chrono::duration<double, std::milli>(t1_cpu - t0_cpu).count();

            // --- Total GPU Execution (Transfers + Launch + Sync) ---
            DeviceBuffer<float> d_A(N);
            DeviceBuffer<float> d_B(N);
            DeviceBuffer<float> d_C(N);
            std::vector<float> h_C_gpu(N);

            auto t0_gpu = std::chrono::high_resolution_clock::now();

            d_A.copyFromHost(h_A.data(), N);
            d_B.copyFromHost(h_B.data(), N);

            // --- GPU Kernel Timing (CUDAContext Events) ---
            context.startTimer();
            vectorAdd(d_A, d_B, d_C, context);
            context.stopTimer();
            float kernelTimeMs = context.elapsedMilliseconds();

            d_C.copyToHost(h_C_gpu.data(), N);

            auto t1_gpu = std::chrono::high_resolution_clock::now();
            double totalGpuTimeMs = std::chrono::duration<double, std::milli>(t1_gpu - t0_gpu).count();

            // --- Verification ---
            bool isCorrect = verifyResults(h_C_gpu, h_C_cpu, h_A, h_B);
            double speedup = (kernelTimeMs > 0.0f) ? (cpuTimeMs / kernelTimeMs) : 0.0;

            std::cout << std::left
                      << std::setw(12) << N
                      << std::setw(15) << std::fixed << std::setprecision(3) << cpuTimeMs
                      << std::setw(18) << std::fixed << std::setprecision(3) << kernelTimeMs
                      << std::setw(18) << std::fixed << std::setprecision(3) << totalGpuTimeMs
                      << std::setw(15) << std::fixed << std::setprecision(2) << (std::to_string(speedup) + "x")
                      << std::setw(12) << (isCorrect ? "✓ PASS" : "✗ FAIL")
                      << '\n';
        }

        std::cout << "========================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: CUDA benchmarking failed: " << ex.what() << '\n';
        return 1;
    }
}
