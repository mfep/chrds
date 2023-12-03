cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

project(chrds CXX)
option(BUILD_TEST "Controls whether tests are built" OFF)

add_library(chrds INTERFACE)
target_include_directories(chrds INTERFACE include)
target_compile_features(chrds INTERFACE cxx_std_20)

if(BUILD_TEST)
    add_subdirectory(tests)
endif()
