# 2-1 光线与球体求交

**文件位置：** `2-1_ray_sphere/`

**目标：** 从光栅化切换到光线追踪，用 C++ 实现光线与球体的求交，渲染一个带漫反射光照的 3D 球体。

## 预备知识

**光线追踪 vs 光栅化：** 光栅化是"遍历三角形判断像素"（从物体到屏幕），光线追踪是"遍历像素发射光线"（从屏幕到物体）。前者快但难以处理复杂光照，后者慢但真实感更强。

**光线表示：**
```
Ray(origin, direction)
P(t) = origin + t * direction,  t >= 0
```

**球体求交：** 代入光线方程到球体方程 `|P - C|^2 = r^2`，解二次方程取最小正 t。

## 新知识点

| 概念 | 说明 |
|------|------|
| **CMake** | `cmake -B build` 生成项目文件，`cmake --build build` 编译 |
| **CMakeLists.txt** | 4 行配置，告诉 CMake 项目名、C++ 标准、要编译的文件 |
| **Vec3 手写类** | 运算符重载（+、-、\*、/）、dot、cross、length、normalized |
| **光线求交** | 二次方程判别式，取最小正 t |
| **堆数组** | `malloc`/`free`（栈 ~3MB 放不下 512×512×3 的 float 数组） |
| **MSVC 编译** | 中文注释需 `/utf-8` 标志，`fopen` 需 `_CRT_SECURE_NO_WARNINGS` |

## 实现要点

1. **Vec3 类**：手写 3D 向量运算（点积、叉积、归一化）
2. **Ray 结构体**：origin + direction
3. **Sphere 结构体**：center + radius + color，带 `intersect()` 函数
4. **主循环**：
   - 像素坐标映射到 NDC [-1, 1]
   - 发射光线 → 球体求交 → 漫反射着色 → 写入 framebuffer

## 构建与运行

```cmd
cd 2-1_ray_sphere
cmake -B build
cmake --build build
.\build\Debug\ray_sphere.exe
```

## 验收标准

- [ ] 生成一个带光照的 3D 球体
- [ ] 球体表面有明暗变化，光源方向正确
- [ ] 球体边缘平滑（非锯齿可接受）
