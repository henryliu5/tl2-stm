cmake_minimum_required(VERSION 3.5)
project(TL2_STM)

set(CMAKE_CXX_STANDARD 17)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# add_definitions(-g)

add_subdirectory(unordered_dense)
link_libraries(unordered_dense::unordered_dense)
# target_link_libraries(your_project_name unordered_dense::unordered_dense)

find_package( Boost 1.30 COMPONENTS program_options REQUIRED )

set(SRC_FILES main.cpp stm.cpp)
set(ALL_TEST_FILES tests.cpp stm.cpp)
set(BENCHMARK_FILES benchmark.cpp stm.cpp)

# Global settings
add_definitions(-Wall -Werror)
link_libraries(pthread)

add_executable(main ${SRC_FILES})
add_executable(stm_tests ${ALL_TEST_FILES})
add_executable(no_stm_tests ${ALL_TEST_FILES})

# -------------------------- Test to check correctness --------------------------
target_compile_definitions(stm_tests PUBLIC USE_STM)
enable_testing()
add_test(NAME CorrectnessTest
         COMMAND stm_tests)

# -------------------------- Static lib for STAMP --------------------------

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../my_tl2_lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../my_tl2_lib)
add_library(tl2 STATIC include/stm.hpp stm.cpp my_tl2_lib/stm.h)
target_compile_definitions(tl2 PUBLIC USE_STM)

# -------------------------- Set up different benchmarks --------------------------
add_executable(bench ${BENCHMARK_FILES})
target_compile_definitions(bench PUBLIC USE_STM OPTIMISTIC_READ_ONLY)
target_include_directories( bench PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries( bench ${Boost_LIBRARIES} )

add_executable(compiled_ro_bench ${BENCHMARK_FILES})
target_compile_definitions(compiled_ro_bench PUBLIC USE_STM)
target_include_directories( compiled_ro_bench PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries( compiled_ro_bench ${Boost_LIBRARIES} )

# Benchmark using no read only
add_executable(no_ro_bench ${BENCHMARK_FILES})
target_compile_definitions(no_ro_bench PUBLIC USE_STM NO_RO_TX)
target_include_directories( no_ro_bench PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries( no_ro_bench ${Boost_LIBRARIES} )

# Benchmark using exponential backoff
add_executable(backoff_bench ${BENCHMARK_FILES})
target_compile_definitions(backoff_bench PUBLIC USE_STM USE_BACKOFF)
target_include_directories( backoff_bench PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries( backoff_bench ${Boost_LIBRARIES} )

# Benchmark using global mutex
add_executable(mutex_bench ${BENCHMARK_FILES})
target_compile_definitions(mutex_bench PUBLIC)
target_include_directories( mutex_bench PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries( mutex_bench ${Boost_LIBRARIES} )

# Benchmark using gcc __transaction_atomic (on my hardware this will be STM)
add_executable(gcc_bench benchmark_gcc.cpp stm.cpp)
target_compile_options(gcc_bench PUBLIC "-fgnu-tm")
target_include_directories( gcc_bench PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(gcc_bench ${Boost_LIBRARIES} "-fgnu-tm")
