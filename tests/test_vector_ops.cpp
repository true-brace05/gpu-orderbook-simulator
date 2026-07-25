#include "cuda/vector_ops.h"
#include "cuda/CUDAContext.h"
#include "cuda/DeviceBuffer.h"

#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <stdexcept>
#include <string>

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
 * @brief Helper to validate element-by-element match between GPU result and CPU reference.
 */
bool verifyResults(const std::vector<float>& gpuResult, const std::vector<float>& cpuReference, float tolerance = 1e-5f)
{
    if (gpuResult.size() != cpuReference.size())
    {
        std::cerr << "  Failure: Size mismatch between GPU result (" << gpuResult.size()
                  << ") and CPU reference (" << cpuReference.size() << ")\n";
        return false;
    }

    for (std::size_t i = 0; i < gpuResult.size(); ++i)
    {
        if (std::abs(gpuResult[i] - cpuReference[i]) > tolerance)
        {
            std::cerr << "  Failure: Element mismatch at index " << i
                      << ": GPU=" << gpuResult[i] << ", CPU=" << cpuReference[i] << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 1: Small vectors (N = 16).
 */
bool testSmallVector(CUDAContext& context)
{
    constexpr std::size_t N = 16;
    std::vector<float> h_A(N);
    std::vector<float> h_B(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        h_A[i] = static_cast<float>(i * 2 + 1);
        h_B[i] = static_cast<float>(i * 3 + 5);
    }

    std::vector<float> h_C_cpu;
    cpuVectorAdd(h_A, h_B, h_C_cpu);

    DeviceBuffer<float> d_A(N);
    DeviceBuffer<float> d_B(N);
    DeviceBuffer<float> d_C(N);

    d_A.copyFromHost(h_A.data(), N);
    d_B.copyFromHost(h_B.data(), N);

    vectorAdd(d_A, d_B, d_C, context);

    std::vector<float> h_C_gpu(N);
    d_C.copyToHost(h_C_gpu.data(), N);

    return verifyResults(h_C_gpu, h_C_cpu);
}

/**
 * @brief Test 2: Large vectors (N = 1,000,000).
 */
bool testLargeVector(CUDAContext& context)
{
    constexpr std::size_t N = 1000000;
    std::vector<float> h_A(N);
    std::vector<float> h_B(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        h_A[i] = static_cast<float>(i % 100) * 0.5f;
        h_B[i] = static_cast<float>(i % 200) * 0.25f;
    }

    std::vector<float> h_C_cpu;
    cpuVectorAdd(h_A, h_B, h_C_cpu);

    DeviceBuffer<float> d_A(N);
    DeviceBuffer<float> d_B(N);
    DeviceBuffer<float> d_C(N);

    d_A.copyFromHost(h_A.data(), N);
    d_B.copyFromHost(h_B.data(), N);

    vectorAdd(d_A, d_B, d_C, context);

    std::vector<float> h_C_gpu(N);
    d_C.copyToHost(h_C_gpu.data(), N);

    return verifyResults(h_C_gpu, h_C_cpu);
}

/**
 * @brief Test 3: Random vectors (N = 65,536).
 */
bool testRandomVector(CUDAContext& context)
{
    constexpr std::size_t N = 65536;
    std::vector<float> h_A(N);
    std::vector<float> h_B(N);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);

    for (std::size_t i = 0; i < N; ++i)
    {
        h_A[i] = dist(rng);
        h_B[i] = dist(rng);
    }

    std::vector<float> h_C_cpu;
    cpuVectorAdd(h_A, h_B, h_C_cpu);

    DeviceBuffer<float> d_A(N);
    DeviceBuffer<float> d_B(N);
    DeviceBuffer<float> d_C(N);

    d_A.copyFromHost(h_A.data(), N);
    d_B.copyFromHost(h_B.data(), N);

    vectorAdd(d_A, d_B, d_C, context);

    std::vector<float> h_C_gpu(N);
    d_C.copyToHost(h_C_gpu.data(), N);

    return verifyResults(h_C_gpu, h_C_cpu);
}

/**
 * @brief Test 4: Empty vectors (N = 0).
 */
bool testEmptyVector(CUDAContext& context)
{
    DeviceBuffer<float> d_A;
    DeviceBuffer<float> d_B;
    DeviceBuffer<float> d_C;

    try
    {
        vectorAdd(d_A, d_B, d_C, context);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "  Failure: Empty vector addition threw unexpected exception: " << ex.what() << '\n';
        return false;
    }

    return true;
}

/**
 * @brief Test 5: Size mismatch exception.
 */
bool testSizeMismatch(CUDAContext& context)
{
    DeviceBuffer<float> d_A(100);
    DeviceBuffer<float> d_B(50);
    DeviceBuffer<float> d_C(100);

    bool threw = false;
    try
    {
        vectorAdd(d_A, d_B, d_C, context);
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: Size mismatch threw unexpected exception type\n";
        return false;
    }

    if (!threw)
    {
        std::cerr << "  Failure: Size mismatch did not throw std::invalid_argument\n";
        return false;
    }

    return true;
}

struct TestCase
{
    std::string name;
    bool (*func)(CUDAContext&);
};

int main()
{
    std::cout << "========================================\n";
    std::cout << "     GPU Vector Operations Unit Tests   \n";
    std::cout << "========================================\n";

    try
    {
        CUDAContext context;

        const std::vector<TestCase> tests = {
            {"1. Small Vectors (N=16)", testSmallVector},
            {"2. Large Vectors (N=1,000,000)", testLargeVector},
            {"3. Random Vectors (N=65,536)", testRandomVector},
            {"4. Empty Vectors (N=0)", testEmptyVector},
            {"5. Size Mismatch Exception", testSizeMismatch}
        };

        std::size_t passedCount = 0;

        for (const auto& test : tests)
        {
            std::cout << "Running: " << test.name << "... ";
            try
            {
                if (test.func(context))
                {
                    std::cout << "✓ PASS\n";
                    ++passedCount;
                }
                else
                {
                    std::cout << "✗ FAIL\n";
                }
            }
            catch (const std::exception& ex)
            {
                std::cout << "✗ FAIL (Unhandled Exception: " << ex.what() << ")\n";
            }
            catch (...)
            {
                std::cout << "✗ FAIL (Unknown Exception)\n";
            }
        }

        std::cout << "========================================\n";
        std::cout << "Summary: " << passedCount << " / " << tests.size() << " tests passed.\n";
        std::cout << "========================================\n";

        return (passedCount == tests.size()) ? 0 : 1;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: Unable to initialize CUDAContext: " << ex.what() << '\n';
        return 1;
    }
}
