# 1-1 输出一张图片

**文件位置：** `1-1_hello_ppm.py`

**目标：** 创建帧缓冲，填充渐变色，输出为 PPM 文件。

---

## PPM 格式说明

PPM（Portable Pixmap）是一种纯文本图像格式，后缀 `.ppm`。它将每个像素的 RGB 值直接用数字写出，没有任何压缩或编码，所以不需要任何第三方库就能生成和查看。

```
P3
宽度 高度
最大颜色值
r g b
r g b
...
```

每一行三个数字代表一个像素的 R G B，从左到右、从上到下。

示例 2×2 图片：
```
P3
2 2
255
255 0 0    0 255 0
0 0 255    255 255 255
```

## 实现步骤

1. **创建 framebuffer**
   - 设定常量：`WIDTH = 512`，`HEIGHT = 512`
   - 创建一个 numpy 数组，形状为 `(HEIGHT, WIDTH, 3)`（3 代表 R、G、B 三个颜色通道），dtype 为 `int` 或 `uint8`
   - 初始值全为 0（黑色）
   - 如果用纯 Python 列表，可以用 `[[[0,0,0] for _ in range(WIDTH)] for _ in range(HEIGHT)]`

2. **填充渐变**
   - 遍历每个像素 `(x, y)`
   - 红色通道 = `x / WIDTH * 255`（从左到右从黑到红）
   - 蓝色通道 = `y / HEIGHT * 255`（从上到下从黑到蓝）
   - 绿色通道 = 128（固定值）
   - 每个通道的值四舍五入为整数，范围 [0, 255]

3. **写入 PPM 文件**
   - 文件路径：`output_1-1.ppm`
   - 第一行写 `P3`
   - 第二行写 `WIDTH HEIGHT`
   - 第三行写 `255`
   - 之后每行写一个像素的 `r g b`，像素顺序：先按行，每行内从左到右

4. **运行**
   - 打开终端，执行 `python 1-1_hello_ppm.py`

5. **验证结果**
   - **PyCharm（推荐）：** 直接点击 `output_1-1.ppm` 文件，PyCharm 内置图片查看器会自动显示
   - **VS Code：** 搜索安装 **"PPM/PNM Viewer"** 插件，安装后在文件列表点击即可预览
   - 或在线上传查看（搜索 "PPM viewer online"）
   - 或安装 ImageMagick 转 PNG：`winget install ImageMagick.ImageMagick`，然后 `magick output_1-1.ppm output_1-1.png`
   - 正确结果：左上角黑色 → 右上角红色 → 左下角蓝色 → 右下角紫红色（红+蓝）的渐变

## 验收标准
- [ ] 生成 `output_1-1.ppm` 文件
- [ ] 文件内容符合 PPM 格式
- [ ] 图片显示正确的红蓝渐变
