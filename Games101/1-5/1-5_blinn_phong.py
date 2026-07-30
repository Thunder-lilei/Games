WIDTH = 512
HEIGHT = 512

# ===== 顶点定义 =====
# 格式: (screen_x, screen_y, world_x, world_y, world_z, nx, ny, nz)
P0 = (200, 50,   0.0, 0.5, 0.0,  0.0, 0.0, 1.0)
P1 = (50,  400,  0.0, 0.0, 0.0,  0.0, 0.0, 1.0)
P2 = (450, 350,  1.0, 0.0, 0.0,  0.0, 0.0, 1.0)

framebuffer = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]


def edge(p, a, b):
    """有符号边值：edge(P, A, B)"""
    return (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0])


# 重心坐标分母
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
        p_pos = (x + 0.5, y + 0.5)

        e0 = edge(p_pos, P0, P1)
        e1 = edge(p_pos, P1, P2)
        e2 = edge(p_pos, P2, P0)

        if e0 >= 0 and e1 >= 0 and e2 >= 0:
            alpha = e1 / denom
            beta = e2 / denom
            gamma = e0 / denom

            # 第一张：纯色，不加光照
            framebuffer[y][x] = [200, 100, 100]

with open("output_1-5.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer[y][x]
            f.write(f"{r} {g} {b}\n")

# ===== 第二张：加上光照 =====
framebuffer2 = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]

# 光照参数
light_pos = (0.5, 1.0, 1.0)         # 光源在世界空间中的位置（靠近三角形，产生明显明暗过渡）
camera_pos = (0.5, 0.0, 2.0)        # 摄像机在世界空间中的位置
light_intensity = 1.5                # 光源强度
ka = [0.1, 0.1, 0.1]                # 环境光反射系数（环境色）
kd = [1.0, 0.0, 0.0]                # 漫反射系数（物体漫反射颜色，红色）
ks = [1.0, 1.0, 1.0]                # 高光反射系数（高光颜色，白色）
p = 32                               # 高光指数，越大高光点越小越亮


def normalize(v):
    length = (v[0]**2 + v[1]**2 + v[2]**2)**0.5
    if length == 0:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)


def dot(a, b):
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]


for y in range(ymin, ymax + 1):
    for x in range(xmin, xmax + 1):
        p_pos = (x + 0.5, y + 0.5)

        e0 = edge(p_pos, P0, P1)
        e1 = edge(p_pos, P1, P2)
        e2 = edge(p_pos, P2, P0)

        if e0 >= 0 and e1 >= 0 and e2 >= 0:
            alpha = e1 / denom
            beta = e2 / denom
            gamma = e0 / denom

            # 插值世界坐标
            Px = alpha*P0[2] + beta*P1[2] + gamma*P2[2]
            Py = alpha*P0[3] + beta*P1[3] + gamma*P2[3]
            Pz = alpha*P0[4] + beta*P1[4] + gamma*P2[4]
            P = (Px, Py, Pz)

            # 插值法线
            nx = alpha*P0[5] + beta*P1[5] + gamma*P2[5]
            ny = alpha*P0[6] + beta*P1[6] + gamma*P2[6]
            nz = alpha*P0[7] + beta*P1[7] + gamma*P2[7]
            n = normalize((nx, ny, nz))

            # ==== 在这里计算光照 ====
            l = normalize((light_pos[0]-P[0], light_pos[1]-P[1], light_pos[2]-P[2]))
            v = normalize((camera_pos[0]-P[0], camera_pos[1]-P[1], camera_pos[2]-P[2]))
            h = normalize((l[0]+v[0], l[1]+v[1], l[2]+v[2]))

            diff = max(0.0, dot(n, l))
            spec = max(0.0, dot(n, h))**p

            r = int((ka[0] + kd[0]*diff + ks[0]*spec) * light_intensity * 255)
            g = int((ka[1] + kd[1]*diff + ks[1]*spec) * light_intensity * 255)
            b = int((ka[2] + kd[2]*diff + ks[2]*spec) * light_intensity * 255)

            framebuffer2[y][x] = [min(r, 255), min(g, 255), min(b, 255)]

with open("output_1-5_2.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer2[y][x]
            f.write(f"{r} {g} {b}\n")
