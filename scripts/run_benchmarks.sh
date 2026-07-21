#!/bin/bash

echo "======================================"
echo "Building Benchmarks..."
echo "======================================"

g++ -O2 -std=c++20 \
-Iinclude \
benchmark/benchmark_insert.cpp \
src/OrderBook.cpp \
src/dispatcher/OrderDispatcher.cpp \
src/handlers/LimitHandler.cpp \
src/handlers/MarketHandler.cpp \
-o benchmark_insert

g++ -O2 -std=c++20 \
-Iinclude \
benchmark/benchmark_match.cpp \
src/OrderBook.cpp \
src/dispatcher/OrderDispatcher.cpp \
src/handlers/LimitHandler.cpp \
src/handlers/MarketHandler.cpp \
-o benchmark_match

g++ -O2 -std=c++20 \
-Iinclude \
benchmark/benchmark_cancel.cpp \
src/OrderBook.cpp \
src/dispatcher/OrderDispatcher.cpp \
src/handlers/LimitHandler.cpp \
src/handlers/MarketHandler.cpp \
-o benchmark_cancel

echo
echo "======================================"
echo "INSERTION BENCHMARK"
echo "======================================"

./benchmark_insert

echo
echo "======================================"
echo "MATCHING BENCHMARK"
echo "======================================"

./benchmark_match

echo
echo "======================================"
echo "CANCELLATION BENCHMARK"
echo "======================================"

./benchmark_cancel