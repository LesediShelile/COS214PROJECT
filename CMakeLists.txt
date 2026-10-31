
cmake_minimum_required(VERSION 3.16)

project(TIVIDY LANGUAGES CXX)

# --------------------------------------------------
# C++ Configuration
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic --coverage -O0 -g)
    add_link_options(--coverage)
endif()


# --------------------------------------------------
# Core Workflow Engine
# --------------------------------------------------

file(GLOB TIVIDY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# The core must not contain GUI implementation files.
list(FILTER TIVIDY_SOURCES EXCLUDE
    REGEX ".*/(main|TIVIDYDesktop)\\.cpp$"
)

add_library(tividy_core STATIC
    ${TIVIDY_SOURCES}
)

target_include_directories(tividy_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


# --------------------------------------------------
# Tests
# --------------------------------------------------

option(TIVIDY_BUILD_TESTS
    "Build TIVIDY tests"
    ON
)

if(TIVIDY_BUILD_TESTS)

    enable_testing()

    file(GLOB TIVIDY_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )

    if(TIVIDY_TEST_SOURCES)

        add_executable(tividy_tests
            ${TIVIDY_TEST_SOURCES}
        )

        target_include_directories(tividy_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/third_party"
        )

        target_link_libraries(tividy_tests PRIVATE
            tividy_core
        )

        add_test(
            NAME tividy_tests
            COMMAND tividy_tests
        )

    endif()

endif()


# --------------------------------------------------
# Optional Qt Desktop GUI
# --------------------------------------------------

option(TIVIDY_BUILD_DESKTOP
    "Build TIVIDY desktop GUI"
    OFF
)

if(TIVIDY_BUILD_DESKTOP)

    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    add_executable(tividy_desktop
        main.cpp
        include/ui/Desktop.h
    )

    target_include_directories(tividy_desktop PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )

    target_link_libraries(tividy_desktop PRIVATE
        tividy_core
        Qt6::Widgets
    )

endif()