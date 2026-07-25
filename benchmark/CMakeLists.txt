# ---------------------------------------
# Insertion Benchmark
# ---------------------------------------

add_executable(
    benchmark_insert

    benchmark_insert.cpp
)

target_link_libraries(
    benchmark_insert

    PRIVATE

    lob_engine
)

# ---------------------------------------
# Matching Benchmark
# ---------------------------------------

add_executable(
    benchmark_match

    benchmark_match.cpp
)

target_link_libraries(
    benchmark_match

    PRIVATE

    lob_engine
)

# ---------------------------------------
# Cancellation Benchmark
# ---------------------------------------

add_executable(
    benchmark_cancel

    benchmark_cancel.cpp
)

target_link_libraries(
    benchmark_cancel

    PRIVATE

    lob_engine
)

# ---------------------------------------
# Vector Operations Benchmark (Optional CUDA)
# ---------------------------------------

if(ENABLE_CUDA AND CMAKE_CUDA_COMPILER)
    add_executable(
        benchmark_vector_ops

        benchmark_vector_ops.cpp
    )

    target_link_libraries(
        benchmark_vector_ops

        PRIVATE

        cuda_core
    )
endif()