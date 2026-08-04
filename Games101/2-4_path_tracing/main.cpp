// ============================================================
// 2-4 路径追踪（Path Tracing）
// 蒙特卡洛路径追踪：逐像素发射多条光线，光线在场景中随机弹跳，
// 用采样近似渲染方程积分，渲染出带间接光照（颜色溢出）的画面。
// 场景：Cornell Box（红/蓝墙 + 白房间 + 天花板面光源 + 两个漫反射球）
// 输出：PPM 格式图片（P3 文本格式，无第三方依赖）
// ============================================================

#include <cmath>
#include <cstdio>     // fopen, fprintf, fclose（PPM 输出）
#include <cstdlib>    // malloc, free
#include <cstring>    // memset
#include <ctime>      // clock（耗时统计）
#include <algorithm>  // std::swap
#include <random>     // std::mt19937（随机数生成器）
#include <string>     // __FILE__ 路径处理
#include <vector>     // std::vector

const float PI = 3.14159265358979f;

// ========== 三维向量 Vec3（沿用 2-3，新增分量相乘） ==========
struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    // 按索引取分量（0=x, 1=y, 2=z）
    float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    float& operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
    // 逐分量相乘（用于 albedo × 入射辐射度 这类颜色乘法）
    Vec3 operator*(const Vec3& v) const { return Vec3(x * v.x, y * v.y, z * v.z); }

    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y,
                    z * v.x - x * v.z,
                    x * v.y - y * v.x);
    }
    float length() const { return sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        float l = length();
        return l > 0 ? *this / l : Vec3();
    }
};

// ========== 光线：P(t) = origin + t * direction, t >= 0 ==========
struct Ray {
    Vec3 origin;
    Vec3 direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
};

// ========== 随机数：全局 mt19937 + 均匀分布 [0,1) ==========
std::mt19937 g_rng(2026);  // 固定种子，保证每次运行结果可复现
std::uniform_real_distribution<float> g_uni(0.0f, 1.0f);

// 返回 [0,1) 的随机数
float random01() { return g_uni(g_rng); }

const float INF = 1e30f;  // 表示"无穷远"，用于 t 的初始值

// ========== 材质 ==========
struct Material {
    Vec3 albedo;    // 漫反射率（颜色），各分量范围 [0,1]
    Vec3 emitted;   // 自发光（仅光源用），非光源为 (0,0,0)

    Material(const Vec3& a = Vec3(), const Vec3& e = Vec3())
        : albedo(a), emitted(e) {}

    // 是否为光源（只要自发光任一分量 > 0）
    bool is_light() const {
        return emitted.x > 0 || emitted.y > 0 || emitted.z > 0;
    }
};

// ========== 球体（与 2-1 相同，去掉 color 字段，颜色交给材质） ==========
struct Sphere {
    Vec3 center;
    float radius;

    Sphere(const Vec3& c = Vec3(), float r = 0) : center(c), radius(r) {}

    // 解二次方程求最近交点 t（与 2-1 相同）
    bool intersect(const Ray& ray, float& t) const {
        Vec3 oc = ray.origin - center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float disc = b * b - 4 * a * c;
        if (disc < 0) return false;
        float sqrt_disc = sqrt(disc);
        float t0 = (-b - sqrt_disc) / (2 * a);
        float t1 = (-b + sqrt_disc) / (2 * a);
        t = (t0 > 0) ? t0 : t1;
        return t > 1e-4f;
    }

    // 交点处的法线（球心指向交点，单位化）
    Vec3 normal_at(const Vec3& P) const { return (P - center).normalized(); }
};

// ========== 三角形（MT 算法，与 2-2 相同，去掉 color 字段） ==========
struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 n;  // 面法线（构造时由顶点顺序叉乘得到）

    Triangle(const Vec3& a = Vec3(), const Vec3& b = Vec3(),
             const Vec3& c = Vec3())
        : v0(a), v1(b), v2(c) {
        n = (v1 - v0).cross(v2 - v0).normalized();
    }

    // Möller–Trumbore 算法求交（与 2-2 相同）
    bool intersect(const Ray& ray, float& t) const {
        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        Vec3 s = ray.origin - v0;
        Vec3 s1 = ray.direction.cross(e2);
        Vec3 s2 = s.cross(e1);
        float det = s1.dot(e1);
        if (fabs(det) < 1e-8f) return false;
        float inv_det = 1.0f / det;
        t = s2.dot(e2) * inv_det;
        float u = s1.dot(s) * inv_det;
        float v = s2.dot(ray.direction) * inv_det;
        return t > 1e-4f && u >= 0.0f && v >= 0.0f && u + v <= 1.0f;
    }

    // 交点处的法线（平面，直接返回面法线）
    Vec3 normal_at(const Vec3&) const { return n; }
};

// ========== 图元（球体或三角形 + 材质） ==========
enum PrimType { SPHERE, TRIANGLE };

struct Primitive {
    PrimType type;
    Material mat;   // 材质（决定颜色与是否发光）
    Sphere sphere;
    Triangle tri;

    // 静态工厂：创建球体图元
    static Primitive make_sphere(const Vec3& c, float r, const Material& m) {
        Primitive p;
        p.type = SPHERE;
        p.mat = m;
        p.sphere = Sphere(c, r);
        return p;
    }

    // 静态工厂：创建三角形图元
    static Primitive make_triangle(const Vec3& a, const Vec3& b,
                                   const Vec3& c, const Material& m) {
        Primitive p;
        p.type = TRIANGLE;
        p.mat = m;
        p.tri = Triangle(a, b, c);
        return p;
    }

    // 求交测试
    bool intersect(const Ray& ray, float& t) const {
        if (type == SPHERE) return sphere.intersect(ray, t);
        return tri.intersect(ray, t);
    }

    // 交点处的法线（着色用）
    Vec3 normal_at(const Vec3& P) const {
        if (type == SPHERE) return sphere.normal_at(P);
        return tri.n;
    }
};

// ========== 朴素遍历：每条光线测所有图元，取最近交点 ==========
// 场景很小（约 10 个图元），无需 BVH，直接遍历即可
bool naive_intersect(const std::vector<Primitive>& prims, const Ray& ray,
                     float& t_hit, int& hit_idx) {
    bool hit = false;
    for (int i = 0; i < (int)prims.size(); i++) {
        float t;
        if (prims[i].intersect(ray, t) && t < t_hit) {
            t_hit = t;
            hit_idx = i;
            hit = true;
        }
    }
    return hit;
}

// ========== 半球余弦加权采样 ==========
// 以法线 n 为局部坐标的 z 轴，在 n 一侧的半球面上采样随机方向 wi。
// 用球坐标：theta = acos(sqrt(r1)) 使得采样密度与 cos(theta) 成正比
// （余弦加权，与漫反射 BRDF 匹配，可显著降低噪声）。
// pdf(wi) = cos(theta) / PI
// @param n 交点处的单位法线
// @return  采样出的随机方向（单位向量，与 n 夹角 < 90°）
Vec3 cosine_sample_hemisphere(const Vec3& n) {
    float r1 = random01();
    float r2 = random01();

    // 球坐标采样（z 轴 = 法线方向）
    float theta = acos(sqrt(r1));            // 极角：0（法线）→ PI/2（切平面）
    float phi = 2.0f * PI * r2;              // 方位角：绕法线一圈

    // 在法线周围构造正交基 (tangent, bitangent, n)
    // 选一个与 n 不平行的轴做辅助轴，保证叉乘结果不为零向量
    Vec3 helper = (fabs(n.y) < 0.99f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 tangent = helper.cross(n).normalized();   // 切向
    Vec3 bitangent = n.cross(tangent);             // 副法向

    // 球坐标 → 局部笛卡尔坐标（x=sin·cos, y=sin·sin, z=cos）
    float sx = sin(theta) * cos(phi);
    float sy = sin(theta) * sin(phi);
    float sz = cos(theta);

    // 局部坐标 → 世界坐标
    return tangent * sx + bitangent * sy + n * sz;
}

// 渲染参数（想换分辨率 / 采样数 / 噪点水平时改这里）
const int WIDTH = 256;          // 图像宽度（像素）
const int HEIGHT = 256;         // 图像高度（像素）
const int SPP_LOW = 64;         // 对比采样：低（噪点多，快）
const int SPP_HIGH = 256;       // 对比采样：高（噪点少，慢 4 倍）
const int MAX_DEPTH = 12;       // 最大递归深度（配合 RR 防止无限递归）
const float SURVIVAL = 0.8f;    // 俄罗斯轮盘赌继续追踪的概率

// ========== 路径追踪（递归） ==========
// 渲染方程的蒙特卡洛估计：
//   L(P, wo) = Le(P) + ∫ fr(P, wi, wo) · Li(P, wi) · cosθ dωi
// 对半球方向 wi 做单点采样近似：
//   L ≈ Le + fr · Li · cosθ / pdf(wi)
// 漫反射 fr = albedo / PI，余弦加权采样 pdf = cosθ / PI，代入后
//   L ≈ Le + albedo · Li · PI
// 再除以俄罗斯轮盘赌的存活概率 P，保持无偏：
//   L ≈ Le + albedo · Li · PI / P
//
// @param prims 场景图元
// @param ray   当前光线
// @param depth 当前递归深度（0 为相机首发光线）
// @return      该光线方向的辐射度（未经色调映射）
Vec3 path_trace(const std::vector<Primitive>& prims, const Ray& ray, int depth) {
    // ① 求最近交点
    float t_hit = INF;
    int hit_idx = -1;
    if (!naive_intersect(prims, ray, t_hit, hit_idx)) {
        return Vec3();  // 未命中：背景为黑色
    }
    const Primitive& prim = prims[hit_idx];
    Vec3 P = ray.origin + ray.direction * t_hit;  // 交点
    Vec3 n = prim.normal_at(P);                   // 法线（路径追踪统一按外法线处理）

    // ② 命中光源：直接返回自发光（这束光就是光源贡献）
    if (prim.mat.is_light()) return prim.mat.emitted;

    // ③ 俄罗斯轮盘赌（Russian Roulette）：以概率 P 继续追踪，1-P 终止
    //    终止的路径贡献记作 0，继续的路径结果乘 1/P，保证期望无偏
    if (depth >= MAX_DEPTH) return Vec3();     // 超过最大深度，强制终止
    if (random01() > SURVIVAL) return Vec3();  // 以概率 1-P 随机终止

    // ④ 在法线半球上采样一个出射方向 wi，发射次级光线
    Vec3 wi = cosine_sample_hemisphere(n);
    Vec3 offset = P + n * 1e-3f;  // 沿法线偏移，避免与自己相交（浮点误差）
    Vec3 incoming = path_trace(prims, Ray(offset, wi), depth + 1);

    // ⑤ 渲染方程单点采样估计（推导见函数头注释）
    return prim.mat.emitted + prim.mat.albedo * incoming * (PI / SURVIVAL);
}

// ========== 渲染一帧并输出 PPM ==========
// 逐像素发射 spp 条光线取平均，gamma 校正后写入 PPM。
// 用同一个场景分别以 SPP_LOW / SPP_HIGH 各渲染一次，对比噪点随采样数下降的效果。
// @param prims        场景图元
// @param camera_pos   相机位置
// @param tan_half_fov 视场半角正切
// @param aspect       宽高比
// @param spp          每像素采样数
// @param ppm_path     PPM 输出路径
// @param tag          日志前缀（区分两次渲染）
void render(const std::vector<Primitive>& prims, const Vec3& camera_pos,
            float tan_half_fov, float aspect, int spp,
            const std::string& ppm_path, const char* tag) {
    // 累计缓冲：每像素累加 spp 条路径的辐射度，最后取平均
    float* acc = (float*)malloc(WIDTH * HEIGHT * 3 * sizeof(float));
    if (!acc) return;
    memset(acc, 0, WIDTH * HEIGHT * 3 * sizeof(float));

    clock_t t0 = clock();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            Vec3 color(0, 0, 0);   // 该像素的累计颜色

            for (int s = 0; s < spp; s++) {
                // 像素中心 (x+0.5, y+0.5) 归一化到 [-1, 1]（y 轴翻转：图像向下，世界向上）
                float ndc_x = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * tan_half_fov * aspect;
                float ndc_y = (2.0f * (y + 0.5f) / HEIGHT - 1.0f) * tan_half_fov;
                Vec3 dir = Vec3(ndc_x, -ndc_y, -1.0f).normalized();   // 相机朝向 -Z
                color = color + path_trace(prims, Ray(camera_pos, dir), 0);
            }

            color = color / (float)spp;   // 平均 → 收敛到真实辐射度
            acc[(y * WIDTH + x) * 3 + 0] = color.x;
            acc[(y * WIDTH + x) * 3 + 1] = color.y;
            acc[(y * WIDTH + x) * 3 + 2] = color.z;
        }
        // 每 25% 打印一次进度
        if ((y + 1) % (HEIGHT / 4) == 0) {
            printf("%s 渲染进度: %d%%\n", tag, (y + 1) * 100 / HEIGHT);
        }
    }
    clock_t t1 = clock();
    printf("%s SPP=%d 渲染完成, 耗时: %.3f s\n", tag, spp,
           (double)(t1 - t0) / CLOCKS_PER_SEC);

    // gamma 校正（人眼对暗部更敏感，输出前做 1/2.2 次幂提亮）后写 PPM
    FILE* f = fopen(ppm_path.c_str(), "w");
    if (!f) { free(acc); return; }
    fprintf(f, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            const float* c = &acc[(y * WIDTH + x) * 3];
            for (int k = 0; k < 3; k++) {
                float g = pow(c[k], 1.0f / 2.2f);
                int v = (int)(g * 255.0f);
                if (v < 0) v = 0;
                if (v > 255) v = 255;
                fprintf(f, "%d ", v);
            }
            fprintf(f, "\n");
        }
    }
    fclose(f);
    printf("%s 输出图片: %s\n", tag, ppm_path.c_str());
    free(acc);
}

// ========== 主函数 ==========
int main() {
    // ===== 1. 搭建场景（Cornell Box） =====
    std::vector<Primitive> prims;

    // 房间尺寸
    const float RX = 1.1f;   // 房间半宽（x 方向）
    const float RY = 1.1f;   // 房间半高（y 方向）
    const float BACK_Z = -2.0f;   // 后墙 z 坐标
    const float FRONT_Z = 0.0f;   // 前开口 z 坐标（相机从这里看向房间）

    // 材质
    Material m_white(Vec3(0.75f, 0.75f, 0.75f));                  // 墙 / 地板 / 天花板
    Material m_red(Vec3(0.65f, 0.05f, 0.05f));                    // 左墙（红）
    Material m_blue(Vec3(0.05f, 0.05f, 0.65f));                   // 右墙（蓝）
    Material m_red_sphere(Vec3(0.90f, 0.15f, 0.15f));             // 红色球
    Material m_blue_sphere(Vec3(0.15f, 0.15f, 0.90f));            // 蓝色球
    Material m_light(Vec3(), Vec3(8, 8, 8));                      // 面光源（白色，无反射率）

    // 房间中心参考点（用于判断法线是否朝向房间内侧）
    Vec3 room_center(0, 0, -1.0f);

    // 添加一个四边形（两个三角形），自动校正法线朝房间内侧
    // @param a,b,c,d 四边形的四个角点（顺序任意，自动修正绕序）
    auto add_inward_quad = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, const Material& m) {
        // 第一个三角形 (a,b,c)：法线不指向房间内侧就交换 b/c 翻转绕序
        Vec3 n1 = (b - a).cross(c - a);
        if (n1.dot(room_center - (a + b + c) / 3.0f) < 0) std::swap(b, c);
        prims.push_back(Primitive::make_triangle(a, b, c, m));

        // 第二个三角形 (a,c,d)：同样校正
        Vec3 n2 = (c - a).cross(d - a);
        if (n2.dot(room_center - (a + c + d) / 3.0f) < 0) std::swap(c, d);
        prims.push_back(Primitive::make_triangle(a, c, d, m));
    };

    // 后墙（白色）z = BACK_Z
    add_inward_quad(Vec3(-RX, -RY, BACK_Z), Vec3(RX, -RY, BACK_Z),
                    Vec3(RX, RY, BACK_Z), Vec3(-RX, RY, BACK_Z), m_white);
    // 地板（白色）y = -RY
    add_inward_quad(Vec3(-RX, -RY, BACK_Z), Vec3(RX, -RY, BACK_Z),
                    Vec3(RX, -RY, FRONT_Z), Vec3(-RX, -RY, FRONT_Z), m_white);
    // 天花板（白色）y = +RY
    add_inward_quad(Vec3(-RX, RY, BACK_Z), Vec3(RX, RY, BACK_Z),
                    Vec3(RX, RY, FRONT_Z), Vec3(-RX, RY, FRONT_Z), m_white);
    // 左墙（红色）x = -RX
    add_inward_quad(Vec3(-RX, -RY, BACK_Z), Vec3(-RX, RY, BACK_Z),
                    Vec3(-RX, RY, FRONT_Z), Vec3(-RX, -RY, FRONT_Z), m_red);
    // 右墙（蓝色）x = +RX
    add_inward_quad(Vec3(RX, -RY, BACK_Z), Vec3(RX, RY, BACK_Z),
                    Vec3(RX, RY, FRONT_Z), Vec3(RX, -RY, FRONT_Z), m_blue);

    // 天花板面光源：略低于天花板平面避免 z-fighting，法线朝下（向房间内发光）
    const float LIGHT_Y = RY - 0.01f;    // y = 1.09
    const float LIGHT_HW = 0.45f;        // 光源半宽
    add_inward_quad(Vec3(-LIGHT_HW, LIGHT_Y, -1.0f), Vec3(LIGHT_HW, LIGHT_Y, -1.0f),
                    Vec3(LIGHT_HW, LIGHT_Y, -0.6f), Vec3(-LIGHT_HW, LIGHT_Y, -0.6f),
                    m_light);

    // 两个漫反射小球（一红一蓝，放在地板上）
    // 红球放右半区（背景是蓝墙）、蓝球放左半区（背景是红墙），颜色对比强烈
    prims.push_back(Primitive::make_sphere(Vec3(0.55f, -0.70f, -1.15f), 0.40f, m_red_sphere));
    prims.push_back(Primitive::make_sphere(Vec3(-0.55f, -0.70f, -0.55f), 0.40f, m_blue_sphere));

    printf("场景图元数: %d\n", (int)prims.size());

    // ===== 2. 相机与光线生成参数 =====
    const Vec3 camera_pos(0, 0.2f, 1.8f);   // 相机位置（房间前方）
    const float FOV = 45.0f;                // 垂直视场角（度）
    const float tan_half_fov = tan(FOV * PI / 180.0f / 2.0f);  // 视场半角正切
    const float aspect = (float)WIDTH / (float)HEIGHT;          // 宽高比

    // ===== 3. 同一场景渲染两次（SPP 低/高对比） =====
    // 输出文件固定生成在 2-4_path_tracing/ 目录下（基于 __FILE__ 定位）
    std::string cpp_path(__FILE__);
    std::string out_dir = cpp_path.substr(0, cpp_path.find_last_of("/\\")) + "/";
    render(prims, camera_pos, tan_half_fov, aspect, SPP_LOW,
           out_dir + "output_2-4_spp64.ppm", "[SPP=64 ]");
    render(prims, camera_pos, tan_half_fov, aspect, SPP_HIGH,
           out_dir + "output_2-4_spp256.ppm", "[SPP=256]");

    return 0;
}
