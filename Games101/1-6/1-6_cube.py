import math

WIDTH = 512
HEIGHT = 512

framebuffer = [[[0, 0, 0] for _ in range(WIDTH)] for _ in range(HEIGHT)]
depth_buffer = [[float('inf')] * WIDTH for _ in range(HEIGHT)]

# ========== 向量运算 ==========
def vec3_add(a: list, b: list):
    """返回 a + b（逐分量相加）"""
    return [a[0]+b[0], a[1]+b[1], a[2]+b[2]]

def vec3_sub(a: list, b: list):
    """返回 a - b（逐分量相减）"""
    return [a[0]-b[0], a[1]-b[1], a[2]-b[2]]

def dot(a: list, b: list):
    """向量点积 a·b"""
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

def cross(a: list, b: list):
    """向量叉积 a×b"""
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

def normalize(v: list):
    """返回单位向量 v/|v|，零向量返回自身"""
    l = math.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    return [v[0]/l, v[1]/l, v[2]/l] if l != 0 else [0, 0, 0]

# ========== 4×4 矩阵运算（行优先，16 个 float） ==========
def mat4_mul(A: list, B: list):
    """4×4 矩阵乘法 A × B（行优先）"""
    out = [0]*16
    for i in range(4):
        for j in range(4):
            s = 0
            for k in range(4):
                s += A[i*4+k] * B[k*4+j]
            out[i*4+j] = s
    return out

def mat4_vec4_mul(M: list, v: list):
    """4×4 矩阵 × 4D 向量"""
    return [M[0]*v[0]+M[1]*v[1]+M[2]*v[2]+M[3]*v[3],
            M[4]*v[0]+M[5]*v[1]+M[6]*v[2]+M[7]*v[3],
            M[8]*v[0]+M[9]*v[1]+M[10]*v[2]+M[11]*v[3],
            M[12]*v[0]+M[13]*v[1]+M[14]*v[2]+M[15]*v[3]]

# ========== MVP 矩阵构建 ==========
def rotate_y(deg: float):
    """绕 Y 轴旋转 deg 度的 4×4 Model 矩阵"""
    rad = math.radians(deg)
    c, s = math.cos(rad), math.sin(rad)
    return [c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1]

def rotate_x(deg: float):
    """绕 X 轴旋转 deg 度的 4×4 矩阵"""
    rad = math.radians(deg)
    c, s = math.cos(rad), math.sin(rad)
    return [1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1]

def look_at(eye: list, target: list, up: list):
    """从 eye 看向 target 的 4×4 View 矩阵"""
    f = normalize(vec3_sub(target, eye))        # 视线方向
    r = normalize(cross(f, up))                 # 右向量（右手坐标系）
    u = cross(r, f)                             # 重正交化的上向量
    return [r[0], r[1], r[2], -dot(r, eye),
            u[0], u[1], u[2], -dot(u, eye),
            -f[0], -f[1], -f[2], dot(f, eye),
            0, 0, 0, 1]

def perspective(fov_deg: float, aspect: float, n: float, f: float):
    """透视投影 4×4 矩阵，fov 为视角（度），aspect 为宽高比，n/f 为近远裁剪面"""
    rad = math.radians(fov_deg)
    t = n * math.tan(rad / 2)
    r = t * aspect
    return [n/r, 0, 0, 0,
            0, n/t, 0, 0,
            0, 0, (n+f)/(n-f), 2*n*f/(n-f),
            0, 0, -1, 0]

def viewport_mtx(width: int, height: int):
    """NDC → 屏幕坐标的视口变换矩阵"""
    return [width/2, 0, 0, width/2,
            0, -height/2, 0, height/2,
            0, 0, 1, 0,
            0, 0, 0, 1]

# ========== 立方体定义 ==========
# 8 个顶点，每条边 1 单位长度，中心在原点
# 顶点格式：(x, y, z)
V = {
    'A': (-0.5, -0.5,  0.5), 'B': ( 0.5, -0.5,  0.5),
    'C': (-0.5,  0.5,  0.5), 'D': ( 0.5,  0.5,  0.5),
    'E': (-0.5, -0.5, -0.5), 'F': ( 0.5, -0.5, -0.5),
    'G': (-0.5,  0.5, -0.5), 'H': ( 0.5,  0.5, -0.5),
}
# 每个三角形: (v0, v1, v2, 颜色[r,g,b], 法线[nx,ny,nz])
triangles = [
    # 前面 Z+ (蓝)
    (V['A'], V['B'], V['C'], [128, 128, 255], (0, 0, 1)),
    (V['B'], V['D'], V['C'], [128, 128, 255], (0, 0, 1)),
    # 右面 X+ (红)
    (V['B'], V['F'], V['D'], [255, 128, 128], (1, 0, 0)),
    (V['F'], V['H'], V['D'], [255, 128, 128], (1, 0, 0)),
    # 后面 Z- (绿)
    (V['F'], V['E'], V['H'], [128, 255, 128], (0, 0, -1)),
    (V['E'], V['G'], V['H'], [128, 255, 128], (0, 0, -1)),
    # 左面 X- (黄)
    (V['E'], V['A'], V['G'], [255, 255, 128], (-1, 0, 0)),
    (V['A'], V['C'], V['G'], [255, 255, 128], (-1, 0, 0)),
    # 顶面 Y+ (紫)
    (V['C'], V['D'], V['G'], [255, 128, 255], (0, 1, 0)),
    (V['D'], V['H'], V['G'], [255, 128, 255], (0, 1, 0)),
    # 底面 Y- (橙)
    (V['A'], V['E'], V['B'], [255, 200, 128], (0, -1, 0)),
    (V['E'], V['F'], V['B'], [255, 200, 128], (0, -1, 0)),
]

# ========== 构建 MVP 矩阵 ==========
camera_pos = [1.0, 1.0, 1.0]        # 摄像机位置（与 View 矩阵中的 eye 一致）
model = mat4_mul(rotate_y(20), rotate_x(-15))     # Model：先绕 X 轴转 -15°，再绕 Y 轴转 20°，让三个面对准相机
view = look_at(camera_pos, [0, 0, 0], [0, 1, 0]) # View：从 (1,1,1) 看向原点
proj = perspective(45, WIDTH/HEIGHT, 0.1, 10)    # Projection：45° 视角，宽高比 1:1
vp = viewport_mtx(WIDTH, HEIGHT)                  # 视口变换：NDC → 512×512 像素
mvp = mat4_mul(proj, mat4_mul(view, model))       # P × V × M 合并
mvpv = mat4_mul(vp, mvp)                          # Viewport × P × V × M 合并

# ========== 渲染 ==========
for tri in triangles:
    v0, v1, v2, color, normal = tri

    def to_screen(v: tuple):
        """将物体空间顶点通过 MVP + 视口变换，返回屏幕坐标 (sx, sy, sz)"""
        h = mat4_vec4_mul(mvpv, [v[0], v[1], v[2], 1.0])
        return [h[0] / h[3], h[1] / h[3], h[2] / h[3]]

    # 三个顶点的屏幕坐标
    s0, s1, s2 = to_screen(v0), to_screen(v1), to_screen(v2)

    # 包围盒：三角形在屏幕上的最小矩形范围，只遍历这些像素
    xmin = max(0, int(min(s0[0], s1[0], s2[0])))
    xmax = min(WIDTH - 1, int(max(s0[0], s1[0], s2[0])))
    ymin = max(0, int(min(s0[1], s1[1], s2[1])))
    ymax = min(HEIGHT - 1, int(max(s0[1], s1[1], s2[1])))

    def edge_func(p: list, a: list, b: list):
        """边缘函数 edge(P, A, B) = (P-A) × (B-A) 的 z 分量"""
        return (p[0]-a[0])*(b[1]-a[1]) - (p[1]-a[1])*(b[0]-a[0])

    # 三角形有符号面积的 2 倍
    # 注：area 是以 s0 为参考点计算的 (s1-s0)×(s2-s0)
    # 而 edge(s0,s1,s2) 是以 s1 为参考点的 (s0-s1)×(s2-s1)，值 = -area
    # 所以正确的重心中分母是 abs(area)
    signed_area2 = (s1[0]-s0[0])*(s2[1]-s0[1]) - (s1[1]-s0[1])*(s2[0]-s0[0])
    if abs(signed_area2) < 1e-12:
        continue  # 退化三角形跳过
    abs_area2 = abs(signed_area2)

    for y in range(ymin, ymax + 1):
        for x in range(xmin, xmax + 1):
            p = [x + 0.5, y + 0.5]  # 像素中心
            e0 = edge_func(p, s0, s1)  # edge(P, A, B)
            e1 = edge_func(p, s1, s2)  # edge(P, B, C)
            e2 = edge_func(p, s2, s0)  # edge(P, C, A)

            # 三角形内部：三个 edge 值同号（全 ≥ 0 或全 ≤ 0）
            if e0 >= 0 and e1 >= 0 and e2 >= 0:
                alpha = e1 / abs_area2
                beta  = e2 / abs_area2
            elif e0 <= 0 and e1 <= 0 and e2 <= 0:
                alpha = -e1 / abs_area2
                beta  = -e2 / abs_area2
            else:
                continue
            gamma = 1 - alpha - beta

            # Z-Buffer 测试
            z = alpha * s0[2] + beta * s1[2] + gamma * s2[2]
            if z >= depth_buffer[y][x]:
                continue
            depth_buffer[y][x] = z

            # 直接使用三角形面颜色（纯色，无光照）
            framebuffer[y][x] = [max(0, min(255, c)) for c in color]

# ========== 输出 PPM ==========
with open("output_1-6.ppm", "w") as f:
    f.write("P3\n")
    f.write(f"{WIDTH} {HEIGHT}\n")
    f.write("255\n")
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = framebuffer[y][x]
            f.write(f"{r} {g} {b}\n")
