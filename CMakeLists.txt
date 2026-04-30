cmake_minimum_required(VERSION 3.10)

project(cpp_algorithms)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# include 头文件路径
include_directories(${PROJECT_SOURCE_DIR}/src)

# 收集所有源文件
file(GLOB_RECURSE SOURCES
    src/*.cpp
    tests/*.cpp
    utils/*.cpp
)

# 生成可执行文件
add_executable(main ${SOURCES})
