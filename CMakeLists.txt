cmake_minimum_required(VERSION 3.5)
project(TL2_STM)

set(CMAKE_CXX_STANDARD 17)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
# add_definitions(-g)

find_package( Boost 1.30 COMPONENTS program_options REQUIRED )

set(SRC_FILES main.cpp stm.cpp)
set(ALL_TEST_FILES tests.cpp stm.cpp)
set(BENCHMARK_FILES benchmark.cpp stm.cpp)
set(TX_TEST_FILES tx_test.cpp stm.cpp stm.cpp)

# Global settings
add_definitions(-Wall -Werror)
link_libraries(pthread)

add_executable(main ${SRC_FILES})
add_executable(stm_tests ${ALL_TEST_FILES})
add_executable(no_stm_tests ${ALL_TEST_FILES})
add_executable(tx_test ${TX_TEST_FILES})
add_executable(bench ${BENCHMARK_FILES})

target_compile_definitions(tx_test PUBLIC USE_STM)
target_compile_definitions(stm_tests PUBLIC USE_STM)
target_compile_definitions(bench PUBLIC USE_STM)
target_include_directories( bench PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries( bench ${Boost_LIBRARIES} )