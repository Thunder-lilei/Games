cmake_minimum_required(VERSION 3.10)   # 要求 CMake 最低版本 3.10
project(RayBvh)                        # 项目名称
set(CMAKE_CXX_STANDARD 17)             # 使用 C++17 标准
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")   # MSVC 强制 UTF-8，兼容中文注释
add_compile_definitions("$<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>")  # MSVC 允许 fopen
add_executable(ray_bvh main.cpp)       # 编译 main.cpp，生成 ray_bvh.exe
