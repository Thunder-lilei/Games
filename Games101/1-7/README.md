# 1-7 进阶：给立方体加上光照

**文件位置：** `1-7_cube_light.py`

**目标：** 在 1-6 的彩色立方体基础上加上 Blinn-Phong 光照，让立方体有立体感。

## 与 1-6 的区别

| 对比项 | 1-6 纯色 | 1-7 光照 |
|--------|----------|----------|
| 颜色来源 | 直接使用三角形面颜色 | Blinn-Phong 模型计算 |
| 法线 | 仅定义，未使用 | 用 Model 的旋转部分变换后用于光照 |
| 世界坐标 | 不需要 | 需要，用于光照计算 |
| 输出文件 | `output_1-6.ppm` | `output_1-7.ppm` |

## 实现要点

1. 复用 1-6 的代码（立方体定义、MVP 矩阵、Z-Buffer）
2. 新增 `to_world()` 函数：只应用 Model 矩阵，获取顶点在世界空间中的位置
3. 法线变换：用 Model 矩阵的 3×3 旋转部分乘法线 → 世界空间法线（纯旋转无需逆转置）
4. 新增光照参数：`light_pos`、`light_intensity`、`ka`、`ks`、`p`
5. 渲染循环中插值世界坐标 → 计算 Blinn-Phong → 写入 framebuffer

## 完整的光照公式

```
ambient  = ka * kd
diffuse  = kd * light_intensity * max(0, n·l)
specular = ks * light_intensity * (n·h)^p
final    = ambient + diffuse + specular
```

## 验收标准

- [ ] 立方体各面有明暗过渡，光源方向正确
- [ ] 能看到三个面（正面、顶面、右侧面）
- [ ] 与 `output_1-6.ppm` 对比，光照版本更有立体感
