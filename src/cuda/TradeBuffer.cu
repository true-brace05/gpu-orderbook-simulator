#include "cuda/TradeBuffer.h"

#include <algorithm>
#include <stdexcept>
#include <string>

TradeBuffer::TradeBuffer(std::size_t initialCapacity)
{
    allocate(initialCapacity);
}

TradeBuffer::TradeBuffer(TradeBuffer&& other) noexcept
    : tradesBuf(std::move(other.tradesBuf)),
      tradeCountBuf(std::move(other.tradeCountBuf)),
      activeTradeCount(other.activeTradeCount),
      capacityElements(other.capacityElements)
{
    other.activeTradeCount = 0;
    other.capacityElements = 0;
}

TradeBuffer& TradeBuffer::operator=(TradeBuffer&& other) noexcept
{
    if (this != &other)
    {
        tradesBuf = std::move(other.tradesBuf);
        tradeCountBuf = std::move(other.tradeCountBuf);
        activeTradeCount = other.activeTradeCount;
        capacityElements = other.capacityElements;

        other.activeTradeCount = 0;
        other.capacityElements = 0;
    }

    return *this;
}

void TradeBuffer::allocate(std::size_t capacity)
{
    if (capacity == 0)
    {
        throw std::invalid_argument("TradeBuffer::allocate: Allocation capacity cannot be zero");
    }

    tradesBuf.allocate(capacity);
    tradeCountBuf.allocate(1);
    tradeCountBuf.zeroMemory();

    activeTradeCount = 0;
    capacityElements = capacity;
}

void TradeBuffer::reserve(std::size_t newCapacity)
{
    if (newCapacity == 0)
    {
        throw std::invalid_argument("TradeBuffer::reserve: Reserve capacity cannot be zero");
    }

    if (newCapacity <= capacityElements)
    {
        return;
    }

    DeviceBuffer<TradeRecord> newTrades(newCapacity);
    DeviceBuffer<int> newCountBuf(1);
    newCountBuf.zeroMemory();

    if (activeTradeCount > 0 && tradesBuf.isValid())
    {
        cudaError_t err = cudaMemcpy(
            newTrades.data(),
            tradesBuf.data(),
            activeTradeCount * sizeof(TradeRecord),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error("Failed cudaMemcpy DeviceToDevice for trades in TradeBuffer::reserve");
        }
    }

    if (tradeCountBuf.isValid())
    {
        cudaError_t err = cudaMemcpy(
            newCountBuf.data(),
            tradeCountBuf.data(),
            sizeof(int),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error("Failed cudaMemcpy DeviceToDevice for tradeCount in TradeBuffer::reserve");
        }
    }

    tradesBuf = std::move(newTrades);
    tradeCountBuf = std::move(newCountBuf);
    capacityElements = newCapacity;
}

void TradeBuffer::resize(std::size_t newSize)
{
    if (newSize > capacityElements)
    {
        reserve(newSize);
    }
    activeTradeCount = newSize;
}

void TradeBuffer::clear() noexcept
{
    if (tradeCountBuf.isValid())
    {
        cudaMemset(tradeCountBuf.data(), 0, sizeof(int));
    }
    activeTradeCount = 0;
}

void TradeBuffer::release() noexcept
{
    tradesBuf.release();
    tradeCountBuf.release();
    activeTradeCount = 0;
    capacityElements = 0;
}

void TradeBuffer::updateHostCount(cudaStream_t stream)
{
    if (!tradeCountBuf.isValid())
    {
        activeTradeCount = 0;
        return;
    }

    int rawCount = 0;
    if (stream != nullptr)
    {
        tradeCountBuf.copyToHostAsync(&rawCount, 1, stream);
        cudaStreamSynchronize(stream);
    }
    else
    {
        tradeCountBuf.copyToHost(&rawCount, 1);
    }

    activeTradeCount = static_cast<std::size_t>(rawCount);
}

std::size_t TradeBuffer::size() const noexcept
{
    return activeTradeCount;
}

std::size_t TradeBuffer::capacity() const noexcept
{
    return capacityElements;
}

bool TradeBuffer::empty() const noexcept
{
    return activeTradeCount == 0;
}

bool TradeBuffer::isValid() const noexcept
{
    return tradesBuf.isValid() && tradeCountBuf.isValid();
}
