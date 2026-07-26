#include "cuda/ReplayBuffer.h"

#include <algorithm>
#include <string>

ReplayBuffer::ReplayBuffer(std::size_t initialCapacity)
    : deviceBuffer(initialCapacity), elementCount(0)
{
}

void ReplayBuffer::allocate(std::size_t capacity)
{
    if (capacity == 0)
    {
        throw std::invalid_argument("ReplayBuffer::allocate: Allocation capacity cannot be zero");
    }

    deviceBuffer.allocate(capacity);
    elementCount = 0;
}

void ReplayBuffer::reserve(std::size_t newCapacity)
{
    if (newCapacity == 0)
    {
        throw std::invalid_argument("ReplayBuffer::reserve: Reserve capacity cannot be zero");
    }

    if (newCapacity <= deviceBuffer.size())
    {
        return;
    }

    DeviceBuffer<Event> newBuffer(newCapacity);

    if (elementCount > 0 && deviceBuffer.isValid())
    {
        const std::size_t copyCount = std::min(elementCount, newCapacity);
        cudaError_t err = cudaMemcpy(
            newBuffer.data(),
            deviceBuffer.data(),
            copyCount * sizeof(Event),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error(
                std::string("Failed cudaMemcpy DeviceToDevice in ReplayBuffer::reserve: ") +
                cudaGetErrorString(err)
            );
        }
    }

    deviceBuffer = std::move(newBuffer);
    elementCount = std::min(elementCount, newCapacity);
}

void ReplayBuffer::resize(std::size_t newSize)
{
    if (newSize == 0)
    {
        throw std::invalid_argument("ReplayBuffer::resize: Target size cannot be zero");
    }

    if (newSize > capacity())
    {
        reserve(newSize);
    }

    elementCount = newSize;
}

void ReplayBuffer::uploadBatch(const Event* hostEvents, std::size_t batchCount, cudaStream_t stream)
{
    if (hostEvents == nullptr)
    {
        throw std::invalid_argument("ReplayBuffer::uploadBatch: hostEvents pointer cannot be null");
    }

    if (batchCount == 0)
    {
        throw std::invalid_argument("ReplayBuffer::uploadBatch: batchCount cannot be zero");
    }

    if (batchCount > capacity())
    {
        reserve(batchCount);
    }

    deviceBuffer.copyFromHostAsync(hostEvents, batchCount, stream);
    elementCount = batchCount;
}

void ReplayBuffer::appendBatch(const Event* hostEvents, std::size_t batchCount, cudaStream_t stream)
{
    if (hostEvents == nullptr)
    {
        throw std::invalid_argument("ReplayBuffer::appendBatch: hostEvents pointer cannot be null");
    }

    if (batchCount == 0)
    {
        throw std::invalid_argument("ReplayBuffer::appendBatch: batchCount cannot be zero");
    }

    const std::size_t requiredCapacity = elementCount + batchCount;
    if (requiredCapacity > capacity())
    {
        reserve(requiredCapacity);
    }

    Event* targetPtr = deviceBuffer.data() + elementCount;
    cudaError_t err = cudaMemcpyAsync(
        targetPtr,
        hostEvents,
        batchCount * sizeof(Event),
        cudaMemcpyHostToDevice,
        stream
    );
    if (err != cudaSuccess)
    {
        throw std::runtime_error(
            std::string("Failed cudaMemcpyAsync HostToDevice in ReplayBuffer::appendBatch: ") +
            cudaGetErrorString(err)
        );
    }

    elementCount += batchCount;
}

void ReplayBuffer::downloadBatch(Event* hostEvents, std::size_t count, cudaStream_t stream) const
{
    if (hostEvents == nullptr)
    {
        throw std::invalid_argument("ReplayBuffer::downloadBatch: hostEvents pointer cannot be null");
    }

    if (count == 0)
    {
        throw std::invalid_argument("ReplayBuffer::downloadBatch: count cannot be zero");
    }

    if (count > elementCount || !deviceBuffer.isValid())
    {
        throw std::invalid_argument(
            "ReplayBuffer::downloadBatch: Download count (" + std::to_string(count) +
            ") exceeds logical batch size (" + std::to_string(elementCount) + ")"
        );
    }

    deviceBuffer.copyToHostAsync(hostEvents, count, stream);
}

void ReplayBuffer::clear() noexcept
{
    elementCount = 0;
}

void ReplayBuffer::release() noexcept
{
    deviceBuffer.release();
    elementCount = 0;
}

std::size_t ReplayBuffer::size() const noexcept
{
    return elementCount;
}

std::size_t ReplayBuffer::capacity() const noexcept
{
    return deviceBuffer.size();
}

std::size_t ReplayBuffer::sizeBytes() const noexcept
{
    return elementCount * sizeof(Event);
}

std::size_t ReplayBuffer::capacityBytes() const noexcept
{
    return deviceBuffer.sizeBytes();
}

bool ReplayBuffer::empty() const noexcept
{
    return elementCount == 0;
}

Event* ReplayBuffer::data() noexcept
{
    return deviceBuffer.data();
}

const Event* ReplayBuffer::data() const noexcept
{
    return deviceBuffer.data();
}

bool ReplayBuffer::isValid() const noexcept
{
    return deviceBuffer.isValid();
}

const DeviceBuffer<Event>& ReplayBuffer::getDeviceBuffer() const noexcept
{
    return deviceBuffer;
}
