#pragma once

#include "cuda/DeviceBuffer.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

/**
 * @brief CSR-style representation of a single GPU price level.
 */
struct PriceLevel
{
    uint32_t priceTick = 0;    // Price converted to integer tick (e.g. price / tickSize)
    int totalQuantity = 0;     // Sum of quantities of active Add orders at this price level
    int firstOrder = -1;       // Starting index offset in shared orderIndices buffer (CSR layout)
    int orderCount = 0;        // Number of active orders at this price level
    uint8_t side = 0;          // 0 = Buy, 1 = Sell (Buy & Sell at same price are distinct)
};

/**
 * @brief GPU memory container storing aggregated price levels alongside a CSR order-index mapping.
 *
 * PriceLevelBuffer stores contiguous PriceLevel structures in levelsBuf and CSR order-to-level
 * mappings in orderIndicesBuf. Supports std::vector semantics (allocate, reserve, resize, clear, release).
 */
class PriceLevelBuffer
{
private:
    DeviceBuffer<PriceLevel> levelsBuf;
    DeviceBuffer<int> orderIndicesBuf;   // Shared order index buffer (CSR contiguous mapping)
    DeviceBuffer<int> levelCountBuf;     // 1 int in GPU memory for atomic counter
    std::size_t activeLevelCount = 0;
    std::size_t activeOrderCount = 0;

public:
    /**
     * @brief Constructs an empty PriceLevelBuffer with zero allocated GPU capacity.
     */
    PriceLevelBuffer() noexcept = default;

    /**
     * @brief Constructs a PriceLevelBuffer and allocates GPU memory capacity for initialCapacity elements.
     * Throws std::invalid_argument if initialCapacity == 0.
     * Throws std::runtime_error if allocation fails.
     *
     * @param initialCapacity Target level/order capacity to reserve on GPU.
     */
    explicit PriceLevelBuffer(std::size_t initialCapacity);

    /**
     * @brief Destructor. Automatically releases all GPU allocations via DeviceBuffer<T> RAII.
     */
    ~PriceLevelBuffer() noexcept = default;

    // Non-copyable
    PriceLevelBuffer(const PriceLevelBuffer&) = delete;
    PriceLevelBuffer& operator=(const PriceLevelBuffer&) = delete;

    // Movable
    PriceLevelBuffer(PriceLevelBuffer&& other) noexcept;
    PriceLevelBuffer& operator=(PriceLevelBuffer&& other) noexcept;

    /**
     * @brief Allocates GPU memory capacity for capacity elements across levels and order indices buffers.
     * Throws std::invalid_argument if capacity == 0.
     *
     * @param capacity Target element capacity.
     */
    void allocate(std::size_t capacity);

    /**
     * @brief Ensures GPU allocation capacity across buffers is at least newCapacity.
     * Preserves existing price levels and order index mappings if reallocation occurs.
     * Throws std::invalid_argument if newCapacity == 0.
     *
     * @param newCapacity Target element capacity.
     */
    void reserve(std::size_t newCapacity);

    /**
     * @brief Sets logical active level count to newSize, auto-expanding capacity via reserve() if needed.
     *
     * @param newSize Target active level count.
     */
    void resize(std::size_t newSize);

    /**
     * @brief Resets active level count to 0 without releasing GPU memory allocation.
     * Guarantees noexcept.
     */
    void clear() noexcept;

    /**
     * @brief Releases all allocated GPU memory across buffers and resets counts to 0.
     * Guarantees noexcept.
     */
    void release() noexcept;

    /**
     * @brief Synchronizes active price level count from GPU device memory to host.
     *
     * @param stream CUDA stream handle for async copy.
     */
    void updateHostCount(cudaStream_t stream = nullptr);

    /**
     * @brief Sets the total number of orders mapped in the CSR order indices buffer.
     * @param count Total orders mapped.
     */
    void setOrderCount(std::size_t count) noexcept { activeOrderCount = count; }

    /**
     * @brief Gets total number of unique active price levels.
     * @return Unique price level count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Gets total number of active orders mapped across all price levels.
     * @return Total active order count.
     */
    [[nodiscard]] std::size_t orderCount() const noexcept;

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
     * @brief Gets raw GPU device pointer to contiguous PriceLevel array.
     * @return Raw PriceLevel* pointer on GPU.
     */
    [[nodiscard]] PriceLevel* data() noexcept { return levelsBuf.data(); }
    [[nodiscard]] const PriceLevel* data() const noexcept { return levelsBuf.data(); }

    /**
     * @brief Gets raw GPU device pointer to CSR order indices mapping array.
     * @return Raw int* pointer on GPU.
     */
    [[nodiscard]] int* orderIndicesData() noexcept { return orderIndicesBuf.data(); }
    [[nodiscard]] const int* orderIndicesData() const noexcept { return orderIndicesBuf.data(); }

    [[nodiscard]] DeviceBuffer<PriceLevel>& getLevelsBuffer() noexcept { return levelsBuf; }
    [[nodiscard]] const DeviceBuffer<PriceLevel>& getLevelsBuffer() const noexcept { return levelsBuf; }

    [[nodiscard]] DeviceBuffer<int>& getOrderIndicesBuffer() noexcept { return orderIndicesBuf; }
    [[nodiscard]] const DeviceBuffer<int>& getOrderIndicesBuffer() const noexcept { return orderIndicesBuf; }

    [[nodiscard]] int* levelCountDevicePtr() noexcept { return levelCountBuf.data(); }
    [[nodiscard]] const int* levelCountDevicePtr() const noexcept { return levelCountBuf.data(); }
};
