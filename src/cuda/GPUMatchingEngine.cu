#include "cuda/GPUMatchingEngine.h"
#include "cuda/DeviceBuffer.h"

#include <cuda_runtime.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// CUDA Kernel
// -----------------------------------------------------------------------------

/**
 * @brief CUDA kernel for matching incoming Add orders against resting PriceLevelBuffer in price-time priority.
 * One block (or thread) processes one incoming Add order sequentially in timestamp order.
 *
 * @param addIndices Pointer to classified Add event indices.
 * @param numAddEvents Total number of incoming Add orders.
 * @param incomingSoA Raw view of incoming DecodedEventBuffer attributes.
 * @param levels Array of resting PriceLevels on GPU.
 * @param orderIndices Array of resting order indices (CSR layout).
 * @param numLevels Number of active resting price levels.
 * @param trades Array of output TradeRecords.
 * @param tradeCount Pointer to atomic trade counter on GPU.
 * @param maxTrades Allocated TradeBuffer capacity.
 * @param gpuStats Device array for atomic execution statistics (4 ints).
 * @param tickSize Price tick conversion multiplier.
 */
__global__ void matchAddOrdersKernel(
    const int* addIndices,
    std::size_t numAddEvents,
    ConstDecodedEventSoAView incomingSoA,
    ConstDecodedEventSoAView restingSoA,
    PriceLevel* levels,
    const int* orderIndices,
    std::size_t numLevels,
    TradeRecord* trades,
    int* tradeCount,
    std::size_t maxTrades,
    int* gpuStats,
    double tickSize)
{
    std::size_t threadIdxGlobal = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (threadIdxGlobal >= numAddEvents)
    {
        return;
    }

    int addIdx = addIndices[threadIdxGlobal];
    int incId = incomingSoA.orderIds[addIdx];
    uint8_t incSide = incomingSoA.sides[addIdx];
    double incPrice = incomingSoA.prices[addIdx];
    uint32_t incTick = static_cast<uint32_t>(llround(incPrice / tickSize));
    int incQty = incomingSoA.quantities[addIdx];
    const int origQty = incQty;
    uint64_t incTime = incomingSoA.timestamps[addIdx];

    if (incQty <= 0)
    {
        return;
    }

    // Match against opposing resting price levels in price-time priority
    for (std::size_t l = 0; l < numLevels && incQty > 0; ++l)
    {
        PriceLevel& level = levels[l];

        // Buy matches Sell (side 1); Sell matches Buy (side 0)
        bool isOppositeSide = (incSide == 0 && level.side == 1) || (incSide == 1 && level.side == 0);
        if (!isOppositeSide)
        {
            continue;
        }

        // Price crossing check: Buy crosses ask <= incTick; Sell crosses bid >= incTick
        bool crossesPrice = (incSide == 0 && level.priceTick <= incTick) ||
                            (incSide == 1 && level.priceTick >= incTick);
        if (!crossesPrice)
        {
            continue;
        }

        // Traverse CSR resting orders in FIFO order
        for (int i = 0; i < level.orderCount && incQty > 0; ++i)
        {
            int restOrderIdx = orderIndices[level.firstOrder + i];
            int restId = restingSoA.orderIds[restOrderIdx];

            // Atomic CAS loop to claim resting order quantity safely across concurrent GPU threads
            int* restQtyPtr = const_cast<int*>(&restingSoA.quantities[restOrderIdx]);
            int oldQty = *restQtyPtr;
            int execQty = 0;

            while (oldQty > 0 && incQty > 0)
            {
                execQty = (incQty < oldQty) ? incQty : oldQty;
                int prevQty = atomicCAS(restQtyPtr, oldQty, oldQty - execQty);
                if (prevQty == oldQty)
                {
                    // Successfully claimed execQty from resting order
                    break;
                }
                oldQty = prevQty;
                execQty = 0;
            }

            if (execQty <= 0)
            {
                continue;
            }

            incQty -= execQty;
            atomicSub(&level.totalQuantity, execQty);

            // Record Trade via Warp-Level Aggregated Allocation
            if (execQty > 0)
            {
#if defined(__CUDA_ARCH__)
                uint32_t activeMask = __activemask();
                uint32_t tradeMask = __ballot_sync(activeMask, true);

                int laneId = threadIdx.x % 32;
                uint32_t lowerLanesMask = (1u << laneId) - 1u;
                int laneOffset = __popc(tradeMask & lowerLanesMask);
                int totalWarpTrades = __popc(tradeMask);
                int leaderLane = __ffs(tradeMask) - 1;

                int warpBaseIdx = 0;
                if (laneId == leaderLane)
                {
                    warpBaseIdx = atomicAdd(tradeCount, totalWarpTrades);
                }
                int baseIdx = __shfl_sync(tradeMask, warpBaseIdx, leaderLane);
                int tradeIdx = baseIdx + laneOffset;
#else
                int tradeIdx = atomicAdd(tradeCount, 1);
#endif

                if (static_cast<std::size_t>(tradeIdx) < maxTrades && trades != nullptr)
                {
                    TradeRecord record;
                    record.buyOrderId = (incSide == 0) ? incId : restId;
                    record.sellOrderId = (incSide == 0) ? restId : incId;
                    record.priceTick = level.priceTick; // Execution at resting order price tick
                    record.executedQuantity = execQty;
                    record.timestamp = incTime;
                    record.aggressorSide = incSide;

                    trades[tradeIdx] = record;
                }
            }

            atomicAdd(&gpuStats[0], 1); // tradesGenerated
        }
    }

    // Update execution statistics
    if (incQty == 0)
    {
        atomicAdd(&gpuStats[1], 1); // fullyMatchedOrders
    }
    else if (incQty < origQty)
    {
        atomicAdd(&gpuStats[2], 1); // partiallyMatchedOrders
    }
    else
    {
        atomicAdd(&gpuStats[3], 1); // restedOrders
    }
}

// -----------------------------------------------------------------------------
// Host API Implementation
// -----------------------------------------------------------------------------

void matchAddOrdersAsync(
    const DecodedEventBuffer& incomingEvents,
    const ClassifiedEventBuffer& classifiedEvents,
    const DecodedEventBuffer& restingEvents,
    PriceLevelBuffer& restingBook,
    TradeBuffer& tradeOutput,
    MatchingStatistics& statistics,
    cudaStream_t stream,
    double tickSize)
{
    if (tickSize <= 0.0)
    {
        throw std::invalid_argument("matchAddOrdersAsync: tickSize must be greater than zero");
    }

    const std::size_t addCount = classifiedEvents.getCount(EventType::Add);
    if (addCount == 0)
    {
        tradeOutput.clear();
        statistics.clear();
        return;
    }

    if (!incomingEvents.isValid())
    {
        throw std::invalid_argument("matchAddOrdersAsync: Input DecodedEventBuffer is not valid");
    }

    if (!classifiedEvents.isValid())
    {
        throw std::invalid_argument("matchAddOrdersAsync: Input ClassifiedEventBuffer is not valid");
    }

    if (!restingEvents.isValid())
    {
        throw std::invalid_argument("matchAddOrdersAsync: Resting DecodedEventBuffer is not valid");
    }

    // Reserve trade output capacity (estimate: up to 2 trades per incoming order)
    const std::size_t estimatedTradeCapacity = std::max<std::size_t>(addCount * 2, 64);
    if (tradeOutput.capacity() < estimatedTradeCapacity)
    {
        tradeOutput.reserve(estimatedTradeCapacity);
    }

    tradeOutput.clear();
    statistics.clear();

    if (!tradeOutput.isValid())
    {
        throw std::invalid_argument("matchAddOrdersAsync: Output TradeBuffer is not valid");
    }

    // Allocate 4 ints GPU device buffer for statistics
    DeviceBuffer<int> statsBuffer(4);
    statsBuffer.zeroMemory();

    constexpr int blockSize = 256;
    const int gridSize = static_cast<int>((addCount + blockSize - 1) / blockSize);

    matchAddOrdersKernel<<<gridSize, blockSize, 0, stream>>>(
        classifiedEvents.getIndices(EventType::Add),
        addCount,
        incomingEvents.getDeviceView(),
        restingEvents.getDeviceView(),
        restingBook.data(),
        restingBook.orderIndicesData(),
        restingBook.size(),
        tradeOutput.data(),
        tradeOutput.tradeCountDevicePtr(),
        tradeOutput.capacity(),
        statsBuffer.data(),
        tickSize
    );

    detail::checkDeviceBufferCudaError(
        cudaGetLastError(),
        "Failed to launch matchAddOrdersKernel"
    );

    tradeOutput.updateHostCount(stream);

    // Sync statistics from GPU to host
    int hostStats[4] = {0};
    if (stream != nullptr)
    {
        statsBuffer.copyToHostAsync(hostStats, 4, stream);
        cudaStreamSynchronize(stream);
    }
    else
    {
        statsBuffer.copyToHost(hostStats, 4);
    }

    statistics.tradesGenerated = static_cast<std::size_t>(hostStats[0]);
    statistics.fullyMatchedOrders = static_cast<std::size_t>(hostStats[1]);
    statistics.partiallyMatchedOrders = static_cast<std::size_t>(hostStats[2]);
    statistics.restedOrders = static_cast<std::size_t>(hostStats[3]);
}

void matchAddOrders(
    const DecodedEventBuffer& incomingEvents,
    const ClassifiedEventBuffer& classifiedEvents,
    const DecodedEventBuffer& restingEvents,
    PriceLevelBuffer& restingBook,
    TradeBuffer& tradeOutput,
    MatchingStatistics& statistics,
    CUDAContext& context,
    double tickSize)
{
    matchAddOrdersAsync(
        incomingEvents,
        classifiedEvents,
        restingEvents,
        restingBook,
        tradeOutput,
        statistics,
        context.getStream(),
        tickSize
    );
    context.synchronize();
}
