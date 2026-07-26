#include "cuda/ClassifiedEventBuffer.h"

#include <algorithm>
#include <string>

ClassifiedEventBuffer::ClassifiedEventBuffer(std::size_t initialCapacity)
{
    allocate(initialCapacity);
}

ClassifiedEventBuffer::ClassifiedEventBuffer(ClassifiedEventBuffer&& other) noexcept
    : addBuf(std::move(other.addBuf)),
      cancelBuf(std::move(other.cancelBuf)),
      modifyBuf(std::move(other.modifyBuf)),
      deleteBuf(std::move(other.deleteBuf)),
      executeVisibleBuf(std::move(other.executeVisibleBuf)),
      executeHiddenBuf(std::move(other.executeHiddenBuf)),
      tradingHaltBuf(std::move(other.tradingHaltBuf)),
      categoryCountsBuf(std::move(other.categoryCountsBuf)),
      hostCounters(other.hostCounters),
      capacityElements(other.capacityElements)
{
    other.hostCounters.clear();
    other.capacityElements = 0;
}

ClassifiedEventBuffer& ClassifiedEventBuffer::operator=(ClassifiedEventBuffer&& other) noexcept
{
    if (this != &other)
    {
        addBuf = std::move(other.addBuf);
        cancelBuf = std::move(other.cancelBuf);
        modifyBuf = std::move(other.modifyBuf);
        deleteBuf = std::move(other.deleteBuf);
        executeVisibleBuf = std::move(other.executeVisibleBuf);
        executeHiddenBuf = std::move(other.executeHiddenBuf);
        tradingHaltBuf = std::move(other.tradingHaltBuf);
        categoryCountsBuf = std::move(other.categoryCountsBuf);

        hostCounters = other.hostCounters;
        capacityElements = other.capacityElements;

        other.hostCounters.clear();
        other.capacityElements = 0;
    }

    return *this;
}

void ClassifiedEventBuffer::allocate(std::size_t capacity)
{
    if (capacity == 0)
    {
        throw std::invalid_argument("ClassifiedEventBuffer::allocate: Allocation capacity cannot be zero");
    }

    addBuf.allocate(capacity);
    cancelBuf.allocate(capacity);
    modifyBuf.allocate(capacity);
    deleteBuf.allocate(capacity);
    executeVisibleBuf.allocate(capacity);
    executeHiddenBuf.allocate(capacity);
    tradingHaltBuf.allocate(capacity);

    categoryCountsBuf.allocate(ClassificationCounters::NUM_CATEGORIES);
    categoryCountsBuf.zeroMemory();

    hostCounters.clear();
    capacityElements = capacity;
}

void ClassifiedEventBuffer::reserve(std::size_t newCapacity)
{
    if (newCapacity == 0)
    {
        throw std::invalid_argument("ClassifiedEventBuffer::reserve: Reserve capacity cannot be zero");
    }

    if (newCapacity <= capacityElements)
    {
        return;
    }

    DeviceBuffer<int> newAdd(newCapacity);
    DeviceBuffer<int> newCancel(newCapacity);
    DeviceBuffer<int> newModify(newCapacity);
    DeviceBuffer<int> newDelete(newCapacity);
    DeviceBuffer<int> newExecuteVisible(newCapacity);
    DeviceBuffer<int> newExecuteHidden(newCapacity);
    DeviceBuffer<int> newTradingHalt(newCapacity);

    auto copyBuf = [](DeviceBuffer<int>& dst, const DeviceBuffer<int>& src, std::size_t count) {
        if (count > 0 && src.isValid())
        {
            cudaError_t err = cudaMemcpy(dst.data(), src.data(), count * sizeof(int), cudaMemcpyDeviceToDevice);
            if (err != cudaSuccess)
            {
                throw std::runtime_error("Failed cudaMemcpy DeviceToDevice in ClassifiedEventBuffer::reserve");
            }
        }
    };

    copyBuf(newAdd, addBuf, hostCounters.getCount(EventType::Add));
    copyBuf(newCancel, cancelBuf, hostCounters.getCount(EventType::Cancel));
    copyBuf(newModify, modifyBuf, hostCounters.getCount(EventType::Modify));
    copyBuf(newDelete, deleteBuf, hostCounters.getCount(EventType::Delete));
    copyBuf(newExecuteVisible, executeVisibleBuf, hostCounters.getCount(EventType::ExecuteVisible));
    copyBuf(newExecuteHidden, executeHiddenBuf, hostCounters.getCount(EventType::ExecuteHidden));
    copyBuf(newTradingHalt, tradingHaltBuf, hostCounters.getCount(EventType::TradingHalt));

    addBuf = std::move(newAdd);
    cancelBuf = std::move(newCancel);
    modifyBuf = std::move(newModify);
    deleteBuf = std::move(newDelete);
    executeVisibleBuf = std::move(newExecuteVisible);
    executeHiddenBuf = std::move(newExecuteHidden);
    tradingHaltBuf = std::move(newTradingHalt);

    capacityElements = newCapacity;
}

void ClassifiedEventBuffer::clear() noexcept
{
    if (categoryCountsBuf.isValid())
    {
        cudaMemset(categoryCountsBuf.data(), 0, ClassificationCounters::NUM_CATEGORIES * sizeof(int));
    }
    hostCounters.clear();
}

void ClassifiedEventBuffer::release() noexcept
{
    addBuf.release();
    cancelBuf.release();
    modifyBuf.release();
    deleteBuf.release();
    executeVisibleBuf.release();
    executeHiddenBuf.release();
    tradingHaltBuf.release();

    categoryCountsBuf.release();
    hostCounters.clear();
    capacityElements = 0;
}

void ClassifiedEventBuffer::updateHostCounters(cudaStream_t stream)
{
    if (!categoryCountsBuf.isValid())
    {
        return;
    }

    int rawCounts[ClassificationCounters::NUM_CATEGORIES] = {0};
    if (stream != nullptr)
    {
        categoryCountsBuf.copyToHostAsync(rawCounts, ClassificationCounters::NUM_CATEGORIES, stream);
        cudaStreamSynchronize(stream);
    }
    else
    {
        categoryCountsBuf.copyToHost(rawCounts, ClassificationCounters::NUM_CATEGORIES);
    }

    for (std::size_t i = 0; i < ClassificationCounters::NUM_CATEGORIES; ++i)
    {
        hostCounters.counts[i] = static_cast<std::size_t>(rawCounts[i]);
    }
}

std::size_t ClassifiedEventBuffer::size() const noexcept
{
    return hostCounters.totalCount();
}

std::size_t ClassifiedEventBuffer::capacity() const noexcept
{
    return capacityElements;
}

bool ClassifiedEventBuffer::empty() const noexcept
{
    return hostCounters.totalCount() == 0;
}

bool ClassifiedEventBuffer::isValid() const noexcept
{
    return addBuf.isValid() &&
           cancelBuf.isValid() &&
           modifyBuf.isValid() &&
           deleteBuf.isValid() &&
           executeVisibleBuf.isValid() &&
           executeHiddenBuf.isValid() &&
           tradingHaltBuf.isValid() &&
           categoryCountsBuf.isValid();
}

std::size_t ClassifiedEventBuffer::getCount(EventType type) const noexcept
{
    return hostCounters.getCount(type);
}

const ClassificationCounters& ClassifiedEventBuffer::getCounters() const noexcept
{
    return hostCounters;
}

int* ClassifiedEventBuffer::getIndices(EventType type) noexcept
{
    switch (type)
    {
        case EventType::Add:            return addBuf.data();
        case EventType::Cancel:         return cancelBuf.data();
        case EventType::Modify:         return modifyBuf.data();
        case EventType::Delete:         return deleteBuf.data();
        case EventType::ExecuteVisible: return executeVisibleBuf.data();
        case EventType::ExecuteHidden:  return executeHiddenBuf.data();
        case EventType::TradingHalt:    return tradingHaltBuf.data();
        default:                        return nullptr;
    }
}

const int* ClassifiedEventBuffer::getIndices(EventType type) const noexcept
{
    switch (type)
    {
        case EventType::Add:            return addBuf.data();
        case EventType::Cancel:         return cancelBuf.data();
        case EventType::Modify:         return modifyBuf.data();
        case EventType::Delete:         return deleteBuf.data();
        case EventType::ExecuteVisible: return executeVisibleBuf.data();
        case EventType::ExecuteHidden:  return executeHiddenBuf.data();
        case EventType::TradingHalt:    return tradingHaltBuf.data();
        default:                        return nullptr;
    }
}

DeviceBuffer<int>& ClassifiedEventBuffer::getIndexBuffer(EventType type)
{
    switch (type)
    {
        case EventType::Add:            return addBuf;
        case EventType::Cancel:         return cancelBuf;
        case EventType::Modify:         return modifyBuf;
        case EventType::Delete:         return deleteBuf;
        case EventType::ExecuteVisible: return executeVisibleBuf;
        case EventType::ExecuteHidden:  return executeHiddenBuf;
        case EventType::TradingHalt:    return tradingHaltBuf;
        default: throw std::invalid_argument("ClassifiedEventBuffer::getIndexBuffer: Invalid EventType");
    }
}

const DeviceBuffer<int>& ClassifiedEventBuffer::getIndexBuffer(EventType type) const
{
    switch (type)
    {
        case EventType::Add:            return addBuf;
        case EventType::Cancel:         return cancelBuf;
        case EventType::Modify:         return modifyBuf;
        case EventType::Delete:         return deleteBuf;
        case EventType::ExecuteVisible: return executeVisibleBuf;
        case EventType::ExecuteHidden:  return executeHiddenBuf;
        case EventType::TradingHalt:    return tradingHaltBuf;
        default: throw std::invalid_argument("ClassifiedEventBuffer::getIndexBuffer: Invalid EventType");
    }
}

ClassifiedEventSoAView ClassifiedEventBuffer::getDeviceView() noexcept
{
    ClassifiedEventSoAView view;
    view.addIndices = addBuf.data();
    view.cancelIndices = cancelBuf.data();
    view.modifyIndices = modifyBuf.data();
    view.deleteIndices = deleteBuf.data();
    view.executeVisibleIndices = executeVisibleBuf.data();
    view.executeHiddenIndices = executeHiddenBuf.data();
    view.tradingHaltIndices = tradingHaltBuf.data();
    view.categoryCounts = categoryCountsBuf.data();
    return view;
}
