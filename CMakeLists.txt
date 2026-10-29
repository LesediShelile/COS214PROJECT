cmake_minimum_required(VERSION 3.10)
project(TIVIDY CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -pedantic)
endif()

# ---- Engine library: every .cpp in src/ except an optional main.cpp ----------
file(GLOB TIVIDY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER TIVIDY_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(tividy_core STATIC ${TIVIDY_SOURCES})
target_include_directories(tividy_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---- Application: built only once src/main.cpp exists ------------------------
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
  add_executable(tividy src/main.cpp)
  target_link_libraries(tividy tividy_core)
endif()

# ---- Tests (doctest) ----------------------------------------------------------
option(TIVIDY_BUILD_TESTS "Build the doctest suite" ON)
if(TIVIDY_BUILD_TESTS)
  enable_testing()
  file(GLOB TIVIDY_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
  add_executable(tividy_tests ${TIVIDY_TEST_SOURCES})
  target_include_directories(tividy_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/third_party)
  target_link_libraries(tividy_tests tividy_core)
  add_test(NAME tividy_tests COMMAND tividy_tests)
endif()