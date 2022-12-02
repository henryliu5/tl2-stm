cmake_minimum_required(VERSION 3.5)
project(TL2_STM)

set(CMAKE_CXX_STANDARD 11)

set(RB_TREE Red)
set(SOURCE_FILES main.cpp RBTree.cpp)
set(TEST_FILES rb_tree_tests.cpp RBTree.cpp)
set(BENCHMARK_FILES benchmark.cpp RBTree.cpp)

add_executable(main ${SOURCE_FILES})
add_executable(test_tree ${TEST_FILES})
add_executable(bench ${BENCHMARK_FILES})