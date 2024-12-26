cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)
project(dsxx CXX)
set(PROJECT_DESCRIPTION "small dsa project for me to get a job")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER "/usr/bin/gcc")






add_library(dsxx STATIC src/hash_table/hashtable.hh)
set_target_properties(dsxx PROPERTIES LINKER_LANGUAGE CXX)

enable_testing()


find_package(Catch2 3 REQUIRED)

add_executable(dsxx_tests "${CMAKE_SOURCE_DIR}/tests/hashtable.cc"
)

target_link_libraries(dsxx_tests dsxx Catch2::Catch2WithMain)
target_include_directories(dsxx_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/)


add_executable(p "${CMAKE_SOURCE_DIR}/main.cc")
target_link_libraries(p dsxx)
target_include_directories(p PRIVATE ${CMAKE_SOURCE_DIR}/src/)







