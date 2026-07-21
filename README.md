# # GPU Order Book Simulator

A modular C++20 limit order book simulator designed as a reference implementation for historical market replay, backtesting, and future GPU acceleration.

Version 1.0 focuses on correctness, modularity, and extensibility. Future versions will introduce GPU acceleration using CUDA.



---

## Overview

The simulator models the core functionality of an electronic limit order book while keeping the architecture modular and easy to extend.
The project is structured so that individual components such as replay, matching, portfolio management, and risk analysis remain independent and can evolve without affecting the rest of the system.

Current capabilities include:

- Price-time priority matching
- Limit, Market, Cancel, and Iceberg orders
- Historical event replay
- Portfolio, risk, and statistics modules
- Automated regression tests
- Modular dispatcher-based order processing

Rather than optimizing prematurely, Version 1.0 focuses on building a correct reference implementation that can later be used to validate GPU-accelerated components.

---
## Project Snapshot

- **Language:** C++20
- **Regression Tests:** 16 (100% passing)
- **Supported Order Types:** Limit, Market, Cancel, Iceberg
- **Build System:** CMake
- **Current Version:** v1.0

---

## Engineering Philosophy

The primary goal of Version 1.0 is to build a correct, modular, and extensible matching engine before pursuing performance optimization. This CPU implementation serves as the reference model for future CUDA acceleration and performance-focused development.

---

## Key Features

- **Price-Time Priority Matching** using ordered price levels and FIFO execution within each price level.
- **Constant-Time Order Cancellation** using iterator indexing for direct order lookup.
- **Historical Replay Pipeline** that replays market events independently of the underlying data source.
- **Resting Iceberg Orders** with automatic tranche replenishment while preserving queue fairness.
- **Modular Architecture** separating replay, matching, portfolio, risk, and statistics components.

---

## System Architecture

```text
Market Data
      │
      ▼
 Replay Engine
      │
      ▼
 Order Dispatcher
      │
      ▼
  Order Book
      │
      ▼
 Matching Engine
      │
      ├── Portfolio
      ├── Risk
      └── Statistics
```

---

## Repository Structure

```text
.
├── include/          # Public headers
├── src/              # Core implementation
├── tests/            # Regression tests
├── benchmarks/        # Performance benchmarks
├── docs/             # Design documentation
├── data/             # Sample replay datasets
└── CMakeLists.txt
```

---
## Documentation

Additional documentation is available in the `docs/` directory.

- Design Decisions
- Architecture *(planned)*
- Benchmarks *(planned)*

---

## Build

```bash
mkdir build
cd build

cmake ..
cmake --build .
```

---

## Running Tests

```bash
ctest --output-on-failure
```

Current status:

- **16 automated regression tests**
- **Executed using CTest**
- **100% passing**

---

## Design Decisions

Major design decisions, implementation trade-offs, and the reasoning behind key architectural choices are documented in:

```text
docs/DesignDecisions.md
```

The document explains the reasoning, alternatives considered, and trade-offs behind major implementation choices.

---

## Roadmap

### Current Status

- ✅ Matching engine
- ✅ Replay engine
- ✅ Portfolio and risk modules
- ✅ Iceberg orders
- ✅ Automated regression testing

### Version 2.0

- CUDA-based replay engine
- GPU-accelerated matching
- Performance profiling
- Historical dataset validation
- CPU vs GPU performance comparison

---

## Future Work

Future work includes:

- **GPU-accelerated replay using CUDA**
- **GPU matching engine**
- **Performance profiling and optimization**
- **Validation against historical market datasets**
- **CPU vs GPU performance comparison**
---

## License

This project is released under the MIT License.