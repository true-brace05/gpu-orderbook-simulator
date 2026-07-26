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

    add_executable(
        benchmark_replay_buffer

        benchmark_replay_buffer.cpp
    )

    target_link_libraries(
        benchmark_replay_buffer

        PRIVATE

        cuda_core
    )

    add_executable(
        benchmark_event_decoder

        benchmark_event_decoder.cpp
    )

    target_link_libraries(
        benchmark_event_decoder

        PRIVATE

        cuda_core
    )

    add_executable(
        benchmark_event_classifier

        benchmark_event_classifier.cpp
    )

    target_link_libraries(
        benchmark_event_classifier

        PRIVATE

        cuda_core
    )

    add_executable(
        benchmark_price_level_builder

        benchmark_price_level_builder.cpp
    )

    target_link_libraries(
        benchmark_price_level_builder

        PRIVATE

        cuda_core
    )

    add_executable(
        benchmark_gpu_matching_engine

        benchmark_gpu_matching_engine.cpp
    )

    target_link_libraries(
        benchmark_gpu_matching_engine

        PRIVATE

        cuda_core
    )
endif()