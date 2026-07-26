#pragma once

#include "cuda/DeviceBuffer.h"
#include "replay/Event.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <stdexcept>
#include <utility>

/**
 * @brief Lightweight domain-specific wrapper around DeviceBuffer<Event> for managing GPU replay event batches.
 *
 * ReplayBuffer manages logical replay batch sizing (active event count, capacity, clear, reserve, append)
 * while delegating raw GPU memory allocations, RAII semantics, host-device transfers, and CUDA error checking
 * to DeviceBuffer<Event>.
 */
class ReplayBuffer
{
private:
    DeviceBuffer<Event> deviceBuffer;
    std::size_t elementCount = 0;

public:
    /**
     * @brief Constructs an empty ReplayBuffer with no allocated GPU capacity.
     */
    ReplayBuffer() noexcept = default;

    /**
     * @brief Constructs a ReplayBuffer and pre-allocates GPU capacity for the given number of Event elements.
     * Throws std::invalid_argument if initialCapacity == 0.
     * Throws std::runtime_error if GPU memory allocation fails.
     *
     * @param initialCapacity Number of Event items to reserve space for on GPU.
     */
    explicit ReplayBuffer(std::size_t initialCapacity);

    /**
     * @brief Destructor. Automatically frees GPU resources via DeviceBuffer<Event> RAII.
     */
    ~ReplayBuffer() noexcept = default;

    // Non-copyable
    ReplayBuffer(const ReplayBuffer&) = delete;
    ReplayBuffer& operator=(const ReplayBuffer&) = delete;

    // Movable
    ReplayBuffer(ReplayBuffer&& other) noexcept;
    ReplayBuffer& operator=(ReplayBuffer&& other) noexcept;

    /**
     * @brief Allocates GPU memory capacity for capacity elements.
     * Releases any previously allocated memory and resets logical batch size to 0.
     * Throws std::invalid_argument if capacity == 0.
     * Throws std::runtime_error if cudaMalloc fails.
     *
     * @param capacity Number of Event items to allocate.
     */
    void allocate(std::size_t capacity);

    /**
     * @brief Ensures GPU allocation capacity is at least newCapacity elements.
     * Preserves existing events (up to min(elementCount, newCapacity)) if reallocation occurs.
     * Throws std::invalid_argument if newCapacity == 0.
     * Throws std::runtime_error if CUDA allocation or copy fails.
     *
     * @param newCapacity Target element capacity.
     */
    void reserve(std::size_t newCapacity);

    /**
     * @brief Adjusts the logical batch size to newSize, ensuring sufficient capacity.
     * If newSize > capacity(), automatically expands capacity via reserve(newSize).
     * Throws std::invalid_argument if newSize == 0.
     *
     * @param newSize Target logical size.
     */
    void resize(std::size_t newSize);

    /**
     * @brief Uploads a batch of host Event objects to GPU memory starting at index 0.
     * Overwrites any existing logical batch content and sets size() = batchCount.
     * Auto-expands GPU capacity if batchCount > capacity().
     * Throws std::invalid_argument if hostEvents == nullptr or batchCount == 0.
     * Throws std::runtime_error if CUDA transfer fails.
     *
     * @param hostEvents Pointer to host source Event array.
     * @param batchCount Number of Event items to upload.
     * @param stream CUDA stream handle for asynchronous transfer (defaults to 0).
     */
    void uploadBatch(const Event* hostEvents, std::size_t batchCount, cudaStream_t stream = nullptr);

    /**
     * @brief Appends a batch of host Event objects to the end of the current logical batch.
     * Expands size() by batchCount. Auto-expands GPU capacity if needed.
     * Throws std::invalid_argument if hostEvents == nullptr or batchCount == 0.
     * Throws std::runtime_error if CUDA transfer fails.
     *
     * @param hostEvents Pointer to host source Event array.
     * @param batchCount Number of Event items to append.
     * @param stream CUDA stream handle for asynchronous transfer (defaults to 0).
     */
    void appendBatch(const Event* hostEvents, std::size_t batchCount, cudaStream_t stream = nullptr);

    /**
     * @brief Asynchronously copies count events from GPU device memory to host CPU memory.
     * Intended primarily for testing and verification.
     * Throws std::invalid_argument if hostEvents == nullptr, count == 0, or count > size().
     * Throws std::runtime_error if CUDA transfer fails.
     *
     * @param hostEvents Pointer to destination host Event array.
     * @param count Number of Event items to download.
     * @param stream CUDA stream handle for asynchronous transfer (defaults to 0).
     */
    void downloadBatch(Event* hostEvents, std::size_t count, cudaStream_t stream = nullptr) const;

    /**
     * @brief Resets current logical batch size to 0 without releasing GPU memory allocation.
     * Guarantees noexcept.
     */
    void clear() noexcept;

    /**
     * @brief Releases allocated GPU memory and resets size and capacity to 0.
     * Guarantees noexcept.
     */
    void release() noexcept;

    /**
     * @brief Gets the current logical batch size (number of valid events uploaded/stored).
     * @return Logical event count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Gets total allocated element capacity in GPU device memory.
     * @return Capacity in Event elements.
     */
    [[nodiscard]] std::size_t capacity() const noexcept;

    /**
     * @brief Gets size of logical batch in bytes.
     * @return Size in bytes.
     */
    [[nodiscard]] std::size_t sizeBytes() const noexcept;

    /**
     * @brief Gets total allocated capacity in bytes.
     * @return Capacity in bytes.
     */
    [[nodiscard]] std::size_t capacityBytes() const noexcept;

    /**
     * @brief Checks if logical batch is empty (size() == 0).
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Gets raw GPU device pointer to the start of the buffer.
     * @return Raw Event* device pointer, or nullptr if unallocated.
     */
    [[nodiscard]] Event* data() noexcept;

    /**
     * @brief Gets constant raw GPU device pointer to the start of the buffer.
     * @return Constant Event* device pointer, or nullptr if unallocated.
     */
    [[nodiscard]] const Event* data() const noexcept;

    /**
     * @brief Checks if underlying DeviceBuffer holds valid allocated memory.
     * @return True if valid, false otherwise.
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief Gets direct reference to the underlying DeviceBuffer<Event>.
     * @return Const reference to internal DeviceBuffer<Event>.
     */
    [[nodiscard]] const DeviceBuffer<Event>& getDeviceBuffer() const noexcept;
};
