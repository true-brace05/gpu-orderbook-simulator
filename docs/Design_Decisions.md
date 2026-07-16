# Design Decisions

This document records the major architectural and implementation decisions made during the development of the GPU-Accelerated Limit Order Book Simulator.

The purpose is to explain **why** each design was chosen, what alternatives were considered, and the associated trade-offs.

---

# Decision 001: Representing Order Side

## Problem

Every order must specify whether it is a Buy or Sell order.

## Alternatives Considered

1. std::string
2. bool
3. enum class Side

## Decision

Use

```cpp
enum class Side
{
    Buy,
    Sell
};

---

# Decision 002

```md
# Decision 002: Representing an Order

## Problem

What information should every order store?

## Decision

Each Order contains:

- Order ID
- Side
- Price
- Quantity
- Timestamp

## Why?

### Order ID

Used for fast cancellation.

### Side

Determines whether the order belongs to the buy book or sell book.

### Price

Used for matching and price priority.

### Quantity

Supports partial fills.

### Timestamp

Maintains FIFO ordering within the same price level.

## Trade-offs

Keeping the structure minimal reduces memory usage and simplifies copying.
# Decision 003: Price-Level Storage

## Problem

How should orders be organized?

## Alternatives

1. std::vector
2. std::unordered_map
3. std::map

## Decision

```cpp
std::map<double, std::list<Order>>


---

# Decision 004

```md
# Decision 004: Queue at Each Price Level

## Problem

How should orders at the same price be stored?

## Alternatives

1. vector
2. deque
3. list

## Decision

Use

```cpp
std::list<Order>


---

# Decision 005

```md
# Decision 005: Fast Order Cancellation

## Problem

How can an order be cancelled efficiently?

## Alternatives

1. Search entire book
2. Additional lookup table

## Decision

Use

```cpp
std::unordered_map<int, std::list<Order>::iterator>


---

# Decision 006

```md
# Decision 006: addOrder() API

## Problem

Should addOrder modify the caller's object?

## Alternatives

1. Order
2. Order&
3. const Order&

## Decision

```cpp
void addOrder(const Order& order);


---

# Decision 007

```md
# Decision 007: Class Responsibilities

## Order

Stores data only.

It should never contain matching logic.

---

## OrderBook

Stores:

- Buy Book
- Sell Book
- Order Index

Provides:

- addOrder()
- cancelOrder()
- printBook()

---

## Matching Engine

Responsible for:

- Matching
- Partial fills
- Trade execution

This separation keeps each class focused on a single responsibility.
# Decision 008: Separating Interface from Implementation

## Problem

How should the project be organized so that it remains maintainable as the codebase grows?

## Alternatives Considered

1. Write everything in a single .cpp file.
2. Place declarations and implementations together in headers.
3. Separate declarations (.h) from implementations (.cpp).

## Decision

Use header files for declarations and source files for implementations.

Example:

Order.h
OrderBook.h

↓

OrderBook.cpp

↓

main.cpp

## Why?

- Improves readability.
- Reduces compilation dependencies.
- Makes the project easier to scale.
- Matches standard C++ project organization.
- Easier to test and maintain.

## Trade-offs

Requires managing multiple files and a build system (CMake), but greatly improves long-term maintainability.
# Decision 009: Separate Matching Logic from Order Entry

## Problem

Should addOrder() contain the entire matching algorithm?

## Alternatives Considered

1. Implement all logic inside addOrder().
2. Delegate matching to dedicated helper functions.

## Decision

Introduce:

- matchBuyOrder(Order&)
- matchSellOrder(Order&)

addOrder() becomes responsible only for routing the incoming order.

## Why?

- Single Responsibility Principle.
- Easier to read.
- Easier to test.
- Easier to extend later.

## Trade-offs

Slightly more functions, but significantly cleaner architecture.
# Decision 011: Extract Common Trade Logic

## Problem

Both Buy and Sell matching perform identical trade execution.

## Alternatives Considered

1. Duplicate trade logic.
2. Extract into executeTrade().

## Decision

Create a dedicated executeTrade() helper.

## Why?

- Eliminates duplicated code.
- Easier to test.
- Easier to maintain.
- Future changes occur in one place only.

## Trade-offs

Introduces one additional function but significantly improves maintainability.
# Decision 012: Store Buy Orders in Descending Price Order

## Problem

Buy orders must always match the highest available buy price.

## Alternatives Considered

1. Store ascending and use rbegin().
2. Store descending using std::greater<double>.

## Decision

Use:

std::map<double, std::list<Order>, std::greater<double>>

for the buy book.

## Why?

- begin() always returns the best buy.
- Matching code becomes symmetric for buy and sell books.
- Simpler and easier to maintain.

## Trade-offs

Introduces a custom comparator but simplifies matching logic throughout the engine.
# Decision 015: Store Iterators Immediately After Insertion

## Problem

After inserting an order into a price level, the engine needs an iterator for O(1) cancellation.

## Decision

Insert first using push_back(), then obtain the iterator using:

std::prev(container.end())

and immediately store it inside orderIndex.

## Why?

- O(1) lookup during cancellation.
- No searching required.
- Iterator remains valid while the order stays in the list.

## Trade-offs

Requires understanding list iterators, but avoids future linear scans.
# Decision 016: Match Oldest Order First

## Problem

Multiple orders can exist at the same price level.

## Decision

Always execute the order at the front of the list.

## Why?

std::list maintains insertion order.

front() therefore represents the oldest order.

This satisfies the Time component of Price-Time Priority.

## Trade-off

FIFO ordering requires storing orders in insertion order, which std::list naturally provides.
# Decision 017: Store Buy Book in Descending Order

## Problem

A sell order must always match the highest available buy price.

## Decision

Store buyBook as:

std::map<double, std::list<Order>, std::greater<double>>

instead of ascending order.

## Why?

Now:

buyBook.begin()

always returns the highest bid.

The matching algorithm becomes symmetric with the sell book, which uses begin() to obtain the lowest ask.

## Trade-off

Requires a custom comparator but simplifies matching logic throughout the engine.
# Decision 022: Separate Testing from Production Code

## Problem

Testing code mixed inside main.cpp becomes difficult to maintain as the project grows.

## Decision

Move all verification code into a dedicated tests/ directory.

## Why?

- Separates production code from test code.
- Easier to run individual tests.
- Matches professional C++ project structure.
- Keeps main.cpp focused on demonstration or application logic.

## Trade-offs

Introduces additional files, but significantly improves maintainability.
# Decision 023: Add Read-Only Query Functions for Testing

## Problem

Unit tests should verify object state without parsing console output.

## Decision

Expose simple read-only functions such as:

- isEmpty()
- getTotalOrders()

## Why?

- Makes automated testing straightforward.
- Keeps tests independent of output formatting.
- Does not expose mutable internal state.

## Trade-off

Slightly larger public API, but significantly better testability.
# Decision 027: Expose Read-Only Order Lookup

## Problem

Tests need to verify properties such as quantity, price, and timestamp without exposing internal containers.

## Decision

Expose:

std::optional<Order> getOrder(int orderId) const;

## Why?

- Keeps containers private.
- Allows detailed assertions.
- Uses modern C++ optional to represent "order may not exist."

## Trade-off

Returns a copy of the order, which is acceptable because Order is a small object.

# Decision 029: Separate Tests by Feature

## Problem

Matching and cancellation validate different behaviors.

Keeping them in one file makes the suite harder to navigate.

## Decision

Create dedicated test files:

- test_matching.cpp
- test_cancel.cpp

## Why?

Each file focuses on one subsystem, making failures easier to locate and maintain.

## Trade-off

Slightly more files, but much better organization as the project grows.