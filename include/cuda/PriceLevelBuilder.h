#pragma once

#include "cuda/CUDAContext.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/ClassifiedEventBuffer.h"
#include "cuda/PriceLevelBuffer.h"

#include <cuda_runtime.h>

/**
 * @brief Constructs CSR-style GPU PriceLevelBuffer from classified Add events.
 *
 * Converts floating-point limit prices to integer price ticks (uint32_t), maintains distinct Buy vs. Sell
 * price levels at the same price tick, aggregates order quantities/counts per price level, and records CSR
 * contiguous order index mappings (firstOrder, orderCount) in shared orderIndices memory for matching kernels.
 *
 * @param decodedBuffer Input DecodedEventBuffer containing SoA event attribute streams.
 * @param classifiedBuffer Input ClassifiedEventBuffer containing classified Add event indices.
 * @param priceLevelBuffer Output PriceLevelBuffer receiving CSR price levels and order index mapping.
 * @param context Active CUDAContext for stream execution and synchronization.
 * @param tickSize Multiplier step for converting price double to uint32_t price tick (default: 0.01).
 */
void buildPriceLevels(
    const DecodedEventBuffer& decodedBuffer,
    const ClassifiedEventBuffer& classifiedBuffer,
    PriceLevelBuffer& priceLevelBuffer,
    CUDAContext& context,
    double tickSize = 0.01);

/**
 * @brief Asynchronously constructs GPU PriceLevelBuffer on the specified CUDA stream.
 *
 * @param decodedBuffer Input DecodedEventBuffer containing SoA event attribute streams.
 * @param classifiedBuffer Input ClassifiedEventBuffer containing classified Add event indices.
 * @param priceLevelBuffer Output PriceLevelBuffer receiving CSR price levels and order index mapping.
 * @param stream CUDA stream handle for asynchronous kernel launch.
 * @param tickSize Multiplier step for converting price double to uint32_t price tick (default: 0.01).
 */
void buildPriceLevelsAsync(
    const DecodedEventBuffer& decodedBuffer,
    const ClassifiedEventBuffer& classifiedBuffer,
    PriceLevelBuffer& priceLevelBuffer,
    cudaStream_t stream = nullptr,
    double tickSize = 0.01);
