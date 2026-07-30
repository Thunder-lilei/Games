#include <cmath>
#include <cstdio>   // fopen, fprintf, fclose（用于 PPM 输出）
#include <cstdlib>  // malloc, free（堆分配 framebuffer）
#include <string>   // __FILE__ 路径处理

// ========== 三维向量 Vec3 ==========
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

// ========== 光线 ==========
struct Ray {
    Vec3 origin;     // 光线起点
    Vec3 direction;  // 光线方向（通常已归一化）

    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
};

// ========== 球体 ==========
struct Sphere {
    Vec3 center;    // 球心位置
    float radius;   // 半径
    Vec3 color;     // 颜色 [R, G, B]，范围 0~255

    Sphere(const Vec3& c, float r, const Vec3& col)
        : center(c), radius(r), color(col) {}

    /**
     * 求光线与球体的交点
     * @param ray  入射光线
     * @param t    [输出] 交点到光线起点的距离（取最小的正 t）
     * @return true 表示有交点，false 表示无交点
     *
     * 推导：球体方程 |P - C|^2 = r^2，代入 P = O + t*D
     * 展开为二次方程 at^2 + bt + c = 0，判别式 b^2 - 4ac
     */
    bool intersect(const Ray& ray, float& t) const {
        Vec3 oc = ray.origin - center;          // 光线起点到球心的向量
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float disc = b * b - 4 * a * c;         // 判别式
        if (disc < 0) return false;             // 无实根 → 不相交

        float sqrt_disc = sqrt(disc);
        float t0 = (-b - sqrt_disc) / (2 * a);  // 第一个交点（通常是近的）
        float t1 = (-b + sqrt_disc) / (2 * a);  // 第二个交点（远的）

        // 取最小的正 t（光线只能向前传播）
        t = (t0 > 0) ? t0 : t1;
        return t > 0;
    }
};

// ========== 主函数 ==========
int main() {
    // ===== 渲染参数 =====
    const int WIDTH = 512;           // 图像宽度（像素）
    const int HEIGHT = 512;          // 图像高度（像素）
    const Vec3 camera_pos(0, 0, 3);  // 相机在 Z 轴正方向，看向原点
    const Vec3 light_dir(0, 0, 1);   // 光源方向（从球体指向光源，已归一化）

    // ===== 场景定义 =====
    // 一个球体：中心在原点，半径 0.8，红色
    Sphere sphere(Vec3(0, 0, 0), 0.8f, Vec3(255, 100, 100));

    // 创建 framebuffer：堆分配二维数组 [HEIGHT][WIDTH][3]
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
            // (x+0.5) 取像素中心，/WIDTH*2-1 映射到 [-1,1]
            float ndc_x = (x + 0.5f) / WIDTH * 2.0f - 1.0f;
            float ndc_y = (y + 0.5f) / HEIGHT * 2.0f - 1.0f;

            // 光线方向：从相机指向 NDC 上的点，再归一化
            // 相机看向 -Z 方向，所以 Z 分量为 -1
            Vec3 dir = (Vec3(ndc_x, ndc_y, -1.0f)).normalized();
            Ray ray(camera_pos, dir);

            // 对每条光线，遍历场景中所有物体，找最近的交点
            float t_hit = 0;
            if (sphere.intersect(ray, t_hit)) {
                // 计算交点 P 和法线 n
                Vec3 P = ray.origin + ray.direction * t_hit;  // 交点位置
                Vec3 n = (P - sphere.center).normalized();     // 球面法线

                // 漫反射着色：n·l，越正对光源越亮
                float diff = n.dot(light_dir);
                if (diff < 0) diff = 0;

                // 最终颜色 = 球体颜色 × 漫反射系数（保留到 [0, 255]）
                framebuffer[y][x][0] = sphere.color.x * diff;
                framebuffer[y][x][1] = sphere.color.y * diff;
                framebuffer[y][x][2] = sphere.color.z * diff;
            }
            // 若不相交，framebuffer 保持初始值 0（黑色），无需额外处理
        }
    }

    // ===== 输出 PPM =====
    // 用 __FILE__ 获取 main.cpp 所在目录，确保 ppm 总输出到 2-1_ray_sphere/ 根目录
    std::string cpp_path(__FILE__);
    std::string ppm_path = cpp_path.substr(0, cpp_path.find_last_of("/\\"))
                         + "/output_2-1.ppm";
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
