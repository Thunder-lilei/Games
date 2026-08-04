cmake_minimum_required(VERSION 3.10)
project(Games101)

# 每个阶段目录通过 add_subdirectory 引入
# 新增阶段时，在此添加一行 add_subdirectory(<目录名>)
add_subdirectory(2-1_ray_sphere)
add_subdirectory(2-2_ray_triangle)
add_subdirectory(2-3_bvh)
add_subdirectory(2-4_path_tracing)
