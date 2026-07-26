#pragma once

#include "cuda/ClassifiedEventBuffer.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/DeviceBuffer.h"
#include "Types.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Representation of an individual resting order slot in GPU memory.
 */
struct GPUOrderState
{
    int orderId = 0;           // Unique Order ID
    uint32_t priceTick = 0;    // Quantized price tick
    int quantity = 0;          // Active remaining quantity
    uint8_t side = 0;          // 0 = Buy, 1 = Sell
    uint64_t timestamp = 0;    // Arrival timestamp
    uint8_t status = 0;        // 0 = Active, 1 = Canceled, 2 = Empty
};

/**
 * @brief GPU Open-Addressing Hash Table entry mapping orderId -> GPUOrderState array slotIndex.
 */
struct HashEntry
{
    int orderId = -1;   // Key (-1 = Empty)
    int slotIndex = -1; // Value (Index into GPUOrderState array)
};

/**
 * @brief Aggregate level statistics tracked per price tick and side.
 */
struct GPULevelAggregate
{
    uint32_t priceTick = 0;
    uint8_t side = 0;
    int totalQuantity = 0;
    int activeOrderCount = 0;
};

/**
 * @brief GPU Order Book State Manager (Phase 1: Shadow Mode Add & Cancel).
 *
 * Authoritative GPU owner of resting order slots, hash map index lookups, and level aggregates.
 * In Phase 1, runs in Shadow Mode alongside PriceLevelBuilder and GPUMatchingEngine without
 * modifying existing matching execution.
 */
class GPUOrderBookStateManager
{
private:
    DeviceBuffer<GPUOrderState> ordersBuf;
    DeviceBuffer<HashEntry> hashMapBuf;
    DeviceBuffer<GPULevelAggregate> levelAggregatesBuf;
    DeviceBuffer<int> activeOrderCountBuf;

    std::size_t orderCapacity = 0;
    std::size_t hashCapacity = 0;
    std::size_t levelCapacity = 0;

public:
    /**
     * @brief Constructs an unallocated GPUOrderBookStateManager.
     */
    GPUOrderBookStateManager() noexcept = default;

    /**
     * @brief Constructs a GPUOrderBookStateManager and allocates GPU memory buffers.
     * @param maxOrders Maximum number of resting orders supported.
     * @param maxLevels Maximum price levels supported (default 10,000).
     */
    explicit GPUOrderBookStateManager(std::size_t maxOrders, std::size_t maxLevels = 10000);

    /**
     * @brief Processes incoming Add and Cancel event streams in Shadow Mode.
     *
     * Mutates GPUOrderState slots, updates the GPU hash table, and adjusts level aggregates
     * asynchronously on the specified CUDA stream.
     */
    void processEventsShadow(
        const ClassifiedEventBuffer& classifiedEvents,
        const DecodedEventBuffer& decodedEvents,
        double tickSize,
        cudaStream_t stream = nullptr);

    /**
     * @brief Verifies batch invariants at batch completion.
     *
     * Checks:
     * 1. Aggregate quantity invariant (sum of order quantities == level totalQuantity).
     * 2. Active order count invariant.
     * 3. Host CPU reference equivalence.
     *
     * @param decodedEvents Decoded event buffer for host CPU cross-validation.
     * @param tickSize Market tick size.
     * @param outErrorMessage Optional output string for diagnostic error messages.
     * @return true if all invariants pass; false if a mismatch is detected.
     */
    bool verifyBatch(
        const DecodedEventBuffer& decodedEvents,
        double tickSize,
        std::string* outErrorMessage = nullptr);

    /**
     * @brief Resets all GPU order slots, hash table entries, and level aggregates to zero/empty.
     */
    void reset(cudaStream_t stream = nullptr);

    /**
     * @brief Gets current active order count on GPU.
     */
    [[nodiscard]] std::size_t getActiveOrderCount(cudaStream_t stream = nullptr) const;

    [[nodiscard]] std::size_t getOrderCapacity() const noexcept { return orderCapacity; }
    [[nodiscard]] std::size_t getHashCapacity() const noexcept { return hashCapacity; }
};
