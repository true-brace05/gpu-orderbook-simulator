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

add_executable(
    test_strategy

    test_strategy.cpp
)

target_link_libraries(
    test_strategy

    PRIVATE

    lob_engine
)

add_executable(
    test_portfolio

    test_portfolio.cpp
)

target_link_libraries(
    test_portfolio

    PRIVATE

    lob_engine
)


add_executable(
    test_trade_listener

    test_trade_listener.cpp
)

target_link_libraries(
    test_trade_listener

    PRIVATE

    lob_engine
)

add_executable(
    test_backtester
    test_backtester.cpp
)

target_link_libraries(
    test_backtester
    PRIVATE
    lob_engine
)

add_executable(
    test_pnl
    test_pnl.cpp
)

target_link_libraries(
    test_pnl
    PRIVATE
    lob_engine
)

add_executable(
    test_risk
    test_risk.cpp
)

target_link_libraries(
    test_risk
    PRIVATE
    lob_engine
)
add_executable(
    test_lobster_reader

    test_lobster_reader.cpp
)

target_link_libraries(
    test_lobster_reader

    PRIVATE

    lob_engine
)

add_executable(
    test_lobster_replay

    test_lobster_replay.cpp
)

target_link_libraries(
    test_lobster_replay

    PRIVATE

    lob_engine
)
add_executable(
    test_lobster_validator

    test_lobster_validator.cpp
)
target_link_libraries(
    test_lobster_validator

    PRIVATE

    lob_engine
)

add_executable(
    test_lobster_validation
    test_lobster_validation.cpp
)

target_link_libraries(
    test_lobster_validation
    PRIVATE
    lob_engine
)

add_executable(
    test_lobster_step_validation
    test_lobster_step_validation.cpp
)

target_link_libraries(
    test_lobster_step_validation
    PRIVATE
    lob_engine
)