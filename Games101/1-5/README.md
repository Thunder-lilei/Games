# 1-5 Blinn-Phong 着色

**文件位置：** `1-5_blinn_phong.py`

**目标：** 给三角形添加光照计算，输出有立体感的渲染结果。

## 预备知识

**齐次坐标：** 把三维向量 `(x, y, z)` 扩展到四维 `(x, y, z, w)`，用 4×4 矩阵统一处理平移、旋转、缩放。
- 3D 点 → `(x, y, z, 1.0)`
- 3D 向量（方向）→ `(x, y, z, 0.0)`
- 回到 3D：`(x, y, z, w)` → `(x/w, y/w, z/w)`（透视除法）

**MVP 变换：** Model → View → Projection → 屏幕坐标

## 核心概念

**法线：** 垂直于表面的单位向量，表示表面的"朝向"。例如 `(0, 0, 1)` 面向 Z 轴正方向。

**Blinn-Phong 模型：**
```
L = L_ambient + L_diffuse + L_specular
L_ambient = ka * Ia
L_diffuse = kd * I * max(0, n·l)
L_specular = ks * I * max(0, n·h)^p
h = normalize(l + v)
```

## 实现步骤

1. 定义光照参数（ka, kd, ks, p, light_pos, camera_pos）
2. 定义顶点：`(screen_x, screen_y, world_x, world_y, world_z, nx, ny, nz)`
3. 对每个三角形像素：插值世界坐标 P、法线 n → 归一化 → 计算光照 → 写入 framebuffer

## 验收标准
- [ ] 三角形表面有明暗过渡
- [ ] 高光点位置随光源移动而变化
- [ ] 环境光保证了背光面也不是纯黑
