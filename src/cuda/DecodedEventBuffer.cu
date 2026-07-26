#include "cuda/DecodedEventBuffer.h"

#include <algorithm>
#include <string>

DecodedEventBuffer::DecodedEventBuffer(std::size_t initialCapacity)
    : timestampsBuf(initialCapacity),
      eventTypesBuf(initialCapacity),
      orderIdsBuf(initialCapacity),
      sidesBuf(initialCapacity),
      orderTypesBuf(initialCapacity),
      pricesBuf(initialCapacity),
      quantitiesBuf(initialCapacity),
      displayQuantitiesBuf(initialCapacity),
      reserveQuantitiesBuf(initialCapacity),
      elementCount(0)
{
}

DecodedEventBuffer::DecodedEventBuffer(DecodedEventBuffer&& other) noexcept
    : timestampsBuf(std::move(other.timestampsBuf)),
      eventTypesBuf(std::move(other.eventTypesBuf)),
      orderIdsBuf(std::move(other.orderIdsBuf)),
      sidesBuf(std::move(other.sidesBuf)),
      orderTypesBuf(std::move(other.orderTypesBuf)),
      pricesBuf(std::move(other.pricesBuf)),
      quantitiesBuf(std::move(other.quantitiesBuf)),
      displayQuantitiesBuf(std::move(other.displayQuantitiesBuf)),
      reserveQuantitiesBuf(std::move(other.reserveQuantitiesBuf)),
      elementCount(other.elementCount)
{
    other.elementCount = 0;
}

DecodedEventBuffer& DecodedEventBuffer::operator=(DecodedEventBuffer&& other) noexcept
{
    if (this != &other)
    {
        timestampsBuf = std::move(other.timestampsBuf);
        eventTypesBuf = std::move(other.eventTypesBuf);
        orderIdsBuf = std::move(other.orderIdsBuf);
        sidesBuf = std::move(other.sidesBuf);
        orderTypesBuf = std::move(other.orderTypesBuf);
        pricesBuf = std::move(other.pricesBuf);
        quantitiesBuf = std::move(other.quantitiesBuf);
        displayQuantitiesBuf = std::move(other.displayQuantitiesBuf);
        reserveQuantitiesBuf = std::move(other.reserveQuantitiesBuf);

        elementCount = other.elementCount;
        other.elementCount = 0;
    }

    return *this;
}

void DecodedEventBuffer::allocate(std::size_t capacity)
{
    if (capacity == 0)
    {
        throw std::invalid_argument("DecodedEventBuffer::allocate: Allocation capacity cannot be zero");
    }

    timestampsBuf.allocate(capacity);
    eventTypesBuf.allocate(capacity);
    orderIdsBuf.allocate(capacity);
    sidesBuf.allocate(capacity);
    orderTypesBuf.allocate(capacity);
    pricesBuf.allocate(capacity);
    quantitiesBuf.allocate(capacity);
    displayQuantitiesBuf.allocate(capacity);
    reserveQuantitiesBuf.allocate(capacity);

    elementCount = 0;
}

template <typename T>
static void copyDeviceBufferData(DeviceBuffer<T>& dst, const DeviceBuffer<T>& src, std::size_t count)
{
    if (count > 0 && src.isValid())
    {
        cudaError_t err = cudaMemcpy(
            dst.data(),
            src.data(),
            count * sizeof(T),
            cudaMemcpyDeviceToDevice
        );
        if (err != cudaSuccess)
        {
            throw std::runtime_error(
                std::string("Failed cudaMemcpy DeviceToDevice in DecodedEventBuffer::reserve: ") +
                cudaGetErrorString(err)
            );
        }
    }
}

void DecodedEventBuffer::reserve(std::size_t newCapacity)
{
    if (newCapacity == 0)
    {
        throw std::invalid_argument("DecodedEventBuffer::reserve: Reserve capacity cannot be zero");
    }

    if (newCapacity <= capacity())
    {
        return;
    }

    DeviceBuffer<uint64_t> newTimestamps(newCapacity);
    DeviceBuffer<uint8_t> newEventTypes(newCapacity);
    DeviceBuffer<int> newOrderIds(newCapacity);
    DeviceBuffer<uint8_t> newSides(newCapacity);
    DeviceBuffer<uint8_t> newOrderTypes(newCapacity);
    DeviceBuffer<double> newPrices(newCapacity);
    DeviceBuffer<int> newQuantities(newCapacity);
    DeviceBuffer<int> newDisplayQuantities(newCapacity);
    DeviceBuffer<int> newReserveQuantities(newCapacity);

    const std::size_t copyCount = std::min(elementCount, newCapacity);

    copyDeviceBufferData(newTimestamps, timestampsBuf, copyCount);
    copyDeviceBufferData(newEventTypes, eventTypesBuf, copyCount);
    copyDeviceBufferData(newOrderIds, orderIdsBuf, copyCount);
    copyDeviceBufferData(newSides, sidesBuf, copyCount);
    copyDeviceBufferData(newOrderTypes, orderTypesBuf, copyCount);
    copyDeviceBufferData(newPrices, pricesBuf, copyCount);
    copyDeviceBufferData(newQuantities, quantitiesBuf, copyCount);
    copyDeviceBufferData(newDisplayQuantities, displayQuantitiesBuf, copyCount);
    copyDeviceBufferData(newReserveQuantities, reserveQuantitiesBuf, copyCount);

    timestampsBuf = std::move(newTimestamps);
    eventTypesBuf = std::move(newEventTypes);
    orderIdsBuf = std::move(newOrderIds);
    sidesBuf = std::move(newSides);
    orderTypesBuf = std::move(newOrderTypes);
    pricesBuf = std::move(newPrices);
    quantitiesBuf = std::move(newQuantities);
    displayQuantitiesBuf = std::move(newDisplayQuantities);
    reserveQuantitiesBuf = std::move(newReserveQuantities);

    elementCount = copyCount;
}

void DecodedEventBuffer::resize(std::size_t newSize)
{
    if (newSize > capacity())
    {
        reserve(newSize);
    }

    elementCount = newSize;
}

void DecodedEventBuffer::clear() noexcept
{
    elementCount = 0;
}

void DecodedEventBuffer::release() noexcept
{
    timestampsBuf.release();
    eventTypesBuf.release();
    orderIdsBuf.release();
    sidesBuf.release();
    orderTypesBuf.release();
    pricesBuf.release();
    quantitiesBuf.release();
    displayQuantitiesBuf.release();
    reserveQuantitiesBuf.release();

    elementCount = 0;
}

std::size_t DecodedEventBuffer::size() const noexcept
{
    return elementCount;
}

std::size_t DecodedEventBuffer::capacity() const noexcept
{
    return timestampsBuf.size();
}

bool DecodedEventBuffer::empty() const noexcept
{
    return elementCount == 0;
}

bool DecodedEventBuffer::isValid() const noexcept
{
    return timestampsBuf.isValid() &&
           eventTypesBuf.isValid() &&
           orderIdsBuf.isValid() &&
           sidesBuf.isValid() &&
           orderTypesBuf.isValid() &&
           pricesBuf.isValid() &&
           quantitiesBuf.isValid() &&
           displayQuantitiesBuf.isValid() &&
           reserveQuantitiesBuf.isValid();
}

std::size_t DecodedEventBuffer::sizeBytes() const noexcept
{
    constexpr std::size_t eventSizeSoA =
        sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int) +
        sizeof(uint8_t)  + sizeof(uint8_t) + sizeof(double) +
        sizeof(int)      + sizeof(int)      + sizeof(int);

    return elementCount * eventSizeSoA;
}

std::size_t DecodedEventBuffer::capacityBytes() const noexcept
{
    constexpr std::size_t eventSizeSoA =
        sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int) +
        sizeof(uint8_t)  + sizeof(uint8_t) + sizeof(double) +
        sizeof(int)      + sizeof(int)      + sizeof(int);

    return capacity() * eventSizeSoA;
}

DecodedEventSoAView DecodedEventBuffer::getDeviceView() noexcept
{
    DecodedEventSoAView view;
    view.timestamps = timestamps();
    view.eventTypes = eventTypes();
    view.orderIds = orderIds();
    view.sides = sides();
    view.orderTypes = orderTypes();
    view.prices = prices();
    view.quantities = quantities();
    view.displayQuantities = displayQuantities();
    view.reserveQuantities = reserveQuantities();
    return view;
}

ConstDecodedEventSoAView DecodedEventBuffer::getDeviceView() const noexcept
{
    ConstDecodedEventSoAView view;
    view.timestamps = timestamps();
    view.eventTypes = eventTypes();
    view.orderIds = orderIds();
    view.sides = sides();
    view.orderTypes = orderTypes();
    view.prices = prices();
    view.quantities = quantities();
    view.displayQuantities = displayQuantities();
    view.reserveQuantities = reserveQuantities();
    return view;
}
