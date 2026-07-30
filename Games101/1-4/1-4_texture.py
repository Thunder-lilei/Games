WIDTH = 512
HEIGHT = 512

# ===== 1. 加载棋盘纹理 =====
with open("checkerboard.ppm", "r") as f:
    fmt = f.readline().strip()  # P3
    tex_w, tex_h = map(int, f.readline().split())
    max_v = int(f.readline().strip())  # 255
    texture = [[[0, 0, 0] for _ in range(tex_w)] for _ in range(tex_h)]
    for y in range(tex_h):
        for x in range(tex_w):
            r, g, b = [int(v) for v in f.readline().split()]
            texture[y][x] = [r, g, b]

# ===== 2. 定义带 UV 的三角形 =====
# 顶点格式：(x, y, u, v)
P0 = (150, 50,  0.0, 0.0)
P1 = (50,  400, 1.0, 0.0)
P2 = (450, 350, 1.0, 1.0)

# ===== 3. 渲染 =====
framebuffer = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]


def edge(p, a, b):
    """有符号边值：edge(P, A, B)"""
    return (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0])


# 分母 = edge(A, B, C)
denom = (P0[0] - P1[0]) * (P2[1] - P1[1]) - (P0[1] - P1[1]) * (P2[0] - P1[0])

# 包围盒
xs = [P0[0], P1[0], P2[0]]
ys = [P0[1], P1[1], P2[1]]
xmin = max(0, int(min(xs)))
xmax = min(WIDTH - 1, int(max(xs)))
ymin = max(0, int(min(ys)))
ymax = min(HEIGHT - 1, int(max(ys)))

for y in range(ymin, ymax + 1):
    for x in range(xmin, xmax + 1):
        p = (x + 0.5, y + 0.5)

        e0 = edge(p, P0, P1)
        e1 = edge(p, P1, P2)
        e2 = edge(p, P2, P0)

        # 该三角形顶点在屏幕上为顺时针（signed_area < 0），内部 edge 值 >= 0
        if e0 >= 0 and e1 >= 0 and e2 >= 0:
            # 重心坐标
            alpha = e1 / denom
            beta  = e2 / denom
            gamma = e0 / denom

            # 插值 UV
            u = alpha * P0[2] + beta * P1[2] + gamma * P2[2]
            v = alpha * P0[3] + beta * P1[3] + gamma * P2[3]

            # 翻转 v（纹理底部对应图像顶部）
            v = 1.0 - v

            # 最近邻采样
            tx = int(u * (tex_w - 1))
            ty = int(v * (tex_h - 1))
            framebuffer[y][x] = texture[ty][tx]

# ===== 4. 输出 PPM =====
with open("output_1-4.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer[y][x]
            f.write(f"{r} {g} {b}\n")
