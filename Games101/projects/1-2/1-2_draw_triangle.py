WIDTH = 512
HEIGHT = 512

framebuffer = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]

# 三角形三个顶点位置
P0 = [200, 50]
P1 = [50, 400]
P2 = [450, 350]

def edge(pos: list[int, int]):
    """
    判断法（Edge Function）：
    给定三角形三个顶点 `A, B, C`（逆时针顺序），对任意点 `P`
    """
    if (pos[0] - P0[0]) * (P1[1] - P0[1]) - (pos[1] - P0[1]) * (P1[0] - P0[0]) < 0:
        return False
    if (pos[0] - P1[0]) * (P2[1] - P1[1]) - (pos[1] - P1[1]) * (P2[0] - P1[0]) < 0:
        return False
    if (pos[0] - P2[0]) * (P0[1] - P2[1]) - (pos[1] - P2[1]) * (P0[0] - P2[0]) < 0:
        return False
    return True

for y in range(HEIGHT):
    for x in range(WIDTH):
        # 三角形范围 红色
        if edge([x, y]):
            framebuffer[y][x] = [255, 0, 0]
        else:
            framebuffer[y][x] = [0, 0, 0]  # 全黑背景

with open("output_1-2.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer[y][x]
            f.write(f"{r} {g} {b}\n")
