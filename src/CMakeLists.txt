# ---------------------------------------
# Core Engine Library (CPU Only)
# ---------------------------------------

add_library(
    lob_engine

    OrderBook.cpp

    dispatcher/OrderDispatcher.cpp

    handlers/LimitHandler.cpp

    handlers/MarketHandler.cpp

    replay/ReplayEngine.cpp

    replay/readers/SyntheticReader.cpp

    replay/readers/CSVReader.cpp

    strategy/PassiveBuyer.cpp

    portfolio/Portfolio.cpp

    backtest/Backtester.cpp

    backtest/StatisticsCollector.cpp

    execution/ExecutionEngine.cpp

    risk/RiskEngine.cpp

    performance/Timer.cpp
    performance/BenchmarkRunner.cpp

    performance/ReportPrinter.cpp

    marketdata/LOBSTERReader.cpp
    marketdata/LOBSTEROrderBookReader.cpp

    marketdata/LOBSTERValidator.cpp
)

# ---------------------------------------
# Include Directories
# ---------------------------------------

target_include_directories(
    lob_engine

    PUBLIC

    ${PROJECT_SOURCE_DIR}/include
)

# ---------------------------------------
# CUDA Core Library (Optional)
# ---------------------------------------

if(ENABLE_CUDA AND CMAKE_CUDA_COMPILER)
    add_library(
        cuda_core

        cuda/CUDAContext.cu
        cuda/DeviceBuffer.cu
        cuda/ReplayBuffer.cu
        cuda/DecodedEventBuffer.cu
        cuda/EventDecoder.cu
        cuda/hello_kernel.cu
        cuda/vector_ops.cu
    )

    target_include_directories(
        cuda_core

        PUBLIC

        ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(
        cuda_core

        PUBLIC

        CUDA::cudart
    )

    target_compile_definitions(
        cuda_core

        PRIVATE

        LOB_ENABLE_CUDA
    )
endif()

# ---------------------------------------
# Main Executable
# ---------------------------------------

add_executable(
    main

    main.cpp
)

# ---------------------------------------
# Link Engine
# ---------------------------------------

target_link_libraries(
    main

    PRIVATE

    lob_engine
)