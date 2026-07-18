# ---------------------------------------
# Matching Tests
# ---------------------------------------

add_executable(
    test_matching

    test_matching.cpp
)

target_link_libraries(
    test_matching

    PRIVATE

    lob_engine
)

# ---------------------------------------
# Cancellation Tests
# ---------------------------------------

add_executable(
    test_cancel

    test_cancel.cpp
)

target_link_libraries(
    test_cancel

    PRIVATE

    lob_engine
)

# ---------------------------------------
# Edge Case Tests
# ---------------------------------------

add_executable(
    test_edge_cases

    test_edge_cases.cpp
)

target_link_libraries(
    test_edge_cases

    PRIVATE

    lob_engine
)


add_executable(
    test_replay

    test_replay.cpp
)

target_link_libraries(
    test_replay

    PRIVATE

    lob_engine
)

add_executable(
    test_csv_reader

    test_csv_reader.cpp
)

target_link_libraries(
    test_csv_reader

    PRIVATE

    lob_engine
)


add_executable(
    test_csv_replay

    test_csv_replay.cpp
)

target_link_libraries(
    test_csv_replay

    PRIVATE

    lob_engine
)