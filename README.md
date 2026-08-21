# Games

GAMES101 课程学习——从光栅化到粒子系统的完整实现。

## 阶段列表

| 阶段 | 目录 | 内容 |
|------|------|------|
| 2-1 | [2-1_ray_sphere](Games101/2-1_ray_sphere/) | 光线与球体求交，最简光线追踪 |
| 2-2 | [2-2_ray_triangle](Games101/2-2_ray_triangle/) | 光线与三角形求交，Möller–Trumbore 算法 |
| 2-3 | [2-3_bvh](Games101/2-3_bvh/) | BVH 加速结构，层次包围盒 |
| 2-4 | [2-4_path_tracing](Games101/2-4_path_tracing/) | 蒙特卡洛路径追踪，渲染方程，间接光照 |
| 2-5 | [2-5_particle](Games101/2-5_particle/) | 粒子系统——发射器、生命周期、物理模拟、广告牌渲染 |

## 构建

使用 CMake 多项目管理，所有阶段统一构建：

```cmd
cd Games101
cmake -B build
cmake --build build --config Release
```

各阶段可执行文件生成在 `Games101/build/Release/` 下。
