WIDTH = 512
HEIGHT = 512

framebuffer = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]
depth_buffer = [[float('inf')] * WIDTH for _ in range(HEIGHT)]

# 三角形 1 — 红色，Z = 0.0（前）
TRI1 = [[100, 100, 0.6], [400, 50, 0.6], [200, 400, 0.6]]
# 三角形 2 — 蓝色，Z = 0.5（后）
TRI2 = [[50, 50, 0.5], [450, 80, 0.5], [250, 350, 0.5]]


def edge_function(p, a, b, c):
    """返回点 p 在三角形 abc 中的三条有符号边值"""
    e0 = (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0])
    e1 = (p[0] - b[0]) * (c[1] - b[1]) - (p[1] - b[1]) * (c[0] - b[0])
    e2 = (p[0] - c[0]) * (a[1] - c[1]) - (p[1] - c[1]) * (a[0] - c[0])
    return e0, e1, e2


COLOR_RED = [255, 0, 0]
COLOR_BLUE = [0, 0, 255]

triangles = [(TRI1, COLOR_RED), (TRI2, COLOR_BLUE)]

for tri, color in triangles:
    a, b, c = tri

    # 包围盒
    xmin = max(0, int(min(a[0], b[0], c[0])))
    xmax = min(WIDTH - 1, int(max(a[0], b[0], c[0])))
    ymin = max(0, int(min(a[1], b[1], c[1])))
    ymax = min(HEIGHT - 1, int(max(a[1], b[1], c[1])))

    # 重心坐标的分母：edge(A, B, C) = (A - B) × (C - B)
    denom = (a[0] - b[0]) * (c[1] - b[1]) - (a[1] - b[1]) * (c[0] - b[0])

    for y in range(ymin, ymax + 1):
        for x in range(xmin, xmax + 1):
            p = [x + 0.5, y + 0.5]
            e0, e1, e2 = edge_function(p, a, b, c)

            # 屏幕坐标 y 向下，逆时针三角形内部 edge 值 ≤ 0
            if e0 <= 0 and e1 <= 0 and e2 <= 0:
                # 重心坐标（三个负数相除得正数）
                alpha = e1 / denom
                beta = e2 / denom
                gamma = e0 / denom

                # 插值深度
                z = alpha * a[2] + beta * b[2] + gamma * c[2]

                # 深度测试
                if z < depth_buffer[y][x]:
                    depth_buffer[y][x] = z
                    framebuffer[y][x] = color

with open("output_1-3.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer[y][x]
            f.write(f"{r} {g} {b}\n")
