#pragma once

#include "cuda/DeviceBuffer.h"
#include "replay/Event.h"
#include "Types.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

/**
 * @brief Raw GPU device pointer view for writing/reading DecodedEventBuffer attributes in CUDA kernels.
 */
struct DecodedEventSoAView
{
    uint64_t* timestamps = nullptr;
    uint8_t* eventTypes = nullptr;
    int* orderIds = nullptr;
    uint8_t* sides = nullptr;
    uint8_t* orderTypes = nullptr;
    double* prices = nullptr;
    int* quantities = nullptr;
    int* displayQuantities = nullptr;
    int* reserveQuantities = nullptr;
};

/**
 * @brief Const raw GPU device pointer view for read-only access in CUDA kernels.
 */
struct ConstDecodedEventSoAView
{
    const uint64_t* timestamps = nullptr;
    const uint8_t* eventTypes = nullptr;
    const int* orderIds = nullptr;
    const uint8_t* sides = nullptr;
    const uint8_t* orderTypes = nullptr;
    const double* prices = nullptr;
    const int* quantities = nullptr;
    const int* displayQuantities = nullptr;
    const int* reserveQuantities = nullptr;
};

/**
 * @brief Structure of Arrays (SoA) GPU device memory container for decoded order book events.
 *
 * Reuses DeviceBuffer<T> for each event attribute array. Enforces move-only RAII semantics,
 * std::vector capacity management (reserve, resize, clear, release), and provides direct raw
 * pointer views for CUDA kernel launches.
 */
class DecodedEventBuffer
{
private:
    DeviceBuffer<uint64_t> timestampsBuf;
    DeviceBuffer<uint8_t> eventTypesBuf;
    DeviceBuffer<int> orderIdsBuf;
    DeviceBuffer<uint8_t> sidesBuf;
    DeviceBuffer<uint8_t> orderTypesBuf;
    DeviceBuffer<double> pricesBuf;
    DeviceBuffer<int> quantitiesBuf;
    DeviceBuffer<int> displayQuantitiesBuf;
    DeviceBuffer<int> reserveQuantitiesBuf;

    std::size_t elementCount = 0;

public:
    /**
     * @brief Constructs an empty DecodedEventBuffer with zero allocated GPU capacity.
     */
    DecodedEventBuffer() noexcept = default;

    /**
     * @brief Constructs a DecodedEventBuffer and allocates GPU memory capacity for initialCapacity elements.
     * Throws std::invalid_argument if initialCapacity == 0.
     * Throws std::runtime_error if allocation fails.
     *
     * @param initialCapacity Number of decoded event elements to reserve on GPU.
     */
    explicit DecodedEventBuffer(std::size_t initialCapacity);

    /**
     * @brief Destructor. Automatically releases all GPU allocations via DeviceBuffer<T> RAII.
     */
    ~DecodedEventBuffer() noexcept = default;

    // Non-copyable
    DecodedEventBuffer(const DecodedEventBuffer&) = delete;
    DecodedEventBuffer& operator=(const DecodedEventBuffer&) = delete;

    // Movable
    DecodedEventBuffer(DecodedEventBuffer&& other) noexcept;
    DecodedEventBuffer& operator=(DecodedEventBuffer&& other) noexcept;

    /**
     * @brief Allocates GPU memory capacity for capacity elements, releasing existing allocations.
     * Resets size to 0. Throws std::invalid_argument if capacity == 0.
     *
     * @param capacity Element capacity to allocate across all SoA buffers.
     */
    void allocate(std::size_t capacity);

    /**
     * @brief Ensures GPU allocation capacity across all SoA arrays is at least newCapacity.
     * Preserves existing decoded elements if reallocation occurs.
     * Throws std::invalid_argument if newCapacity == 0.
     *
     * @param newCapacity Target element capacity.
     */
    void reserve(std::size_t newCapacity);

    /**
     * @brief Sets logical size to newSize, auto-expanding capacity via reserve() if needed.
     * Following std::vector semantics, if newSize == 0, size becomes 0 without freeing memory.
     *
     * @param newSize Target logical size.
     */
    void resize(std::size_t newSize);

    /**
     * @brief Resets logical size to 0 without releasing allocated GPU memory capacity.
     * Guarantees noexcept.
     */
    void clear() noexcept;

    /**
     * @brief Releases all allocated GPU memory across all SoA arrays and resets size and capacity to 0.
     * Guarantees noexcept.
     */
    void release() noexcept;

    /**
     * @brief Gets current logical size (number of valid decoded events).
     * @return Element size.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Gets allocated element capacity across SoA arrays.
     * @return Capacity in elements.
     */
    [[nodiscard]] std::size_t capacity() const noexcept;

    /**
     * @brief Checks if buffer is logically empty (size() == 0).
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Checks if all internal DeviceBuffers hold valid GPU allocations.
     * @return True if valid, false otherwise.
     */
    [[nodiscard]] bool isValid() const noexcept;

    /**
     * @brief Gets total size of active decoded data in bytes across all SoA arrays.
     * @return Active data size in bytes.
     */
    [[nodiscard]] std::size_t sizeBytes() const noexcept;

    /**
     * @brief Gets total allocated capacity in bytes across all SoA arrays.
     * @return Total capacity in bytes.
     */
    [[nodiscard]] std::size_t capacityBytes() const noexcept;

    // Buffer Data Accessors
    [[nodiscard]] uint64_t* timestamps() noexcept { return timestampsBuf.data(); }
    [[nodiscard]] const uint64_t* timestamps() const noexcept { return timestampsBuf.data(); }

    [[nodiscard]] uint8_t* eventTypes() noexcept { return eventTypesBuf.data(); }
    [[nodiscard]] const uint8_t* eventTypes() const noexcept { return eventTypesBuf.data(); }

    [[nodiscard]] int* orderIds() noexcept { return orderIdsBuf.data(); }
    [[nodiscard]] const int* orderIds() const noexcept { return orderIdsBuf.data(); }

    [[nodiscard]] uint8_t* sides() noexcept { return sidesBuf.data(); }
    [[nodiscard]] const uint8_t* sides() const noexcept { return sidesBuf.data(); }

    [[nodiscard]] uint8_t* orderTypes() noexcept { return orderTypesBuf.data(); }
    [[nodiscard]] const uint8_t* orderTypes() const noexcept { return orderTypesBuf.data(); }

    [[nodiscard]] double* prices() noexcept { return pricesBuf.data(); }
    [[nodiscard]] const double* prices() const noexcept { return pricesBuf.data(); }

    [[nodiscard]] int* quantities() noexcept { return quantitiesBuf.data(); }
    [[nodiscard]] const int* quantities() const noexcept { return quantitiesBuf.data(); }

    [[nodiscard]] int* displayQuantities() noexcept { return displayQuantitiesBuf.data(); }
    [[nodiscard]] const int* displayQuantities() const noexcept { return displayQuantitiesBuf.data(); }

    [[nodiscard]] int* reserveQuantities() noexcept { return reserveQuantitiesBuf.data(); }
    [[nodiscard]] const int* reserveQuantities() const noexcept { return reserveQuantitiesBuf.data(); }

    // DeviceBuffer Accessors
    [[nodiscard]] DeviceBuffer<uint64_t>& timestampsBuffer() noexcept { return timestampsBuf; }
    [[nodiscard]] const DeviceBuffer<uint64_t>& timestampsBuffer() const noexcept { return timestampsBuf; }

    [[nodiscard]] DeviceBuffer<uint8_t>& eventTypesBuffer() noexcept { return eventTypesBuf; }
    [[nodiscard]] const DeviceBuffer<uint8_t>& eventTypesBuffer() const noexcept { return eventTypesBuf; }

    [[nodiscard]] DeviceBuffer<int>& orderIdsBuffer() noexcept { return orderIdsBuf; }
    [[nodiscard]] const DeviceBuffer<int>& orderIdsBuffer() const noexcept { return orderIdsBuf; }

    [[nodiscard]] DeviceBuffer<uint8_t>& sidesBuffer() noexcept { return sidesBuf; }
    [[nodiscard]] const DeviceBuffer<uint8_t>& sidesBuffer() const noexcept { return sidesBuf; }

    [[nodiscard]] DeviceBuffer<uint8_t>& orderTypesBuffer() noexcept { return orderTypesBuf; }
    [[nodiscard]] const DeviceBuffer<uint8_t>& orderTypesBuffer() const noexcept { return orderTypesBuf; }

    [[nodiscard]] DeviceBuffer<double>& pricesBuffer() noexcept { return pricesBuf; }
    [[nodiscard]] const DeviceBuffer<double>& pricesBuffer() const noexcept { return pricesBuf; }

    [[nodiscard]] DeviceBuffer<int>& quantitiesBuffer() noexcept { return quantitiesBuf; }
    [[nodiscard]] const DeviceBuffer<int>& quantitiesBuffer() const noexcept { return quantitiesBuf; }

    [[nodiscard]] DeviceBuffer<int>& displayQuantitiesBuffer() noexcept { return displayQuantitiesBuf; }
    [[nodiscard]] const DeviceBuffer<int>& displayQuantitiesBuffer() const noexcept { return displayQuantitiesBuf; }

    [[nodiscard]] DeviceBuffer<int>& reserveQuantitiesBuffer() noexcept { return reserveQuantitiesBuf; }
    [[nodiscard]] const DeviceBuffer<int>& reserveQuantitiesBuffer() const noexcept { return reserveQuantitiesBuf; }

    /**
     * @brief Gets raw device pointer view for launching CUDA kernels.
     * @return DecodedEventSoAView containing raw device pointers to each array.
     */
    [[nodiscard]] DecodedEventSoAView getDeviceView() noexcept;

    /**
     * @brief Gets const raw device pointer view for read-only access in CUDA kernels.
     * @return ConstDecodedEventSoAView containing const raw device pointers to each array.
     */
    [[nodiscard]] ConstDecodedEventSoAView getDeviceView() const noexcept;
};
