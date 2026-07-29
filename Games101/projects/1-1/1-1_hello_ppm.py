WIDTH = 512
HEIGHT = 512

framebuffer = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]

for y in range(HEIGHT):
    for x in range(WIDTH):
        framebuffer[y][x][0] = int(x / WIDTH * 255)  # R: 从左到右
        framebuffer[y][x][2] = int(y / HEIGHT * 255)  # B: 从上到下
        framebuffer[y][x][2] = 128  # G: 固定

with open("output_1-1.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer[y][x]
            f.write(f"{r} {g} {b}\n")
