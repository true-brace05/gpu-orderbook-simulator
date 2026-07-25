#include "cuda/DeviceBuffer.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>

/**
 * @brief Test 1: Default constructor produces empty, invalid state.
 */
bool testDefaultConstructor()
{
    DeviceBuffer<int> buf;

    if (!buf.empty())
    {
        std::cerr << "  Failure: Default constructor empty() returned false\n";
        return false;
    }
    if (buf.size() != 0)
    {
        std::cerr << "  Failure: Default constructor size() returned " << buf.size() << " (expected 0)\n";
        return false;
    }
    if (buf.data() != nullptr)
    {
        std::cerr << "  Failure: Default constructor data() returned non-null pointer\n";
        return false;
    }
    if (buf.isValid())
    {
        std::cerr << "  Failure: Default constructor isValid() returned true\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 2: Allocation of GPU device memory.
 */
bool testAllocation()
{
    DeviceBuffer<int> buf;
    constexpr std::size_t count = 1024;
    buf.allocate(count);

    if (buf.empty())
    {
        std::cerr << "  Failure: Allocated buffer empty() returned true\n";
        return false;
    }
    if (buf.size() != count)
    {
        std::cerr << "  Failure: Allocated buffer size() = " << buf.size() << " (expected " << count << ")\n";
        return false;
    }
    if (buf.sizeBytes() != count * sizeof(int))
    {
        std::cerr << "  Failure: Allocated buffer sizeBytes() = " << buf.sizeBytes()
                  << " (expected " << (count * sizeof(int)) << ")\n";
        return false;
    }
    if (!buf.isValid())
    {
        std::cerr << "  Failure: Allocated buffer isValid() returned false\n";
        return false;
    }
    if (buf.data() == nullptr)
    {
        std::cerr << "  Failure: Allocated buffer data() returned nullptr\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 3: Repeated allocation (reallocation) releases previous memory and updates state.
 */
bool testReallocation()
{
    DeviceBuffer<int> buf;
    constexpr std::size_t initialCount = 1024;
    constexpr std::size_t newCount = 2048;

    buf.allocate(initialCount);
    if (buf.size() != initialCount || !buf.isValid())
    {
        std::cerr << "  Failure: Initial allocation failed in reallocation test\n";
        return false;
    }

    buf.allocate(newCount);
    if (buf.size() != newCount)
    {
        std::cerr << "  Failure: Reallocated buffer size() = " << buf.size() << " (expected " << newCount << ")\n";
        return false;
    }
    if (buf.sizeBytes() != newCount * sizeof(int))
    {
        std::cerr << "  Failure: Reallocated buffer sizeBytes() = " << buf.sizeBytes()
                  << " (expected " << (newCount * sizeof(int)) << ")\n";
        return false;
    }
    if (!buf.isValid())
    {
        std::cerr << "  Failure: Reallocated buffer isValid() returned false\n";
        return false;
    }
    if (buf.data() == nullptr)
    {
        std::cerr << "  Failure: Reallocated buffer data() returned nullptr\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 4: Move construction transfers ownership and leaves source empty.
 */
bool testMoveConstructor()
{
    constexpr std::size_t count = 512;
    DeviceBuffer<int> source(count);

    int* originalPtr = source.data();
    if (originalPtr == nullptr)
    {
        std::cerr << "  Failure: Source buffer allocation returned nullptr before move\n";
        return false;
    }

    DeviceBuffer<int> destination(std::move(source));

    if (destination.size() != count)
    {
        std::cerr << "  Failure: Move-constructed destination size() = " << destination.size()
                  << " (expected " << count << ")\n";
        return false;
    }
    if (destination.data() != originalPtr)
    {
        std::cerr << "  Failure: Move-constructed destination data() does not match original pointer\n";
        return false;
    }
    if (!destination.isValid())
    {
        std::cerr << "  Failure: Move-constructed destination isValid() returned false\n";
        return false;
    }

    if (!source.empty() || source.size() != 0 || source.data() != nullptr || source.isValid())
    {
        std::cerr << "  Failure: Move source buffer was not properly reset to empty state\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 5: Move assignment transfers ownership and cleans up destination.
 */
bool testMoveAssignment()
{
    DeviceBuffer<int> src(256);
    DeviceBuffer<int> dst(128);

    int* originalSrcPtr = src.data();
    if (originalSrcPtr == nullptr)
    {
        std::cerr << "  Failure: Source buffer data() returned nullptr prior to move assignment\n";
        return false;
    }

    dst = std::move(src);

    if (dst.size() != 256)
    {
        std::cerr << "  Failure: Move-assigned destination size() = " << dst.size() << " (expected 256)\n";
        return false;
    }
    if (dst.data() != originalSrcPtr)
    {
        std::cerr << "  Failure: Move-assigned destination data() does not match source pointer\n";
        return false;
    }
    if (!dst.isValid())
    {
        std::cerr << "  Failure: Move-assigned destination isValid() returned false\n";
        return false;
    }

    if (!src.empty() || src.size() != 0 || src.data() != nullptr || src.isValid())
    {
        std::cerr << "  Failure: Move-assigned source buffer was not properly reset\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 6: Host -> Device -> Host round-trip with element-wise validation.
 */
bool testHostDeviceRoundTrip()
{
    constexpr std::size_t N = 1024;
    std::vector<int> h_src(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        h_src[i] = static_cast<int>(i * 3 + 7);
    }

    DeviceBuffer<int> d_buf(N);
    d_buf.copyFromHost(h_src.data(), h_src.size());

    std::vector<int> h_dst(N, 0);
    d_buf.copyToHost(h_dst.data(), h_dst.size());

    for (std::size_t i = 0; i < N; ++i)
    {
        if (h_dst[i] != h_src[i])
        {
            std::cerr << "  Failure: Mismatch at index " << i << ": expected " << h_src[i]
                      << ", got " << h_dst[i] << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 7: zeroMemory fills GPU allocation with zeros.
 */
bool testZeroMemory()
{
    constexpr std::size_t N = 512;
    std::vector<int> h_data(N, 42);

    DeviceBuffer<int> d_buf(N);
    d_buf.copyFromHost(h_data.data(), N);

    d_buf.zeroMemory();

    std::vector<int> h_res(N, 99);
    d_buf.copyToHost(h_res.data(), N);

    for (std::size_t i = 0; i < N; ++i)
    {
        if (h_res[i] != 0)
        {
            std::cerr << "  Failure: Element at index " << i << " is " << h_res[i] << " (expected 0)\n";
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 8: Invalid allocation with 0 count throws std::invalid_argument.
 */
bool testInvalidAllocation()
{
    bool ctorThrew = false;
    try
    {
        DeviceBuffer<int> buf(0);
    }
    catch (const std::invalid_argument&)
    {
        ctorThrew = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: DeviceBuffer(0) threw unexpected exception type\n";
        return false;
    }

    if (!ctorThrew)
    {
        std::cerr << "  Failure: DeviceBuffer(0) did not throw std::invalid_argument\n";
        return false;
    }

    bool allocThrew = false;
    try
    {
        DeviceBuffer<int> buf(100);
        buf.allocate(0);
    }
    catch (const std::invalid_argument&)
    {
        allocThrew = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: allocate(0) threw unexpected exception type\n";
        return false;
    }

    if (!allocThrew)
    {
        std::cerr << "  Failure: allocate(0) did not throw std::invalid_argument\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 9: Null pointer copy operations throw std::invalid_argument.
 */
bool testNullPointerCopy()
{
    DeviceBuffer<int> d_buf(100);

    bool fromNullThrew = false;
    try
    {
        d_buf.copyFromHost(nullptr, 100);
    }
    catch (const std::invalid_argument&)
    {
        fromNullThrew = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: copyFromHost(nullptr) threw unexpected exception type\n";
        return false;
    }

    if (!fromNullThrew)
    {
        std::cerr << "  Failure: copyFromHost(nullptr) did not throw std::invalid_argument\n";
        return false;
    }

    bool toNullThrew = false;
    try
    {
        d_buf.copyToHost(nullptr, 100);
    }
    catch (const std::invalid_argument&)
    {
        toNullThrew = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: copyToHost(nullptr) threw unexpected exception type\n";
        return false;
    }

    if (!toNullThrew)
    {
        std::cerr << "  Failure: copyToHost(nullptr) did not throw std::invalid_argument\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 10: Overflow copy requests (count > elementCount) throw std::invalid_argument.
 */
bool testOverflowCopy()
{
    DeviceBuffer<int> d_buf(100);
    std::vector<int> hostData(200, 1);

    bool fromOverflowThrew = false;
    try
    {
        d_buf.copyFromHost(hostData.data(), 200);
    }
    catch (const std::invalid_argument&)
    {
        fromOverflowThrew = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: copyFromHost overflow threw unexpected exception type\n";
        return false;
    }

    if (!fromOverflowThrew)
    {
        std::cerr << "  Failure: copyFromHost overflow did not throw std::invalid_argument\n";
        return false;
    }

    bool toOverflowThrew = false;
    try
    {
        d_buf.copyToHost(hostData.data(), 200);
    }
    catch (const std::invalid_argument&)
    {
        toOverflowThrew = true;
    }
    catch (...)
    {
        std::cerr << "  Failure: copyToHost overflow threw unexpected exception type\n";
        return false;
    }

    if (!toOverflowThrew)
    {
        std::cerr << "  Failure: copyToHost overflow did not throw std::invalid_argument\n";
        return false;
    }

    return true;
}

struct TestCase
{
    std::string name;
    bool (*func)();
};

int main()
{
    const std::vector<TestCase> tests = {
        {"1. Default Constructor", testDefaultConstructor},
        {"2. Allocation", testAllocation},
        {"3. Repeated Allocation (Reallocation)", testReallocation},
        {"4. Move Constructor", testMoveConstructor},
        {"5. Move Assignment", testMoveAssignment},
        {"6. Host -> Device -> Host Round-Trip", testHostDeviceRoundTrip},
        {"7. zeroMemory()", testZeroMemory},
        {"8. Invalid Allocation (Zero Count)", testInvalidAllocation},
        {"9. Null Pointer Copy Verification", testNullPointerCopy},
        {"10. Overflow Copy Verification", testOverflowCopy}
    };

    std::cout << "========================================\n";
    std::cout << "   DeviceBuffer<T> Correctness Tests    \n";
    std::cout << "========================================\n";

    std::size_t passedCount = 0;

    for (const auto& test : tests)
    {
        std::cout << "Running: " << test.name << "... ";
        try
        {
            if (test.func())
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
