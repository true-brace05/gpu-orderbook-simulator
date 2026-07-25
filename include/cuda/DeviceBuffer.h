#pragma once

#include <cstddef>

template <typename T>
class DeviceBuffer
{
public:
    DeviceBuffer();

    explicit DeviceBuffer(std::size_t size);

    ~DeviceBuffer();

    void allocate(std::size_t size);

    void release();

    void copyFromHost(const T* hostData, std::size_t count);

    void copyToHost(T* hostData, std::size_t count) const;

    T* data();

    const T* data() const;

    std::size_t size() const;

private:
    T* devicePtr = nullptr;

    std::size_t elementCount = 0;
};