#include "cuda/CUDAContext.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    /**
     * @brief Reusable helper function for evaluating CUDA runtime API return codes.
     * Throws std::runtime_error with formatted message and CUDA error string on failure.
     */
    inline void checkCudaError(cudaError_t result, const std::string& message)
    {
        if (result != cudaSuccess)
        {
            throw std::runtime_error(message + ": " + cudaGetErrorString(result));
        }
    }
} // anonymous namespace

CUDAContext::CUDAContext(int deviceId)
    : deviceId(deviceId)
{
    try
    {
        // TODO: In multi-GPU configurations, ensure cudaSetDevice(deviceId) is invoked
        // prior to stream/event operations if the active thread context has changed.
        checkCudaError(
            cudaSetDevice(deviceId),
            "Failed to set CUDA device " + std::to_string(deviceId)
        );

        checkCudaError(
            cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking),
            "Failed to create non-blocking CUDA stream"
        );

        checkCudaError(
            cudaEventCreate(&startEvent),
            "Failed to create start CUDA event"
        );

        checkCudaError(
            cudaEventCreate(&stopEvent),
            "Failed to create stop CUDA event"
        );

        initialized = true;
    }
    catch (...)
    {
        cleanup();
        throw;
    }
}

CUDAContext::~CUDAContext() noexcept
{
    cleanup();
}

void CUDAContext::cleanup() noexcept
{
    if (stopEvent != nullptr)
    {
        cudaEventDestroy(stopEvent);
        stopEvent = nullptr;
    }

    if (startEvent != nullptr)
    {
        cudaEventDestroy(startEvent);
        startEvent = nullptr;
    }

    if (stream != nullptr)
    {
        cudaStreamDestroy(stream);
        stream = nullptr;
    }

    initialized = false;
}

CUDAContext::CUDAContext(CUDAContext&& other) noexcept
    : deviceId(other.deviceId),
      stream(other.stream),
      startEvent(other.startEvent),
      stopEvent(other.stopEvent),
      initialized(other.initialized)
{
    other.deviceId = 0;
    other.stream = nullptr;
    other.startEvent = nullptr;
    other.stopEvent = nullptr;
    other.initialized = false;
}

CUDAContext& CUDAContext::operator=(CUDAContext&& other) noexcept
{
    if (this != &other)
    {
        cleanup();

        deviceId = other.deviceId;
        stream = other.stream;
        startEvent = other.startEvent;
        stopEvent = other.stopEvent;
        initialized = other.initialized;

        other.deviceId = 0;
        other.stream = nullptr;
        other.startEvent = nullptr;
        other.stopEvent = nullptr;
        other.initialized = false;
    }

    return *this;
}

void CUDAContext::synchronize() const
{
    if (!initialized || stream == nullptr)
    {
        throw std::runtime_error("CUDAContext is not initialized");
    }

    checkCudaError(
        cudaStreamSynchronize(stream),
        "Failed to synchronize CUDA stream"
    );
}

void CUDAContext::startTimer() const
{
    if (!initialized || startEvent == nullptr || stream == nullptr)
    {
        throw std::runtime_error("CUDAContext is not initialized");
    }

    checkCudaError(
        cudaEventRecord(startEvent, stream),
        "Failed to record start event"
    );
}

void CUDAContext::stopTimer() const
{
    if (!initialized || stopEvent == nullptr || stream == nullptr)
    {
        throw std::runtime_error("CUDAContext is not initialized");
    }

    checkCudaError(
        cudaEventRecord(stopEvent, stream),
        "Failed to record stop event"
    );
}

float CUDAContext::elapsedMilliseconds() const
{
    if (!initialized || startEvent == nullptr || stopEvent == nullptr)
    {
        throw std::runtime_error("CUDAContext is not initialized");
    }

    checkCudaError(
        cudaEventSynchronize(stopEvent),
        "Failed to synchronize stop event"
    );

    float ms = 0.0f;
    checkCudaError(
        cudaEventElapsedTime(&ms, startEvent, stopEvent),
        "Failed to compute elapsed time between CUDA events"
    );

    return ms;
}

cudaStream_t CUDAContext::getStream() const noexcept
{
    return stream;
}

int CUDAContext::getDeviceId() const noexcept
{
    return deviceId;
}

bool CUDAContext::isValid() const noexcept
{
    return initialized;
}

bool CUDAContext::isAvailable() noexcept
{
    int deviceCount = 0;
    cudaError_t result = cudaGetDeviceCount(&deviceCount);
    return (result == cudaSuccess && deviceCount > 0);
}

int CUDAContext::getDeviceCount() noexcept
{
    int deviceCount = 0;
    cudaError_t result = cudaGetDeviceCount(&deviceCount);
    if (result != cudaSuccess)
    {
        return 0;
    }
    return deviceCount;
}

void CUDAContext::printDeviceInfo()
{
    int count = getDeviceCount();

    std::cout << "CUDA Devices Available: " << count << '\n';

    for (int i = 0; i < count; ++i)
    {
        cudaDeviceProp prop{};
        cudaError_t result = cudaGetDeviceProperties(&prop, i);

        if (result != cudaSuccess)
        {
            std::cout << "  GPU " << i << ": Error reading device properties ("
                      << cudaGetErrorString(result) << ")\n";
            continue;
        }

        std::cout << "  GPU " << i << ": " << prop.name << '\n'
                  << "    Compute Capability : " << prop.major << '.' << prop.minor << '\n'
                  << "    Global Memory      : " << (prop.totalGlobalMem / (1024 * 1024)) << " MB\n"
                  << "    Multiprocessors    : " << prop.multiProcessorCount << '\n'
                  << "    Warp Size          : " << prop.warpSize << '\n';
    }
}