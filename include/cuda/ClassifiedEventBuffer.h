#pragma once

#include "cuda/DeviceBuffer.h"
#include "Types.h"

#include <cuda_runtime.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

/**
 * @brief Structured grouping for per-category classification event counters.
 */
struct ClassificationCounters
{
    static constexpr std::size_t NUM_CATEGORIES = 7;
    std::array<std::size_t, NUM_CATEGORIES> counts = {0};

    [[nodiscard]] std::size_t getCount(EventType type) const noexcept
    {
        auto idx = static_cast<std::size_t>(type);
        return (idx < NUM_CATEGORIES) ? counts[idx] : 0;
    }

    void setCount(EventType type, std::size_t val) noexcept
    {
        auto idx = static_cast<std::size_t>(type);
        if (idx < NUM_CATEGORIES)
        {
            counts[idx] = val;
        }
    }

    [[nodiscard]] std::size_t totalCount() const noexcept
    {
        std::size_t sum = 0;
        for (std::size_t c : counts)
        {
            sum += c;
        }
        return sum;
    }

    void clear() noexcept
    {
        counts.fill(0);
    }
};

/**
 * @brief Raw GPU device pointer view for writing/reading ClassifiedEventBuffer index streams in CUDA kernels.
 */
struct ClassifiedEventSoAView
{
    int* addIndices = nullptr;
    int* cancelIndices = nullptr;
    int* modifyIndices = nullptr;
    int* deleteIndices = nullptr;
    int* executeVisibleIndices = nullptr;
    int* executeHiddenIndices = nullptr;
    int* tradingHaltIndices = nullptr;
    int* categoryCounts = nullptr; // Pointer to 7-element GPU device array
};

/**
 * @brief GPU memory container storing event indices for each event category.
 *
 * ClassifiedEventBuffer owns one index buffer (DeviceBuffer<int>) per category plus an atomic
 * counter buffer (DeviceBuffer<int>) on GPU. Exposes generic accessors getIndices(EventType) and
 * getCount(EventType) following std::vector semantics.
 */
class ClassifiedEventBuffer
{
private:
    DeviceBuffer<int> addBuf;
    DeviceBuffer<int> cancelBuf;
    DeviceBuffer<int> modifyBuf;
    DeviceBuffer<int> deleteBuf;
    DeviceBuffer<int> executeVisibleBuf;
    DeviceBuffer<int> executeHiddenBuf;
    DeviceBuffer<int> tradingHaltBuf;

    DeviceBuffer<int> categoryCountsBuf; // 7 int counters in GPU memory
    ClassificationCounters hostCounters;
    std::size_t capacityElements = 0;

public:
    /**
     * @brief Constructs an empty ClassifiedEventBuffer with zero allocated GPU capacity.
     */
    ClassifiedEventBuffer() noexcept = default;

    /**
     * @brief Constructs a ClassifiedEventBuffer and allocates GPU memory capacity for initialCapacity elements per category.
     * Throws std::invalid_argument if initialCapacity == 0.
     * Throws std::runtime_error if allocation fails.
     *
     * @param initialCapacity Maximum index capacity to reserve for each category index array.
     */
    explicit ClassifiedEventBuffer(std::size_t initialCapacity);

    /**
     * @brief Destructor. Automatically releases all GPU allocations via DeviceBuffer<T> RAII.
     */
    ~ClassifiedEventBuffer() noexcept = default;

    // Non-copyable
    ClassifiedEventBuffer(const ClassifiedEventBuffer&) = delete;
    ClassifiedEventBuffer& operator=(const ClassifiedEventBuffer&) = delete;

    // Movable
    ClassifiedEventBuffer(ClassifiedEventBuffer&& other) noexcept;
    ClassifiedEventBuffer& operator=(ClassifiedEventBuffer&& other) noexcept;

    /**
     * @brief Allocates GPU memory capacity for capacity elements across all category index buffers.
     * Throws std::invalid_argument if capacity == 0.
     *
     * @param capacity Target element capacity per category.
     */
    void allocate(std::size_t capacity);

    /**
     * @brief Ensures GPU allocation capacity per category is at least newCapacity elements.
     * Preserves existing category indices if reallocation occurs.
     * Throws std::invalid_argument if newCapacity == 0.
     *
     * @param newCapacity Target element capacity per category.
     */
    void reserve(std::size_t newCapacity);

    /**
     * @brief Resets event classification counters to 0 without releasing GPU memory allocation.
     * Guarantees noexcept.
     */
    void clear() noexcept;

    /**
     * @brief Releases all allocated GPU memory and resets size and capacity to 0.
     * Guarantees noexcept.
     */
    void release() noexcept;

    /**
     * @brief Synchronizes per-category atomic counts from GPU device memory to host counters.
     *
     * @param stream CUDA stream handle for async copy.
     */
    void updateHostCounters(cudaStream_t stream = nullptr);

    /**
     * @brief Gets total number of classified events across all categories.
     * @return Total classified event count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Gets allocated capacity per category index array.
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
     * @brief Gets the classified event count for a specific EventType.
     *
     * @param type Target EventType.
     * @return Category event count.
     */
    [[nodiscard]] std::size_t getCount(EventType type) const noexcept;

    /**
     * @brief Gets full ClassificationCounters struct containing host per-category counts.
     * @return Const reference to host ClassificationCounters.
     */
    [[nodiscard]] const ClassificationCounters& getCounters() const noexcept;

    /**
     * @brief Generic raw device pointer accessor for a category index array.
     *
     * @param type Target EventType.
     * @return Raw int* device pointer to index array, or nullptr if unallocated.
     */
    [[nodiscard]] int* getIndices(EventType type) noexcept;
    [[nodiscard]] const int* getIndices(EventType type) const noexcept;

    /**
     * @brief Generic DeviceBuffer<int> accessor for a category index array.
     *
     * @param type Target EventType.
     * @return Reference to internal DeviceBuffer<int>.
     */
    [[nodiscard]] DeviceBuffer<int>& getIndexBuffer(EventType type);
    [[nodiscard]] const DeviceBuffer<int>& getIndexBuffer(EventType type) const;

    /**
     * @brief Gets raw device pointer to atomic category counts array (7 ints in GPU memory).
     * @return Raw int* pointer on GPU.
     */
    [[nodiscard]] int* categoryCountsDevicePtr() noexcept { return categoryCountsBuf.data(); }
    [[nodiscard]] const int* categoryCountsDevicePtr() const noexcept { return categoryCountsBuf.data(); }

    /**
     * @brief Gets raw device view for launching CUDA kernels.
     * @return ClassifiedEventSoAView containing raw device pointers.
     */
    [[nodiscard]] ClassifiedEventSoAView getDeviceView() noexcept;
};
