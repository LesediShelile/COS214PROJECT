cmake_minimum_required(VERSION 3.10)
project(TIVIDY CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -pedantic)
endif()

# Work-item core: Composite, Prototype, Iterator (+ Decorator base)
option(TIVIDY_USE_STUBS "Build only the work-item core against stubs/ (until State, Strategy are merged)" ON)
if(TIVIDY_USE_STUBS)
  set(CORE_SRC
    src/WorkItem.cpp src/Task.cpp src/Stage.cpp src/SequentialStage.cpp
    src/ParallelStage.cpp src/TaskDecorator.cpp
    src/DepthFirstIterator.cpp src/AvailableItemsIterator.cpp
    stubs/TaskStates.cpp stubs/TaskMemento.cpp)
else()
  file(GLOB CORE_SRC ${CMAKE_SOURCE_DIR}/src/*.cpp)
endif()
add_library(tividy_core ${CORE_SRC})
target_include_directories(tividy_core PUBLIC include)
if(TIVIDY_USE_STUBS)
  target_include_directories(tividy_core PUBLIC stubs)
endif()

enable_testing()
add_executable(tividy_tests tests/test_work_items.cpp)
target_include_directories(tividy_tests PRIVATE tests/third_party)
target_link_libraries(tividy_tests tividy_core)
add_test(NAME tividy_tests COMMAND tividy_tests)

# Optional native desktop GUI. The core and tests remain buildable without Qt.
option(TIVIDY_BUILD_DESKTOP "Build the TIVIDY Qt desktop application when Qt 6 is available" ON)
if(TIVIDY_BUILD_DESKTOP)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  if(Qt6_FOUND)
    add_executable(tividy_desktop src/TIVIDYDesktop.cpp)
    target_link_libraries(tividy_desktop PRIVATE tividy_core Qt6::Widgets)
  else()
    message(STATUS "Qt 6 Widgets not found; skipping tividy_desktop target")
  endif()
endif()
