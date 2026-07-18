# ---------------------------------------
# Core Engine Library
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