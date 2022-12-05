cmake_minimum_required(VERSION 3.5)
project(TL2_STM)

set(CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
set(SRC_FILES main.cpp stm.cpp)
set(ALL_TEST_FILES tests.cpp stm.cpp)
set(BENCHMARK_FILES benchmark.cpp stm.cpp)
set(TX_TEST_FILES tx_test.cpp stm.cpp stm.cpp)

# Global settings
add_definitions(-Wall -Werror -g)
link_libraries(pthread)

add_executable(main ${SRC_FILES})
add_executable(stm_tests ${ALL_TEST_FILES})
add_executable(no_stm_tests ${ALL_TEST_FILES})
add_executable(tx_test ${TX_TEST_FILES})
add_executable(bench ${BENCHMARK_FILES})

target_compile_definitions(tx_test PUBLIC USE_STM)
target_compile_definitions(stm_tests PUBLIC USE_STM)
target_compile_definitions(bench PUBLIC USE_STM)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../my_tl2_lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../my_tl2_lib)
add_library(tl2 STATIC include/stm.hpp stm.cpp stamp_include/stm.h)
