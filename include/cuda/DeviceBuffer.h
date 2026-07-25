#pragma once

#include <cuda_runtime.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace detail
{
    /**
     * TODO: Move checkDeviceBufferCudaError() into a shared CUDAUtils.h header in future iterations.
     * @brief Helper function for checking CUDA runtime API return codes in DeviceBuffer template methods.
     * Throws std::runtime_error with formatted message on failure.
     */
    inline void checkDeviceBufferCudaError(cudaError_t result, const std::string& message)
    {
        if (result != cudaSuccess)
        {
            throw std::runtime_error(message + ": " + cudaGetErrorString(result));
        }
    }
} // namespace detail

/**
 * @brief RAII wrapper for typed CUDA device memory allocations and synchronous host-device memory transfers.
 *
 * DeviceBuffer enforces move-only semantics to guarantee single ownership of device memory.
 * All device memory allocated via cudaMalloc is automatically freed in the destructor using cudaFree.
 *
 * @tparam T Type of elements stored in the GPU device buffer.
 */
template <typename T>
class DeviceBuffer
{
private:
    T* devicePtr = nullptr;
    std::size_t elementCount = 0;

public:
    /**
     * @brief Constructs an empty DeviceBuffer with no allocated GPU memory.
     */
    DeviceBuffer() noexcept = default;

    /**
     * @brief Constructs a DeviceBuffer and allocates GPU memory for count elements.
     * Throws std::invalid_argument if count == 0.
     * Throws std::runtime_error if cudaMalloc fails.
     *
     * @param count Number of elements to allocate.
     */
    explicit DeviceBuffer(std::size_t count);

    /**
     * @brief Destructor. Releases allocated GPU memory.
     * Guarantees noexcept and will never throw exceptions.
     */
    ~DeviceBuffer() noexcept;

    // Non-copyable
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;

    // Movable
    DeviceBuffer(DeviceBuffer&& other) noexcept;
    DeviceBuffer& operator=(DeviceBuffer&& other) noexcept;

    /**
     * @brief Allocates GPU device memory for count elements.
     * Releases any previously allocated memory first.
     * Throws std::invalid_argument if count == 0.
     * Throws std::runtime_error if cudaMalloc fails.
     *
     * @param count Number of elements to allocate.
     */
    void allocate(std::size_t count);

    /**
     * @brief Releases allocated GPU device memory via cudaFree and resets internal pointers.
     * Guarantees noexcept.
     */
    void release() noexcept;

    /**
     * @brief Synchronously copies count elements from host CPU memory to GPU device memory.
     * Throws std::invalid_argument if hostData is nullptr, count == 0, or count exceeds elementCount.
     * Throws std::runtime_error if cudaMemcpy fails.
     *
     * @param hostData Pointer to source host memory.
     * @param count Number of elements to copy.
     */
    void copyFromHost(const T* hostData, std::size_t count);

    /**
     * @brief Synchronously copies count elements from GPU device memory to host CPU memory.
     * Throws std::invalid_argument if hostData is nullptr, count == 0, or count exceeds elementCount.
     * Throws std::runtime_error if cudaMemcpy fails.
     *
     * @param hostData Pointer to destination host memory.
     * @param count Number of elements to copy.
     */
    void copyToHost(T* hostData, std::size_t count) const;

    /**
     * @brief Fills the allocated GPU memory buffer with zeros using cudaMemset.
     * Throws std::runtime_error if cudaMemset fails or if buffer is unallocated.
     */
    void zeroMemory();

    /**
     * @brief Gets the raw pointer to the GPU device memory.
     * @return Raw device pointer T* or nullptr if unallocated.
     */
    [[nodiscard]] T* data() noexcept;

    /**
     * @brief Gets the constant raw pointer to the GPU device memory.
     * @return Constant raw device pointer const T* or nullptr if unallocated.
     */
    [[nodiscard]] const T* data() const noexcept;

    /**
     * @brief Gets the number of elements allocated in the GPU buffer.
     * @return Element count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Gets the total capacity of allocated GPU memory in bytes.
     * @return Total capacity in bytes.
     */
    [[nodiscard]] std::size_t sizeBytes() const noexcept;

    /**
     * @brief Checks if the GPU buffer is unallocated (elementCount == 0).
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Checks if the buffer holds a valid non-null device pointer.
     * @return True if valid, false otherwise.
     */
    [[nodiscard]] bool isValid() const noexcept;
};

// -----------------------------------------------------------------------------
// Template Method Implementations
// -----------------------------------------------------------------------------

template <typename T>
DeviceBuffer<T>::DeviceBuffer(std::size_t count)
{
    allocate(count);
}

template <typename T>
DeviceBuffer<T>::~DeviceBuffer() noexcept
{
    release();
}

template <typename T>
DeviceBuffer<T>::DeviceBuffer(DeviceBuffer&& other) noexcept
    : devicePtr(other.devicePtr),
      elementCount(other.elementCount)
{
    other.devicePtr = nullptr;
    other.elementCount = 0;
}

template <typename T>
DeviceBuffer<T>& DeviceBuffer<T>::operator=(DeviceBuffer&& other) noexcept
{
    if (this != &other)
    {
        release();

        devicePtr = other.devicePtr;
        elementCount = other.elementCount;

        other.devicePtr = nullptr;
        other.elementCount = 0;
    }

    return *this;
}

template <typename T>
void DeviceBuffer<T>::allocate(std::size_t count)
{
    if (count == 0)
    {
        throw std::invalid_argument("DeviceBuffer::allocate: Allocation count cannot be zero");
    }

    release();

    const std::size_t bytes = count * sizeof(T);
    T* rawPtr = nullptr;

    detail::checkDeviceBufferCudaError(
        cudaMalloc(reinterpret_cast<void**>(&rawPtr), bytes),
        "Failed to allocate GPU memory in DeviceBuffer::allocate"
    );

    devicePtr = rawPtr;
    elementCount = count;
}

template <typename T>
void DeviceBuffer<T>::release() noexcept
{
    if (devicePtr != nullptr)
    {
        cudaFree(devicePtr);
        devicePtr = nullptr;
    }

    elementCount = 0;
}

template <typename T>
void DeviceBuffer<T>::copyFromHost(const T* hostData, std::size_t count)
{
    if (count == 0)
    {
        throw std::invalid_argument("DeviceBuffer::copyFromHost: Copy count cannot be zero");
    }

    if (hostData == nullptr)
    {
        throw std::invalid_argument("DeviceBuffer::copyFromHost: hostData pointer cannot be null");
    }

    if (count > elementCount || devicePtr == nullptr)
    {
        throw std::invalid_argument(
            "DeviceBuffer::copyFromHost: Copy count (" + std::to_string(count) +
            ") exceeds allocated buffer capacity (" + std::to_string(elementCount) + ")"
        );
    }

    detail::checkDeviceBufferCudaError(
        cudaMemcpy(devicePtr, hostData, count * sizeof(T), cudaMemcpyHostToDevice),
        "Failed synchronous cudaMemcpy Host-to-Device in DeviceBuffer::copyFromHost"
    );
}

template <typename T>
void DeviceBuffer<T>::copyToHost(T* hostData, std::size_t count) const
{
    if (count == 0)
    {
        throw std::invalid_argument("DeviceBuffer::copyToHost: Copy count cannot be zero");
    }

    if (hostData == nullptr)
    {
        throw std::invalid_argument("DeviceBuffer::copyToHost: hostData pointer cannot be null");
    }

    if (count > elementCount || devicePtr == nullptr)
    {
        throw std::invalid_argument(
            "DeviceBuffer::copyToHost: Copy count (" + std::to_string(count) +
            ") exceeds allocated buffer capacity (" + std::to_string(elementCount) + ")"
        );
    }

    detail::checkDeviceBufferCudaError(
        cudaMemcpy(hostData, devicePtr, count * sizeof(T), cudaMemcpyDeviceToHost),
        "Failed synchronous cudaMemcpy Device-to-Host in DeviceBuffer::copyToHost"
    );
}

template <typename T>
void DeviceBuffer<T>::zeroMemory()
{
    if (elementCount == 0 || devicePtr == nullptr)
    {
        throw std::runtime_error("DeviceBuffer::zeroMemory: Cannot zero unallocated device buffer");
    }

    detail::checkDeviceBufferCudaError(
        cudaMemset(devicePtr, 0, elementCount * sizeof(T)),
        "Failed cudaMemset in DeviceBuffer::zeroMemory"
    );
}

template <typename T>
T* DeviceBuffer<T>::data() noexcept
{
    return devicePtr;
}

template <typename T>
const T* DeviceBuffer<T>::data() const noexcept
{
    return devicePtr;
}

template <typename T>
std::size_t DeviceBuffer<T>::size() const noexcept
{
    return elementCount;
}

template <typename T>
std::size_t DeviceBuffer<T>::sizeBytes() const noexcept
{
    return elementCount * sizeof(T);
}

template <typename T>
bool DeviceBuffer<T>::empty() const noexcept
{
    return elementCount == 0;
}

template <typename T>
bool DeviceBuffer<T>::isValid() const noexcept
{
    return devicePtr != nullptr && elementCount > 0;
}