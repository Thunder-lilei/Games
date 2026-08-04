#include <cmath>
#include <cstdio>   // fopen, fprintf, fclose（用于 PPM 输出）
#include <cstdlib>  // malloc, free（堆分配 framebuffer）
#include <string>   // __FILE__ 路径处理

// ========== 三维向量 Vec3（与 2-1 相同） ==========
struct Vec3 {
    float x, y, z;

    // 构造函数，默认值为 0
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    // 向量加法：a + b
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }

    // 向量减法：a - b
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }

    // 数乘：a * s
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }

    // 数除：a / s
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }

    // 点积 a·b = ax*bx + ay*by + az*bz
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    // 叉积 a×b，结果垂直于 a 和 b
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y,
                    z * v.x - x * v.z,
                    x * v.y - y * v.x);
    }

    // 向量长度（模）
    float length() const { return sqrt(x * x + y * y + z * z); }

    // 返回单位向量（归一化），零向量返回 (0,0,0)
    Vec3 normalized() const {
        float l = length();
        return l > 0 ? *this / l : Vec3();
    }
};

// ========== 光线（与 2-1 相同） ==========
struct Ray {
    Vec3 origin;     // 光线起点
    Vec3 direction;  // 光线方向（通常已归一化）

    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
};

// ========== 三角形 ==========
struct Triangle {
    Vec3 v0, v1, v2;   // 三个顶点
    Vec3 n0, n1, n2;   // 三个顶点的法线（平面三角形内都等于面法线）
    Vec3 color;        // 颜色 [R, G, B]，范围 0~255

    /**
     * 构造函数
     * @param centroid 物体质心，用于修正面法线的朝向（始终指向外侧）
     */
    Triangle(const Vec3& a, const Vec3& b, const Vec3& c,
             const Vec3& col, const Vec3& centroid)
        : v0(a), v1(b), v2(c), color(col) {
        // 面法线 = (B-A)×(C-A)
        Vec3 n = (b - a).cross(c - a).normalized();
        // 若法线指向物体内侧（与"面中心指向质心"的方向相反），翻转
        Vec3 face_center = (a + b + c) / 3.0f;
        if (n.dot(face_center - centroid) < 0) n = n * -1.0f;
        n0 = n1 = n2 = n;  // 平面三角形三顶点法线相同；平滑网格时各顶点法线可不同
    }

    /**
     * Möller–Trumbore 算法：求光线与三角形交点
     * @param ray 入射光线
     * @param t   [输出] 交点到光线起点的距离
     * @param u   [输出] 重心坐标 u（顶点 v1 的权重）
     * @param v   [输出] 重心坐标 v（顶点 v2 的权重）
     * @return true 表示相交
     *
     * 原理：交点满足 O + t*D = (1-u-v)*v0 + u*v1 + v*v2
     * 移项整理成矩阵方程 [-D, E1, E2] * [t, u, v]^T = S（S = O - v0），
     * 用克莱姆法则解出 t、u、v。
     */
    bool intersect(const Ray& ray, float& t, float& u, float& v) const {
        Vec3 e1 = v1 - v0;             // E1 = B - A
        Vec3 e2 = v2 - v0;             // E2 = C - A
        Vec3 s  = ray.origin - v0;     // S  = O - A
        Vec3 s1 = ray.direction.cross(e2);  // S1 = D × E2
        Vec3 s2 = s.cross(e1);              // S2 = S × E1

        float det = s1.dot(e1);        // 行列式（接近 0 说明光线平行于三角形）
        if (fabs(det) < 1e-8f) return false;

        float inv_det = 1.0f / det;
        t = s2.dot(e2) * inv_det;      // 交点距离
        u = s1.dot(s)  * inv_det;      // 重心坐标 u
        v = s2.dot(ray.direction) * inv_det;  // 重心坐标 v

        // 交点条件：t > 0（光线向前）、u >= 0、v >= 0、u + v <= 1（点在三角形内）
        return t > 1e-4f && u >= 0.0f && v >= 0.0f && u + v <= 1.0f;
    }
};

// ========== 主函数 ==========
int main() {
    // ===== 渲染参数 =====
    const int WIDTH = 512;           // 图像宽度（像素）
    const int HEIGHT = 512;          // 图像高度（像素）
    const Vec3 camera_pos(0, 0, 3);  // 相机在 Z 轴正方向，看向原点
    const Vec3 light_pos(0.8f, 1.5f, 2.5f);  // 点光源位置（用于 Blinn-Phong 着色）

    // ===== 场景定义：四面体 =====
    // 绕 X 轴旋转 90° 的辅助函数（x 不变，y → -z，z → y）
    auto rotateX90 = [](const Vec3& v) { return Vec3(v.x, -v.z, v.y); };

    // 原始四个顶点（A 在正上方，B/C 在后方，D 在前方）
    Vec3 A(0, 0.5f, 0);
    Vec3 B(-0.5f, -0.5f, 0.5f);
    Vec3 C(0.5f, -0.5f, 0.5f);
    Vec3 D(0, -0.5f, -0.5f);

    // 绕 X 轴旋转 90°：顶点 A 从"正上方"转到"朝向相机（+Z）"，
    // 这样相机能同时看到汇聚在 A 的三个面（红/绿/蓝），而底面 BCD 藏在背面
    A = rotateX90(A);
    B = rotateX90(B);
    C = rotateX90(C);
    D = rotateX90(D);
    // 质心（用于让每个面法线朝外）
    const Vec3 centroid = (A + B + C + D) / 4.0f;

    // 四个三角形面，每个面一种颜色
    Triangle tris[4] = {
        Triangle(A, B, C, Vec3(230,  90,  90), centroid),  // 面 ABC：红
        Triangle(A, B, D, Vec3( 90, 200,  90), centroid),  // 面 ABD：绿
        Triangle(A, C, D, Vec3( 90, 120, 220), centroid),  // 面 ACD：蓝
        Triangle(B, C, D, Vec3(230, 200,  90), centroid),  // 面 BCD：黄（背对相机）
    };
    const int NUM_TRIS = 4;

    // ===== 创建 framebuffer：堆分配二维数组 [HEIGHT][WIDTH][3] =====
    // 栈放不下 (~3MB)，用堆分配
    float (*framebuffer)[WIDTH][3] = (float (*)[WIDTH][3])malloc(
        HEIGHT * sizeof(*framebuffer));
    if (!framebuffer) return 1;
    // 初始化为 0
    for (int i = 0; i < HEIGHT; i++)
        for (int j = 0; j < WIDTH; j++)
            for (int k = 0; k < 3; k++)
                framebuffer[i][j][k] = 0.0f;

    // ===== 主循环：逐像素 =====
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // 像素中心坐标映射到 NDC [-1, 1]
            float ndc_x = (x + 0.5f) / WIDTH * 2.0f - 1.0f;
            float ndc_y = (y + 0.5f) / HEIGHT * 2.0f - 1.0f;

            // 光线方向：从相机指向 NDC 上的点，再归一化
            Vec3 dir = (Vec3(ndc_x, ndc_y, -1.0f)).normalized();
            Ray ray(camera_pos, dir);

            // 对每条光线，遍历所有三角形，记录最近的交点（深度测试：取最小正 t）
            float t_hit = 0;
            float u_hit = 0, v_hit = 0;
            const Triangle* hit_tri = nullptr;
            for (int i = 0; i < NUM_TRIS; i++) {
                float t, u, v;
                if (tris[i].intersect(ray, t, u, v)) {
                    if (hit_tri == nullptr || t < t_hit) {
                        t_hit = t;
                        u_hit = u;
                        v_hit = v;
                        hit_tri = &tris[i];
                    }
                }
            }

            if (hit_tri) {
                // 交点位置 P
                Vec3 P = ray.origin + ray.direction * t_hit;

                // 用重心坐标插值法线（平面三角形中三个顶点法线相同，插值结果 = 面法线）
                Vec3 n = (hit_tri->n0 * (1.0f - u_hit - v_hit)
                        + hit_tri->n1 * u_hit
                        + hit_tri->n2 * v_hit).normalized();

                // ===== Blinn-Phong 着色 =====
                Vec3 light_dir = (light_pos - P).normalized();   // 表面 → 光源
                Vec3 view_dir  = (camera_pos - P).normalized();  // 表面 → 视线
                Vec3 half_vec  = (light_dir + view_dir).normalized();  // 半程向量

                // ① 环境光：物体颜色的小比例作为底色
                float ka = 0.1f;

                // ② 漫反射：n·l，越正对光源越亮
                float ndotl = n.dot(light_dir);
                if (ndotl < 0) ndotl = 0;

                // ③ 高光：n·h 的 p 次方，p 越大高光点越小越亮
                float ndoth = n.dot(half_vec);
                if (ndoth < 0) ndoth = 0;
                float spec = pow(ndoth, 32.0f);   // p = 32（中等光滑）
                // 背对光源时不应有高光
                float spec_mult = (ndotl > 0) ? 1.0f : 0.0f;

                Vec3 final = hit_tri->color * (ka + ndotl)
                           + Vec3(255, 255, 255) * spec * spec_mult;
                framebuffer[y][x][0] = final.x;
                framebuffer[y][x][1] = final.y;
                framebuffer[y][x][2] = final.z;
            }
            // 若不相交，framebuffer 保持初始值 0（黑色），无需额外处理
        }
    }

    // ===== 输出 PPM =====
    // 用 __FILE__ 获取 main.cpp 所在目录，确保 ppm 总输出到 2-2_ray_triangle/ 根目录
    std::string cpp_path(__FILE__);
    std::string ppm_path = cpp_path.substr(0, cpp_path.find_last_of("/\\"))
                         + "/output_2-2.ppm";
    FILE* f = fopen(ppm_path.c_str(), "w");
    if (!f) return 1;

    fprintf(f, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // 转 int 并裁切到 [0, 255]
            int r = (int)framebuffer[y][x][0]; if (r < 0) r = 0; if (r > 255) r = 255;
            int g = (int)framebuffer[y][x][1]; if (g < 0) g = 0; if (g > 255) g = 255;
            int b = (int)framebuffer[y][x][2]; if (b < 0) b = 0; if (b > 255) b = 255;
            fprintf(f, "%d %d %d\n", r, g, b);
        }
    }
    fclose(f);

    // 释放堆内存
    free(framebuffer);

    return 0;
}
