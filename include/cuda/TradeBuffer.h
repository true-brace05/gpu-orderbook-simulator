#pragma once

#include "cuda/DeviceBuffer.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

/**
 * @brief Representation of an executed trade on GPU.
 */
struct TradeRecord
{
    int buyOrderId = 0;
    int sellOrderId = 0;
    uint32_t priceTick = 0;
    int executedQuantity = 0;
    uint64_t timestamp = 0;
    uint8_t aggressorSide = 0; // 0 = Buy, 1 = Sell (side of the incoming aggressor order)
};

/**
 * @brief GPU memory container storing executed trade records.
 *
 * TradeBuffer owns a contiguous DeviceBuffer<TradeRecord> array and an atomic counter buffer on GPU.
 * Supports std::vector semantics (allocate, reserve, resize, clear, release, move operations).
 */
class TradeBuffer
{
private:
    DeviceBuffer<TradeRecord> tradesBuf;
    DeviceBuffer<int> tradeCountBuf; // 1 int in GPU memory for atomic counter
    std::size_t activeTradeCount = 0;
    std::size_t capacityElements = 0;

public:
    /**
     * @brief Constructs an empty TradeBuffer with zero allocated GPU capacity.
     */
    TradeBuffer() noexcept = default;

    /**
     * @brief Constructs a TradeBuffer and allocates GPU memory capacity for initialCapacity elements.
     * Throws std::invalid_argument if initialCapacity == 0.
     * Throws std::runtime_error if allocation fails.
     *
     * @param initialCapacity Target trade record capacity to reserve on GPU.
     */
    explicit TradeBuffer(std::size_t initialCapacity);

    /**
     * @brief Destructor. Automatically releases all GPU allocations via DeviceBuffer<T> RAII.
     */
    ~TradeBuffer() noexcept = default;

    // Non-copyable
    TradeBuffer(const TradeBuffer&) = delete;
    TradeBuffer& operator=(const TradeBuffer&) = delete;

    // Movable
    TradeBuffer(TradeBuffer&& other) noexcept;
    TradeBuffer& operator=(TradeBuffer&& other) noexcept;

    /**
     * @brief Allocates GPU memory capacity for capacity trade records.
     * Throws std::invalid_argument if capacity == 0.
     *
     * @param capacity Target element capacity.
     */
    void allocate(std::size_t capacity);

    /**
     * @brief Ensures GPU allocation capacity is at least newCapacity.
     * Preserves existing trade records if reallocation occurs.
     * Throws std::invalid_argument if newCapacity == 0.
     *
     * @param newCapacity Target element capacity.
     */
    void reserve(std::size_t newCapacity);

    /**
     * @brief Sets logical active trade count to newSize, auto-expanding capacity via reserve() if needed.
     *
     * @param newSize Target active trade count.
     */
    void resize(std::size_t newSize);

    /**
     * @brief Resets active trade count to 0 without releasing GPU memory allocation.
     * Guarantees noexcept.
     */
    void clear() noexcept;

    /**
     * @brief Releases all allocated GPU memory and resets counts to 0.
     * Guarantees noexcept.
     */
    void release() noexcept;

    /**
     * @brief Synchronizes active trade record count from GPU device memory to host.
     *
     * @param stream CUDA stream handle for async copy.
     */
    void updateHostCount(cudaStream_t stream = nullptr);

    /**
     * @brief Gets total number of active trade records.
     * @return Active trade count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Gets allocated element capacity.
     * @return Capacity in elements.
     */
    [[nodiscard]] std::size_t capacity() const noexcept;

    /**
     * @brief Checks if buffer is logically empty (size() == 0).
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Checks if all internal DeviceBuffers hold valid GPU allocations.
     * @return True if valid, false otherwise.
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief Gets raw GPU device pointer to contiguous TradeRecord array.
     * @return Raw TradeRecord* pointer on GPU.
     */
    [[nodiscard]] TradeRecord* data() noexcept { return tradesBuf.data(); }
    [[nodiscard]] const TradeRecord* data() const noexcept { return tradesBuf.data(); }

    [[nodiscard]] DeviceBuffer<TradeRecord>& getTradesBuffer() noexcept { return tradesBuf; }
    [[nodiscard]] const DeviceBuffer<TradeRecord>& getTradesBuffer() const noexcept { return tradesBuf; }

    [[nodiscard]] int* tradeCountDevicePtr() noexcept { return tradeCountBuf.data(); }
    [[nodiscard]] const int* tradeCountDevicePtr() const noexcept { return tradeCountBuf.data(); }
};
