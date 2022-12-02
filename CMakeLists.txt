cmake_minimum_required(VERSION 3.5)
project(TL2_STM)

set(CMAKE_CXX_STANDARD 11)

set(RB_TREE Red)
set(SOURCE_FILES main.cpp RedBlackTree.cpp)
add_executable(test_tree ${SOURCE_FILES})