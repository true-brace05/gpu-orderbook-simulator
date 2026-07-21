# Design Decisions

This document records the major architectural and implementation decisions made during the development of the GPU Order Book Simulator.

The goal is to explain **what decision was made** and **why it was chosen**. It serves both as project documentation and as a reference for technical interviews.

---

# Decision 001: Represent Order Side

**Decision**

Use `enum class Side` instead of `bool` or `std::string`.

**Reason**

Improves type safety, readability, and prevents invalid values.

---

# Decision 002: Order Representation

**Decision**

Store only the essential attributes inside an order:

- Order ID
- Side
- Price
- Quantity
- Timestamp

**Reason**

Keeps the order lightweight while supporting matching, cancellation, and FIFO execution.

---

# Decision 003: Price-Level Storage

**Decision**

Store price levels using:

```cpp
std::map<double, std::list<Order>>
```

**Reason**

Maintains sorted price levels while allowing efficient access to the best bid and best ask.

---

# Decision 004: Queue at Each Price Level

**Decision**

Store orders at the same price using `std::list`.

**Reason**

Provides stable iterators, O(1) insertion and deletion, and naturally preserves FIFO ordering.

---

# Decision 005: Fast Order Cancellation

**Decision**

Maintain an order index:

```cpp
std::unordered_map<int, std::list<Order>::iterator>
```

**Reason**

Enables O(1) lookup and cancellation without searching through price levels.

---

# Decision 006: Separate Matching from Order Entry

**Decision**

Keep `addOrder()` responsible for routing while dedicated matching functions execute trades.

**Reason**

Improves readability, testability, and simplifies future extensions.

---

# Decision 007: Descending Buy Book

**Decision**

Store the buy book using:

```cpp
std::map<double, std::list<Order>, std::greater<double>>
```

**Reason**

The best bid is always available at `begin()`, simplifying the matching algorithm.

---

# Decision 008: FIFO Execution

**Decision**

Always execute the order at the front of a price level.

**Reason**

Preserves the time component of Price-Time Priority.

---

# Decision 009: Iterator Storage

**Decision**

Store the iterator immediately after inserting a resting order.

**Reason**

Allows constant-time cancellation while avoiding future searches.

---

# Decision 010: Dispatcher Architecture

**Decision**

Introduce an `OrderDispatcher` between incoming orders and the matching engine.

**Reason**

Separates routing from matching and allows new order types without modifying the core engine.

---

# Decision 011: Handler-Based Processing

**Decision**

Process each order type using a dedicated handler.

**Reason**

Each handler owns its own validation and execution logic, improving modularity and maintainability.

---

# Decision 012: Market Order Design

**Decision**

Reuse the existing matching engine for Market Orders.

**Reason**

Avoids duplicate matching logic while changing only insertion behavior.

---

# Decision 013: Replay Engine Abstraction

**Decision**

ReplayEngine depends only on the `IEventReader` interface.

**Reason**

Replay becomes independent of the underlying data source, making additional readers easy to integrate.

---

# Decision 014: Production vs Test Code

**Decision**

Keep production code and test code in separate directories.

**Reason**

Improves maintainability and allows focused regression testing.

---

# Decision 015: Read-Only Test APIs

**Decision**

Expose only read-only helper functions for testing.

Examples:

- `isEmpty()`
- `getOrder()`
- `getTotalOrders()`

**Reason**

Allows verification without exposing internal data structures.

---

# Decision 016: Feature-Oriented Tests

**Decision**

Organize regression tests by subsystem.

Examples:

- Matching
- Cancellation
- Replay
- Portfolio
- Risk

**Reason**

Failures become easier to identify and maintain.

---

# Decision 017: Benchmark Before Optimizing

**Decision**

Measure performance before making optimizations.

**Reason**

Engineering decisions should be driven by data instead of assumptions.

---

# Decision 018: Modular Build System

**Decision**

Build the engine as a reusable CMake library.

**Reason**

Allows multiple executables (tests, benchmarks, demos) to reuse the same implementation.

---

# Decision 019: Iceberg Order Representation

**Decision**

Extend the `Order` structure with:

```cpp
displayQuantity
reserveQuantity
```

**Reason**

Supports hidden liquidity without changing the existing order book structure.

---

# Decision 020: Iceberg Replenishment

**Decision**

When the visible tranche is exhausted, replenish it by appending the next tranche to the back of the same price-level queue.

**Reason**

Preserves queue fairness while maintaining Price-Time Priority.

---

# Decision 021: CPU-First Development

**Decision**

Complete and validate the CPU implementation before introducing CUDA acceleration.

**Reason**

A correct CPU reference simplifies debugging, testing, benchmarking, and future GPU validation.

---