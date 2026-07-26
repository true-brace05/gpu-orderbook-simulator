#include "cuda/PriceLevelBuffer.h"

#include <algorithm>
#include <stdexcept>
#include <string>

PriceLevelBuffer::PriceLevelBuffer(std::size_t initialCapacity)
{
    allocate(initialCapacity);
}

PriceLevelBuffer::PriceLevelBuffer(PriceLevelBuffer&& other) noexcept
    : levelsBuf(std::move(other.levelsBuf)),
      orderIndicesBuf(std::move(other.orderIndicesBuf)),
      levelCountBuf(std::move(other.levelCountBuf)),
      activeLevelCount(other.activeLevelCount),
      activeOrderCount(other.activeOrderCount)
{
    other.activeLevelCount = 0;
    other.activeOrderCount = 0;
}

PriceLevelBuffer& PriceLevelBuffer::operator=(PriceLevelBuffer&& other) noexcept
{
    if (this != &other)
    {
        levelsBuf = std::move(other.levelsBuf);
        orderIndicesBuf = std::move(other.orderIndicesBuf);
        levelCountBuf = std::move(other.levelCountBuf);
        activeLevelCount = other.activeLevelCount;
        activeOrderCount = other.activeOrderCount;

        other.activeLevelCount = 0;
        other.activeOrderCount = 0;
    }

    return *this;
}

void PriceLevelBuffer::allocate(std::size_t capacity)
{
    if (capacity == 0)
    {
        throw std::invalid_argument("PriceLevelBuffer::allocate: Capacity cannot be zero");
    }

    levelsBuf.allocate(capacity);
    orderIndicesBuf.allocate(capacity);

    levelCountBuf.allocate(1);
    levelCountBuf.zeroMemory();

    activeLevelCount = 0;
    activeOrderCount = 0;
}

void PriceLevelBuffer::reserve(std::size_t newCapacity)
{
    if (newCapacity == 0)
    {
        throw std::invalid_argument("PriceLevelBuffer::reserve: Reserve capacity cannot be zero");
    }

    if (newCapacity <= capacity())
    {
        return;
    }

    DeviceBuffer<PriceLevel> newLevels(newCapacity);
    DeviceBuffer<int> newOrderIndices(newCapacity);
    DeviceBuffer<int> newCountBuf(1);
    newCountBuf.zeroMemory();

    if (activeLevelCount > 0 && levelsBuf.isValid())
    {
        cudaError_t err = cudaMemcpy(
            newLevels.data(),
            levelsBuf.data(),
            activeLevelCount * sizeof(PriceLevel),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error("Failed cudaMemcpy DeviceToDevice for levels in PriceLevelBuffer::reserve");
        }
    }

    if (activeOrderCount > 0 && orderIndicesBuf.isValid())
    {
        cudaError_t err = cudaMemcpy(
            newOrderIndices.data(),
            orderIndicesBuf.data(),
            activeOrderCount * sizeof(int),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error("Failed cudaMemcpy DeviceToDevice for order indices in PriceLevelBuffer::reserve");
        }
    }

    if (levelCountBuf.isValid())
    {
        cudaError_t err = cudaMemcpy(
            newCountBuf.data(),
            levelCountBuf.data(),
            sizeof(int),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error("Failed cudaMemcpy DeviceToDevice for levelCount in PriceLevelBuffer::reserve");
        }
    }

    levelsBuf = std::move(newLevels);
    orderIndicesBuf = std::move(newOrderIndices);
    levelCountBuf = std::move(newCountBuf);
}

void PriceLevelBuffer::resize(std::size_t newSize)
{
    if (newSize > capacity())
    {
        reserve(newSize);
    }
    activeLevelCount = newSize;
}

void PriceLevelBuffer::clear() noexcept
{
    if (levelCountBuf.isValid())
    {
        cudaMemset(levelCountBuf.data(), 0, sizeof(int));
    }
    activeLevelCount = 0;
    activeOrderCount = 0;
}

void PriceLevelBuffer::release() noexcept
{
    levelsBuf.release();
    orderIndicesBuf.release();
    levelCountBuf.release();
    activeLevelCount = 0;
    activeOrderCount = 0;
}

void PriceLevelBuffer::updateHostCount(cudaStream_t stream)
{
    if (!levelCountBuf.isValid())
    {
        activeLevelCount = 0;
        return;
    }

    int rawCount = 0;
    if (stream != nullptr)
    {
        levelCountBuf.copyToHostAsync(&rawCount, 1, stream);
        cudaStreamSynchronize(stream);
    }
    else
    {
        levelCountBuf.copyToHost(&rawCount, 1);
    }

    activeLevelCount = static_cast<std::size_t>(rawCount);
}

std::size_t PriceLevelBuffer::size() const noexcept
{
    return activeLevelCount;
}

std::size_t PriceLevelBuffer::orderCount() const noexcept
{
    return activeOrderCount;
}

std::size_t PriceLevelBuffer::capacity() const noexcept
{
    return levelsBuf.capacity();
}

bool PriceLevelBuffer::empty() const noexcept
{
    return activeLevelCount == 0;
}

bool PriceLevelBuffer::isValid() const noexcept
{
    return levelsBuf.isValid() && orderIndicesBuf.isValid() && levelCountBuf.isValid();
}
