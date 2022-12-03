cmake_minimum_required(VERSION 3.5)
project(TL2_STM)

set(CMAKE_CXX_STANDARD 17)

set(SRC_FILES main.cpp RBTree.cpp)
set(TREE_TEST_FILES rb_tree_tests.cpp RBTree.cpp)
set(BENCHMARK_FILES benchmark.cpp RBTree.cpp)
set(TX_TEST_FILES tx_test.cpp stm.cpp RBTree.cpp)

add_definitions(-Wall -Werror)

add_executable(main ${SRC_FILES})
add_executable(tree_test ${TREE_TEST_FILES})
add_executable(tx_test ${TX_TEST_FILES})
add_executable(bench ${BENCHMARK_FILES})

target_compile_definitions(tx_test PUBLIC USE_STM)