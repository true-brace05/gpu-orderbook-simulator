#pragma once

#include "cuda/CUDAContext.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/ClassifiedEventBuffer.h"
#include "cuda/PriceLevelBuffer.h"
#include "cuda/TradeBuffer.h"

#include <cuda_runtime.h>
#include <cstddef>
#include <cstdint>

/**
 * @brief Execution metrics produced by the GPU Matching Engine.
 */
struct MatchingStatistics
{
    std::size_t tradesGenerated = 0;
    std::size_t fullyMatchedOrders = 0;
    std::size_t partiallyMatchedOrders = 0;
    std::size_t restedOrders = 0;

    void clear() noexcept
    {
        tradesGenerated = 0;
        fullyMatchedOrders = 0;
        partiallyMatchedOrders = 0;
        restedOrders = 0;
    }
};

/**
 * @brief Matches incoming classified Add events against resting PriceLevelBuffer in price-time priority.
 *
 * Incoming orders (pre-sorted by timestamp) are matched sequentially against resting orders in FIFO CSR order.
 * Full/partial fills emit TradeRecords into TradeBuffer, updating MatchingStatistics.
 * Unmatched residual quantities rest on the order book.
 *
 * @param incomingEvents Input DecodedEventBuffer containing incoming order attributes.
 * @param classifiedEvents Input ClassifiedEventBuffer containing classified Add event indices.
 * @param restingBook Resting PriceLevelBuffer order book.
 * @param tradeOutput Output TradeBuffer receiving executed trade records.
 * @param statistics Output MatchingStatistics tracking execution counts.
 * @param context Active CUDAContext for stream execution and synchronization.
 * @param tickSize Multiplier step for converting price double to uint32_t price tick (default: 0.01).
 */
void matchAddOrders(
    const DecodedEventBuffer& incomingEvents,
    const ClassifiedEventBuffer& classifiedEvents,
    PriceLevelBuffer& restingBook,
    TradeBuffer& tradeOutput,
    MatchingStatistics& statistics,
    CUDAContext& context,
    double tickSize = 0.01);

/**
 * @brief Asynchronously matches incoming Add events on the specified CUDA stream.
 *
 * @param incomingEvents Input DecodedEventBuffer containing incoming order attributes.
 * @param classifiedEvents Input ClassifiedEventBuffer containing classified Add event indices.
 * @param restingBook Resting PriceLevelBuffer order book.
 * @param tradeOutput Output TradeBuffer receiving executed trade records.
 * @param statistics Output MatchingStatistics tracking execution counts.
 * @param stream CUDA stream handle for asynchronous kernel launch.
 * @param tickSize Multiplier step for converting price double to uint32_t price tick (default: 0.01).
 */
void matchAddOrdersAsync(
    const DecodedEventBuffer& incomingEvents,
    const ClassifiedEventBuffer& classifiedEvents,
    PriceLevelBuffer& restingBook,
    TradeBuffer& tradeOutput,
    MatchingStatistics& statistics,
    cudaStream_t stream = nullptr,
    double tickSize = 0.01);
