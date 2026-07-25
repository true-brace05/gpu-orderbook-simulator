#pragma once

#include <cuda_runtime.h>

/**
 * @brief RAII wrapper for managing a CUDA device context, non-blocking execution stream,
 *        and timing events.
 *
 * CUDAContext enforces non-copyable, move-only semantics to guarantee single ownership
 * of CUDA stream and event handles. All resources are created upon construction and
 * safely destroyed in the noexcept destructor.
 *
 * TODO: Multi-GPU support may require restoring the active CUDA device (cudaSetDevice)
 *       prior to stream and event operations.
 */
class CUDAContext
{
private:
    int deviceId = 0;
    cudaStream_t stream = nullptr;
    cudaEvent_t startEvent = nullptr;
    cudaEvent_t stopEvent = nullptr;
    bool initialized = false;

    void cleanup() noexcept;

public:
    /**
     * @brief Constructs a CUDAContext bound to the specified GPU device.
     * 
     * Sets the active CUDA device, creates a non-blocking CUDA stream, and initializes
     * timing events. Throws std::runtime_error if device selection or resource allocation fails.
     *
     * @param deviceId ID of the target CUDA device (defaults to 0).
     */
    explicit CUDAContext(int deviceId = 0);

    /**
     * @brief Destructor. Releases all allocated CUDA events and stream resources.
     * Guarantees noexcept and will never throw exceptions.
     */
    ~CUDAContext() noexcept;

    // Non-copyable
    CUDAContext(const CUDAContext&) = delete;
    CUDAContext& operator=(const CUDAContext&) = delete;

    // Movable
    CUDAContext(CUDAContext&& other) noexcept;
    CUDAContext& operator=(CUDAContext&& other) noexcept;

    /**
     * @brief Synchronizes the owned CUDA stream, blocking host execution until all issued
     * commands on the stream have completed.
     */
    void synchronize() const;

    /**
     * @brief Records the start timing event onto the owned CUDA stream.
     */
    void startTimer() const;

    /**
     * @brief Records the stop timing event onto the owned CUDA stream.
     */
    void stopTimer() const;

    /**
     * @brief Synchronizes the stop event and calculates elapsed time in milliseconds between
     * startTimer() and stopTimer().
     *
     * @return Elapsed time in milliseconds.
     */
    [[nodiscard]] float elapsedMilliseconds() const;

    /**
     * @brief Gets the underlying raw CUDA stream handle.
     * @return Raw cudaStream_t handle or nullptr if uninitialized.
     */
    [[nodiscard]] cudaStream_t getStream() const noexcept;

    /**
     * @brief Gets the active CUDA device ID.
     * @return Device ID.
     */
    [[nodiscard]] int getDeviceId() const noexcept;

    /**
     * @brief Checks if the context owns valid CUDA stream and event handles.
     * @return True if initialized and valid, false otherwise.
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief Checks if CUDA runtime is operational and at least one CUDA device is available.
     * @return True if CUDA is available, false otherwise.
     */
    [[nodiscard]] static bool isAvailable() noexcept;

    /**
     * @brief Returns the total number of CUDA-capable devices present in the system.
     * @return Device count.
     */
    [[nodiscard]] static int getDeviceCount() noexcept;

    /**
     * @brief Queries and prints detailed GPU hardware capabilities to stdout.
     */
    static void printDeviceInfo();
};