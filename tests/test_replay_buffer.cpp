#include "cuda/ReplayBuffer.h"
#include "cuda/CUDAContext.h"
#include "replay/Event.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * @brief Helper to generate sample Event object with distinguishable field values.
 */
Event createSampleEvent(uint64_t timestamp, EventType type, int orderId, Side side, double price, int qty)
{
    Event ev;
    ev.timestamp = timestamp;
    ev.type = type;
    ev.orderId = orderId;
    ev.order.id = orderId;
    ev.order.side = side;
    ev.order.type = OrderType::LIMIT;
    ev.order.price = price;
    ev.order.quantity = qty;
    ev.order.timestamp = timestamp;
    ev.order.displayQuantity = qty;
    ev.order.reserveQuantity = 0;
    return ev;
}

/**
 * @brief Helper to compare two Event objects field-by-field.
 */
bool compareEvents(const Event& a, const Event& b)
{
    return a.timestamp == b.timestamp &&
           a.type == b.type &&
           a.orderId == b.orderId &&
           a.order.id == b.order.id &&
           a.order.side == b.order.side &&
           a.order.type == b.order.type &&
           a.order.price == b.order.price &&
           a.order.quantity == b.order.quantity &&
           a.order.displayQuantity == b.order.displayQuantity &&
           a.order.reserveQuantity == b.order.reserveQuantity;
}

/**
 * @brief Test 1: Default constructor state.
 */
bool testDefaultConstructor()
{
    ReplayBuffer buf;

    if (!buf.empty())
    {
        std::cerr << "  Failure: Default constructor empty() returned false\n";
        return false;
    }
    if (buf.size() != 0)
    {
        std::cerr << "  Failure: Default constructor size() = " << buf.size() << " (expected 0)\n";
        return false;
    }
    if (buf.capacity() != 0)
    {
        std::cerr << "  Failure: Default constructor capacity() = " << buf.capacity() << " (expected 0)\n";
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
 * @brief Test 2: Allocation & capacity construction.
 */
bool testAllocation()
{
    constexpr std::size_t cap = 1000;
    ReplayBuffer buf(cap);

    if (buf.capacity() != cap)
    {
        std::cerr << "  Failure: Constructor capacity() = " << buf.capacity() << " (expected " << cap << ")\n";
        return false;
    }
    if (buf.size() != 0)
    {
        std::cerr << "  Failure: Constructor size() = " << buf.size() << " (expected 0)\n";
        return false;
    }
    if (!buf.empty())
    {
        std::cerr << "  Failure: Newly allocated buffer empty() returned false\n";
        return false;
    }
    if (!buf.isValid() || buf.data() == nullptr)
    {
        std::cerr << "  Failure: Allocated buffer state is invalid or data() is null\n";
        return false;
    }

    // Explicit allocate
    constexpr std::size_t newCap = 2000;
    buf.allocate(newCap);
    if (buf.capacity() != newCap || buf.size() != 0 || !buf.empty())
    {
        std::cerr << "  Failure: allocate(" << newCap << ") failed to update capacity or reset size\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 3: Upload and download batch correctness with CUDA stream.
 */
bool testUploadDownloadCorrectness()
{
    CUDAContext context;
    constexpr std::size_t N = 500;

    std::vector<Event> h_src(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        h_src[i] = createSampleEvent(
            1000000 + i,
            (i % 2 == 0) ? EventType::NEW_ORDER : EventType::CANCEL,
            static_cast<int>(i + 1),
            (i % 3 == 0) ? Side::BUY : Side::SELL,
            150.0 + (i * 0.05),
            10 * static_cast<int>((i % 5) + 1)
        );
    }

    ReplayBuffer d_buf(N);
    d_buf.uploadBatch(h_src.data(), N, context.getStream());

    if (d_buf.size() != N)
    {
        std::cerr << "  Failure: uploadBatch size() = " << d_buf.size() << " (expected " << N << ")\n";
        return false;
    }

    std::vector<Event> h_dst(N);
    d_buf.downloadBatch(h_dst.data(), N, context.getStream());
    context.synchronize();

    for (std::size_t i = 0; i < N; ++i)
    {
        if (!compareEvents(h_src[i], h_dst[i]))
        {
            std::cerr << "  Failure: Mismatch at event index " << i << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 4: Batch appending (appendBatch).
 */
bool testAppendBatch()
{
    CUDAContext context;
    constexpr std::size_t chunk1Size = 300;
    constexpr std::size_t chunk2Size = 200;
    constexpr std::size_t totalSize = chunk1Size + chunk2Size;

    std::vector<Event> h_chunk1(chunk1Size);
    std::vector<Event> h_chunk2(chunk2Size);

    for (std::size_t i = 0; i < chunk1Size; ++i)
    {
        h_chunk1[i] = createSampleEvent(100 + i, EventType::NEW_ORDER, static_cast<int>(i + 1), Side::BUY, 100.0 + i, 5);
    }
    for (std::size_t i = 0; i < chunk2Size; ++i)
    {
        h_chunk2[i] = createSampleEvent(500 + i, EventType::CANCEL, static_cast<int>(chunk1Size + i + 1), Side::SELL, 200.0 + i, 10);
    }

    ReplayBuffer d_buf(chunk1Size); // Initial capacity chunk1Size
    d_buf.uploadBatch(h_chunk1.data(), chunk1Size, context.getStream());
    if (d_buf.size() != chunk1Size)
    {
        std::cerr << "  Failure: Initial upload size = " << d_buf.size() << " (expected " << chunk1Size << ")\n";
        return false;
    }

    // appendBatch should auto-expand capacity to totalSize
    d_buf.appendBatch(h_chunk2.data(), chunk2Size, context.getStream());
    context.synchronize();

    if (d_buf.size() != totalSize)
    {
        std::cerr << "  Failure: Post-append size = " << d_buf.size() << " (expected " << totalSize << ")\n";
        return false;
    }
    if (d_buf.capacity() < totalSize)
    {
        std::cerr << "  Failure: Post-append capacity = " << d_buf.capacity() << " (expected >= " << totalSize << ")\n";
        return false;
    }

    std::vector<Event> h_downloaded(totalSize);
    d_buf.downloadBatch(h_downloaded.data(), totalSize, context.getStream());
    context.synchronize();

    for (std::size_t i = 0; i < chunk1Size; ++i)
    {
        if (!compareEvents(h_chunk1[i], h_downloaded[i]))
        {
            std::cerr << "  Failure: Mismatch in chunk1 element at index " << i << '\n';
            return false;
        }
    }
    for (std::size_t i = 0; i < chunk2Size; ++i)
    {
        if (!compareEvents(h_chunk2[i], h_downloaded[chunk1Size + i]))
        {
            std::cerr << "  Failure: Mismatch in chunk2 element at index " << i << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 5: Auto-expansion and data preservation during reserve/resize.
 */
bool testReserveAndResize()
{
    CUDAContext context;
    constexpr std::size_t N = 100;
    std::vector<Event> h_src(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        h_src[i] = createSampleEvent(i * 10, EventType::NEW_ORDER, static_cast<int>(i), Side::BUY, 50.0 + i, 1);
    }

    ReplayBuffer buf(N);
    buf.uploadBatch(h_src.data(), N, context.getStream());
    context.synchronize();

    // Reserve larger capacity
    constexpr std::size_t expandedCap = 500;
    buf.reserve(expandedCap);
    if (buf.capacity() != expandedCap)
    {
        std::cerr << "  Failure: reserve capacity = " << buf.capacity() << " (expected " << expandedCap << ")\n";
        return false;
    }
    if (buf.size() != N)
    {
        std::cerr << "  Failure: reserve size changed to " << buf.size() << " (expected " << N << ")\n";
        return false;
    }

    // Verify existing data preserved after reserve
    std::vector<Event> h_dst(N);
    buf.downloadBatch(h_dst.data(), N, context.getStream());
    context.synchronize();

    for (std::size_t i = 0; i < N; ++i)
    {
        if (!compareEvents(h_src[i], h_dst[i]))
        {
            std::cerr << "  Failure: Data lost or corrupted after reserve at index " << i << '\n';
            return false;
        }
    }

    // Resize
    constexpr std::size_t resizedSize = 30;
    buf.resize(resizedSize);
    if (buf.size() != resizedSize)
    {
        std::cerr << "  Failure: resize size = " << buf.size() << " (expected " << resizedSize << ")\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 6: Clear and Release.
 */
bool testClearAndRelease()
{
    CUDAContext context;
    constexpr std::size_t N = 200;
    std::vector<Event> h_src(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        h_src[i] = createSampleEvent(i, EventType::NEW_ORDER, static_cast<int>(i), Side::BUY, 10.0, 1);
    }

    ReplayBuffer buf(N);
    buf.uploadBatch(h_src.data(), N, context.getStream());

    buf.clear();
    if (buf.size() != 0 || !buf.empty())
    {
        std::cerr << "  Failure: clear() did not reset size to 0\n";
        return false;
    }
    if (buf.capacity() != N || !buf.isValid())
    {
        std::cerr << "  Failure: clear() modified capacity or invalidated buffer\n";
        return false;
    }

    buf.release();
    if (buf.size() != 0 || buf.capacity() != 0 || buf.isValid() || buf.data() != nullptr)
    {
        std::cerr << "  Failure: release() failed to reset state completely\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 7: Move construction and move assignment.
 */
bool testMoveSemantics()
{
    CUDAContext context;
    constexpr std::size_t N = 150;
    std::vector<Event> h_src(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        h_src[i] = createSampleEvent(i, EventType::NEW_ORDER, static_cast<int>(i), Side::SELL, 99.0, 3);
    }

    ReplayBuffer src(N);
    src.uploadBatch(h_src.data(), N, context.getStream());
    context.synchronize();

    Event* origPtr = src.data();

    // Move constructor
    ReplayBuffer dst(std::move(src));
    if (dst.size() != N || dst.data() != origPtr || !dst.isValid())
    {
        std::cerr << "  Failure: Move constructor failed to transfer state\n";
        return false;
    }
    if (src.size() != 0 || src.capacity() != 0 || src.data() != nullptr || src.isValid())
    {
        std::cerr << "  Failure: Move source was not reset properly after move constructor\n";
        return false;
    }

    // Move assignment
    ReplayBuffer dst2;
    dst2 = std::move(dst);
    if (dst2.size() != N || dst2.data() != origPtr || !dst2.isValid())
    {
        std::cerr << "  Failure: Move assignment failed to transfer state\n";
        return false;
    }
    if (dst.size() != 0 || dst.capacity() != 0 || dst.data() != nullptr || dst.isValid())
    {
        std::cerr << "  Failure: Move source was not reset properly after move assignment\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 8: Invalid arguments and error handling.
 */
bool testInvalidArguments()
{
    // Zero capacity constructor
    bool ctorThrew = false;
    try
    {
        ReplayBuffer buf(0);
    }
    catch (const std::invalid_argument&)
    {
        ctorThrew = true;
    }
    if (!ctorThrew)
    {
        std::cerr << "  Failure: ReplayBuffer(0) did not throw std::invalid_argument\n";
        return false;
    }

    ReplayBuffer buf(100);

    // Null pointer upload
    bool nullUploadThrew = false;
    try
    {
        buf.uploadBatch(nullptr, 50);
    }
    catch (const std::invalid_argument&)
    {
        nullUploadThrew = true;
    }
    if (!nullUploadThrew)
    {
        std::cerr << "  Failure: uploadBatch(nullptr) did not throw std::invalid_argument\n";
        return false;
    }

    // Zero count upload
    std::vector<Event> dummy(10);
    bool zeroUploadThrew = false;
    try
    {
        buf.uploadBatch(dummy.data(), 0);
    }
    catch (const std::invalid_argument&)
    {
        zeroUploadThrew = true;
    }
    if (!zeroUploadThrew)
    {
        std::cerr << "  Failure: uploadBatch(..., 0) did not throw std::invalid_argument\n";
        return false;
    }

    // Download exceeding size()
    CUDAContext context;
    buf.uploadBatch(dummy.data(), 10, context.getStream());
    bool overflowDownloadThrew = false;
    try
    {
        buf.downloadBatch(dummy.data(), 20, context.getStream());
    }
    catch (const std::invalid_argument&)
    {
        overflowDownloadThrew = true;
    }
    if (!overflowDownloadThrew)
    {
        std::cerr << "  Failure: downloadBatch exceeding size() did not throw std::invalid_argument\n";
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
        {"2. Allocation & Capacity", testAllocation},
        {"3. Batch Upload & Download Correctness", testUploadDownloadCorrectness},
        {"4. Batch Appending (appendBatch)", testAppendBatch},
        {"5. Reserve & Resize Auto-Expansion", testReserveAndResize},
        {"6. Clear & Release Operations", testClearAndRelease},
        {"7. Move Semantics (Constructor & Assignment)", testMoveSemantics},
        {"8. Invalid Argument Exception Handling", testInvalidArguments}
    };

    std::cout << "========================================\n";
    std::cout << "     ReplayBuffer Correctness Tests     \n";
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
