#include <cmath>
#include <cstdio>   // fopen, fprintf, fclose（用于 PPM 输出）
#include <cstdlib>  // malloc, free（堆分配 framebuffer）
#include <cstring>  // memset
#include <ctime>    // clock（渲染耗时统计）
#include <algorithm>// std::sort
#include <string>   // __FILE__ 路径处理
#include <vector>   // std::vector

const float PI = 3.14159265358979f;

// ========== 三维向量 Vec3 ==========
struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    // 按索引取分量（0=x, 1=y, 2=z），AABB 求交的逐分量循环用
    float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    float& operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }

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

// ========== 光线 ==========
struct Ray {
    Vec3 origin;
    Vec3 direction;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d) {}
};

const float INF = 1e30f;  // 表示"无穷远"，用于 t 的初始值

// ========== 求交测试计数器（全局） ==========
// 图元求交次数（Sphere/Triangle 的 intersect）
long long g_prim_count = 0;
// AABB 求交次数（BVH 遍历时对每个节点的包围盒测试）
long long g_aabb_count = 0;

// ========== AABB 包围盒 ==========
struct AABB {
    Vec3 mn, mx;  // 最小角点 / 最大角点

    // 空盒：min 取极大、max 取极小，方便后续 expand 合并
    AABB() : mn(INF, INF, INF), mx(-INF, -INF, -INF) {}

    // 把点 p 并进包围盒
    void expand(const Vec3& p) {
        mn.x = fmin(mn.x, p.x); mn.y = fmin(mn.y, p.y); mn.z = fmin(mn.z, p.z);
        mx.x = fmax(mx.x, p.x); mx.y = fmax(mx.y, p.y); mx.z = fmax(mx.z, p.z);
    }

    // 把另一个包围盒并进来（构建时向上合并用）
    void expand(const AABB& b) { expand(b.mn); expand(b.mx); }

    /**
     * 光线与 AABB 求交（slab 方法）
     * 原理：把盒子看成三对平行平面（slab），光线与每对平面的交点区间取交集。
     * 若三个方向的区间有公共部分，则光线穿过盒子。
     */
    bool intersect(const Ray& ray, float& t_enter, float& t_exit) const {
        float tmin = -INF, tmax = INF;
        for (int i = 0; i < 3; i++) {
            float o = ray.origin[i];
            float d = ray.direction[i];
            if (fabs(d) < 1e-12f) {
                // 光线与该轴平行：原点必须落在盒子范围内，否则永不相交
                if (o < mn[i] || o > mx[i]) return false;
                continue;
            }
            float t1 = (mn[i] - o) / d;   // 进入该 slab 的时刻
            float t2 = (mx[i] - o) / d;   // 离开该 slab 的时刻
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;  // 三个区间没有公共交集
        }
        t_enter = tmin;
        t_exit = tmax;
        return t_exit > 0;  // 交点必须在光线前方
    }
};

// ========== 球体（与 2-1 相同） ==========
struct Sphere {
    Vec3 center;
    float radius;
    Vec3 color;

    Sphere(const Vec3& c = Vec3(), float r = 0, const Vec3& col = Vec3())
        : center(c), radius(r), color(col) {}

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

    Vec3 normal_at(const Vec3& P) const { return (P - center).normalized(); }
};

// ========== 三角形（MT 算法，与 2-2 相同） ==========
struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 color;
    Vec3 n;  // 面法线

    Triangle(const Vec3& a = Vec3(), const Vec3& b = Vec3(),
             const Vec3& c = Vec3(), const Vec3& col = Vec3())
        : v0(a), v1(b), v2(c), color(col) {
        n = (v1 - v0).cross(v2 - v0).normalized();
    }

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

    Vec3 normal_at(const Vec3&) const { return n; }
};

// ========== 图元（球体或三角形的统一封装） ==========
enum PrimType { SPHERE, TRIANGLE };

struct Primitive {
    PrimType type;
    Vec3 color;      // 图元颜色
    Sphere sphere;
    Triangle tri;
    AABB box;      // 该图元的包围盒（构建 BVH 时用）

    static Primitive make_sphere(const Vec3& c, float r, const Vec3& col) {
        Primitive p;
        p.type = SPHERE;
        p.color = col;
        p.sphere = Sphere(c, r, col);
        p.box = AABB();
        p.box.expand(c - Vec3(r, r, r));
        p.box.expand(c + Vec3(r, r, r));
        return p;
    }

    static Primitive make_triangle(const Vec3& a, const Vec3& b,
                                   const Vec3& c, const Vec3& col) {
        Primitive p;
        p.type = TRIANGLE;
        p.color = col;
        p.tri = Triangle(a, b, c, col);
        p.box = AABB();
        p.box.expand(a); p.box.expand(b); p.box.expand(c);
        return p;
    }

    // 求交测试（计数点：所有图元求交都经过这里）
    bool intersect(const Ray& ray, float& t) const {
        g_prim_count++;
        if (type == SPHERE) return sphere.intersect(ray, t);
        return tri.intersect(ray, t);
    }

    // 图元中心（BVH 构建时选分割轴、排序用）
    Vec3 center() const {
        if (type == SPHERE) return sphere.center;
        return (tri.v0 + tri.v1 + tri.v2) / 3.0f;
    }

    // 交点处的法线（着色用）
    Vec3 normal_at(const Vec3& P) const {
        if (type == SPHERE) return sphere.normal_at(P);
        return tri.n;
    }
};

// ========== BVH：包围体层次结构 ==========
const int LEAF_SIZE = 2;  // 叶子节点最多容纳的图元数

struct BVHNode {
    AABB box;         // 包住该节点下所有图元的包围盒
    int left, right;  // 子节点索引（-1 表示无；内部节点才有）
    int first_prim;   // 叶子节点：图元区间起点（指向索引数组 idx）
    int n_prims;      // 叶子节点：图元数量（>0 时是叶子）
};

struct BVH {
    std::vector<BVHNode> nodes;  // 节点池（用索引代替指针，避免手动管理内存）
    std::vector<int> idx;        // 图元索引数组：构建时重排，叶子区间连续
    int root;                    // 根节点索引

    void build(const std::vector<Primitive>& prims) {
        nodes.clear();
        idx.resize(prims.size());
        for (int i = 0; i < (int)prims.size(); i++) idx[i] = i;
        root = build_rec(prims, 0, (int)prims.size());
    }

    // 递归构建 [start, end) 区间的图元，返回节点索引
    int build_rec(const std::vector<Primitive>& prims, int start, int end) {
        BVHNode node;
        // 当前区间的包围盒 = 所有图元包围盒的并集
        node.box = AABB();
        for (int i = start; i < end; i++)
            node.box.expand(prims[idx[i]].box);

        // 图元数 ≤ 阈值 → 叶子节点
        if (end - start <= LEAF_SIZE) {
            node.left = node.right = -1;
            node.first_prim = start;
            node.n_prims = end - start;
            nodes.push_back(node);
            return (int)nodes.size() - 1;
        }

        // ① 选分割轴：图元中心跨度最大的轴
        Vec3 cmin(INF, INF, INF), cmax(-INF, -INF, -INF);
        for (int i = start; i < end; i++) {
            Vec3 c = prims[idx[i]].center();
            for (int k = 0; k < 3; k++) {
                cmin[k] = fmin(cmin[k], c[k]);
                cmax[k] = fmax(cmax[k], c[k]);
            }
        }
        float span[3] = { cmax[0] - cmin[0], cmax[1] - cmin[1], cmax[2] - cmin[2] };
        int axis = (span[0] >= span[1] && span[0] >= span[2]) ? 0 :
                   (span[1] >= span[2]) ? 1 : 2;

        // ② 按该轴的中心坐标排序（只重排索引数组）
        std::sort(idx.begin() + start, idx.begin() + end,
                  [&](int a, int b) {
                      return prims[a].center()[axis] < prims[b].center()[axis];
                  });

        // ③ 中位数对半切，递归构建左右子树
        int mid = (start + end) / 2;
        node.left = build_rec(prims, start, mid);
        node.right = build_rec(prims, mid, end);
        node.first_prim = node.n_prims = 0;  // 内部节点不存图元
        nodes.push_back(node);
        return (int)nodes.size() - 1;
    }

    /**
     * 光线遍历 BVH，找最近交点
     * @param hit_prim [输出] 命中的图元在 prims 中的索引
     */
    bool traverse(const std::vector<Primitive>& prims, int node_idx,
                  const Ray& ray, float& t_hit, int& hit_prim) const {
        const BVHNode& node = nodes[node_idx];

        float t_enter, t_exit;
        g_aabb_count++;  // AABB 求交测试计数
        if (!node.box.intersect(ray, t_enter, t_exit)) return false;
        // 包围盒比已知最近交点还远 → 里面的图元不可能更近，整棵跳过
        if (t_enter > t_hit) return false;

        if (node.n_prims > 0) {
            // 叶子节点：测试内部所有图元
            bool hit = false;
            for (int i = node.first_prim; i < node.first_prim + node.n_prims; i++) {
                float t;
                if (prims[idx[i]].intersect(ray, t) && t < t_hit) {
                    t_hit = t;
                    hit_prim = idx[i];
                    hit = true;
                }
            }
            return hit;
        }

        // 内部节点：递归左右子树
        bool l = traverse(prims, node.left, ray, t_hit, hit_prim);
        bool r = traverse(prims, node.right, ray, t_hit, hit_prim);
        return l || r;
    }
};

// ========== 朴素遍历：每条光线测所有图元（2-2 的做法） ==========
bool naive_intersect(const std::vector<Primitive>& prims, const Ray& ray,
                     float& t_hit, int& hit_prim) {
    bool hit = false;
    for (int i = 0; i < (int)prims.size(); i++) {
        float t;
        if (prims[i].intersect(ray, t) && t < t_hit) {
            t_hit = t;
            hit_prim = i;
            hit = true;
        }
    }
    return hit;
}

// ========== HSV → RGB（给球体生成彩色） ==========
Vec3 hsv2rgb(float h, float s, float v) {
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    float r, g, b;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return Vec3(r, g, b);
}

// ========== 生成网格球（经纬细分，图元全部是三角形） ==========
// 把球面细分成 rings×sectors 个四边形，每个四边形拆成 2 个三角形。
// 这样就得到一个"单个物体 + 大量三角面"的场景，模拟真实引擎中一个模型
// 由成千上万个三角形组成的情形，交给 BVH 加速这些三角形的求交。
std::vector<Primitive> make_uv_sphere(const Vec3& center, float radius,
                                      int rings, int sectors) {
    std::vector<Primitive> out;
    out.reserve(rings * sectors * 2);

    for (int i = 0; i < rings; i++) {
        // 纬度角 theta：0（北极）→ PI（南极）
        float theta0 = (float)i / rings * PI;
        float theta1 = (float)(i + 1) / rings * PI;
        for (int j = 0; j < sectors; j++) {
            // 经度角 phi：绕 Y 轴一圈
            float phi0 = (float)j / sectors * 2.0f * PI;
            float phi1 = (float)(j + 1) / sectors * 2.0f * PI;

            // 四边形四个角点（球坐标 → 笛卡尔坐标）
            Vec3 p00(radius * sin(theta0) * cos(phi0), radius * cos(theta0),
                     radius * sin(theta0) * sin(phi0));
            Vec3 p01(radius * sin(theta0) * cos(phi1), radius * cos(theta0),
                     radius * sin(theta0) * sin(phi1));
            Vec3 p10(radius * sin(theta1) * cos(phi0), radius * cos(theta1),
                     radius * sin(theta1) * sin(phi0));
            Vec3 p11(radius * sin(theta1) * cos(phi1), radius * cos(theta1),
                     radius * sin(theta1) * sin(phi1));

            // 颜色：沿经度方向 HSV 渐变，形成一圈彩色带，方便看出三角面的划分
            Vec3 col = hsv2rgb((float)j / sectors, 0.75f, 1.0f) * 255.0f;

            // 加入一个三角形并确保法线朝外：
            // 法线与 (质心 - 球心) 同向则朝外，否则交换 v1/v2 翻转
            auto push_tri = [&](Vec3 a, Vec3 b, Vec3 c) {
                Vec3 n = (b - a).cross(c - a);
                Vec3 centroid = (a + b + c) / 3.0f;
                if (n.dot(centroid - center) < 0) std::swap(b, c);
                out.push_back(Primitive::make_triangle(a, b, c, col));
            };
            // 四边形拆成两个三角形
            push_tri(p00, p10, p11);
            push_tri(p00, p11, p01);
        }
    }
    return out;
}

// ========== 渲染一帧 ==========
// use_bvh=true 走 BVH 遍历，false 走朴素遍历；两种方式各自计数
long long render(const std::vector<Primitive>& prims, const BVH& bvh, bool use_bvh,
                 int WIDTH, int HEIGHT,
                 const Vec3& camera_pos, const Vec3& light_pos,
                 float (*framebuffer)[512][3]) {
    g_prim_count = 0;
    g_aabb_count = 0;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float ndc_x = (x + 0.5f) / WIDTH * 2.0f - 1.0f;
            float ndc_y = (y + 0.5f) / HEIGHT * 2.0f - 1.0f;
            Vec3 dir = (Vec3(ndc_x, ndc_y, -1.0f)).normalized();
            Ray ray(camera_pos, dir);

            float t_hit = INF;
            int hit_prim = -1;
            if (use_bvh)
                bvh.traverse(prims, bvh.root, ray, t_hit, hit_prim);
            else
                naive_intersect(prims, ray, t_hit, hit_prim);

            if (hit_prim >= 0) {
                const Primitive& p = prims[hit_prim];
                Vec3 P = ray.origin + ray.direction * t_hit;   // 交点
                Vec3 n = p.normal_at(P);                       // 法线

                // Blinn-Phong 着色
                Vec3 light_dir = (light_pos - P).normalized();
                Vec3 view_dir = (camera_pos - P).normalized();
                Vec3 half_vec = (light_dir + view_dir).normalized();
                float ndotl = n.dot(light_dir); if (ndotl < 0) ndotl = 0;
                float ndoth = n.dot(half_vec); if (ndoth < 0) ndoth = 0;
                float spec = pow(ndoth, 32.0f);
                float spec_mult = (ndotl > 0) ? 1.0f : 0.0f;
                Vec3 final = p.color * (0.1f + ndotl) + Vec3(255, 255, 255) * spec * spec_mult;

                framebuffer[y][x][0] = final.x;
                framebuffer[y][x][1] = final.y;
                framebuffer[y][x][2] = final.z;
            }
            // 未命中：保持黑色
        }
    }
    return g_prim_count;  // 返回图元求交次数
}

// ========== 主函数 ==========
int main() {
    const int WIDTH = 512;
    const int HEIGHT = 512;
    const Vec3 camera_pos(0, 0, 4.5f);
    const Vec3 light_pos(1.5f, 2.5f, 3.0f);

    // ===== 场景：一个网格球（单个物体细分出大量三角面） =====
    // 24 个纬度带 × 12 个经度段 × 2 = 576 个三角形
    // 模拟真实引擎中"一个模型由成千上万三角形组成"的情形
    const int RINGS = 24;      // 纬度带数量
    const int SECTORS = 12;    // 经度段数量
    const float SPHERE_R = 1.2f;
    std::vector<Primitive> prims = make_uv_sphere(Vec3(0, 0, 0), SPHERE_R, RINGS, SECTORS);

    // ===== 构建 BVH =====
    BVH bvh;
    bvh.build(prims);
    printf("图元总数: %d (网格球 %d×%d×2 个三角形), BVH 节点数: %d\n",
           (int)prims.size(), RINGS, SECTORS, (int)bvh.nodes.size());

    // ===== 创建 framebuffer =====
    float (*framebuffer)[WIDTH][3] = (float (*)[WIDTH][3])malloc(
        HEIGHT * sizeof(*framebuffer));
    if (!framebuffer) return 1;
    memset(framebuffer, 0, HEIGHT * sizeof(*framebuffer));

    // ===== 分别用朴素遍历和 BVH 渲染各一次，统计求交次数与耗时 =====
    clock_t t0 = clock();
    long long naive_prim = render(prims, bvh, /*use_bvh=*/false,
                                  WIDTH, HEIGHT, camera_pos, light_pos, framebuffer);
    clock_t t1 = clock();
    printf("[朴素] 图元求交次数: %lld, 耗时: %.3f s\n",
           naive_prim, (double)(t1 - t0) / CLOCKS_PER_SEC);

    long long bvh_prim = render(prims, bvh, /*use_bvh=*/true,
                                WIDTH, HEIGHT, camera_pos, light_pos, framebuffer);
    clock_t t2 = clock();
    printf("[BVH ] AABB 求交次数: %lld\n", g_aabb_count);
    printf("[BVH ] 图元求交次数: %lld, 耗时: %.3f s\n",
           bvh_prim, (double)(t2 - t1) / CLOCKS_PER_SEC);
    printf("图元求交次数减少: %.1f%%\n",
           100.0f * (1.0f - (float)bvh_prim / (float)naive_prim));

    // ===== 输出 BVH 渲染的那一帧 =====
    std::string cpp_path(__FILE__);
    std::string ppm_path = cpp_path.substr(0, cpp_path.find_last_of("/\\"))
                         + "/output_2-3.ppm";
    FILE* f = fopen(ppm_path.c_str(), "w");
    if (!f) return 1;
    fprintf(f, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int r = (int)framebuffer[y][x][0]; if (r < 0) r = 0; if (r > 255) r = 255;
            int g = (int)framebuffer[y][x][1]; if (g < 0) g = 0; if (g > 255) g = 255;
            int b = (int)framebuffer[y][x][2]; if (b < 0) b = 0; if (b > 255) b = 255;
            fprintf(f, "%d %d %d\n", r, g, b);
        }
    }
    fclose(f);

    free(framebuffer);
    return 0;
}
