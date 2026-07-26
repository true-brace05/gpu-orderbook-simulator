# GPU Matching Engine Benchmark Guide

This document describes the benchmark framework design, execution stages, scenarios, metrics, and steps to run reproducible performance tests for the **GPU Order Book Simulator Matching Engine**.

---

## 1. Benchmark Architecture & Stage Breakdown

Every benchmark run measures each pipeline phase independently using high-precision GPU/CPU timers:

| Pipeline Stage | Measurement Description |
| :--- | :--- |
| **Upload H→D** | Copying host `Event` arrays to GPU device `ReplayBuffer` memory. |
| **Event Decode** | `decodeEventsAsync` kernel execution (AoS to SoA attribute extraction). |
| **Event Classification** | `classifyEventsAsync` kernel execution (categorizing event indices by `EventType`). |
| **Price Level Builder** | `buildPriceLevelsAsync` kernel execution (Thrust stable sort & CSR mapping). |
| **GPU Matching Kernel** | `matchAddOrdersAsync` kernel execution (price-time & FIFO CSR matching). |
| **Download D→H** | Synchronizing `TradeBuffer` records and `MatchingStatistics` from device to host memory. |
| **Verification** | Validating execution counts (`fullyMatched + partiallyMatched + rested == N`). |
| **Total End-to-End** | Complete end-to-end processing pipeline latency from host upload to verification. |

### Correctness Guarantee across Iterations
- Before **every timed iteration**, the resting order book is re-built fresh from raw events (`restReplay` → `restDecoded` → `restClassified` → `restingBook`).
- This guarantees that resting order quantities are never zeroed out across iterations and measurements remain 100% reproducible and state-isolated.

---

## 2. Benchmark Scenarios

### Scenario A — No Match (Search Overhead)
- **Market Dynamics**: Incoming Buy orders have limit prices lower than the lowest resting Sell price (non-crossing book).
- **Primary Metric**: Measures pure price-level scan overhead and price-crossing filter performance without trade emissions.
- **Expected Outcome**: `0` trades generated; 100% rested orders.

### Scenario B — Full Match (Throughput Limit)
- **Market Dynamics**: Incoming Buy orders match exact resting Sell order prices and quantities.
- **Primary Metric**: Measures maximum trade emission throughput and atomic execution capacity.
- **Expected Outcome**: `N` trades generated; 100% fully matched orders.

### Scenario C — Mixed Market (Default Workload)
- **Market Dynamics**: Realistic market simulation where ~70% of incoming orders cross resting liquidity and ~30% rest on the book without crossing. Includes partial fills and multi-level price sweeps.
- **Primary Metric**: Serves as the primary benchmark representing a production exchange environment.
- **Expected Outcome**: Combination of full fills, partial fills, and rested orders.

---

## 3. Development vs. Release Configurations

| Configuration | Flag | Batch Sizes | Primary Use Case |
| :--- | :--- | :--- | :--- |
| **Development** (Default) | *(None)* | 1K, 10K, 100K, 1M | Quick iteration and local dev profiling (< 5s execution time). |
| **Release** | `--release` | 1K, 10K, 100K, 1M, 10M | Full throughput benchmarking up to 10 Million events. |

---

## 4. How to Build & Run

### Build Command
```bash
mkdir -p build && cd build
cmake .. -DENABLE_CUDA=ON
make -j$(sysctl -n hw.ncpu)
```

### Run Benchmark in Development Mode (Default)
```bash
./benchmark/benchmark_gpu_matching_engine
```

### Run Benchmark in Release Mode (Up to 10M Events)
```bash
./benchmark/benchmark_gpu_matching_engine --release
```

---

## 5. Sample System Header & Output

When run, the benchmark automatically detects and prints system GPU details:

```text
=========================================================================================================================================
                                   GPU Matching Engine Performance Benchmark                                             
=========================================================================================================================================
GPU Device         : NVIDIA A100-SXM4-40GB
CUDA Runtime       : 12.2
Compute Capability : 8.0
Global Memory      : 40536 MB
=========================================================================================================================================
Configuration      : DEVELOPMENT Mode (1K, 10K, 100K, 1M) [Use --release for 10M]

-----------------------------------------------------------------------------------------------------------------------------------------
  Scenario C — Mixed Market (70% Match / 30% No-Match)
-----------------------------------------------------------------------------------------------------------------------------------------
Orders    Trades    Levels    AvgOrd/Lvl Upload(ms) Decode(ms) Class(ms)  PBuild(ms) Match(ms)  Dnload(ms) Verify(ms) Total(ms)  M Orders/s   M Trades/s   EffectiveGB/s 
1000      700       50        10.0       0.120      0.045      0.038      0.150      0.080      0.015      0.002      0.450      12.50        8.75         0.85          
10000     7000      50        100.0      0.850      0.320      0.280      1.100      0.650      0.090      0.005      3.295      15.38        10.77        1.05          
100000    70000     50        1000.0     8.100      3.100      2.750      10.500     6.200      0.850      0.020      31.520     16.13        11.29        1.10          
1000000   700000    50        10000.0    82.000     30.500     26.800     108.000    64.000     8.200      0.150      319.650    15.63        10.94        1.07          
```
