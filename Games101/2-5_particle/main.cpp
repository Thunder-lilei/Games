// =============================================================
// 2-5 粒子系统（Particle System）
// 实现：单粒子 → 发射器 → 生命周期 → 重力/阻尼/碰撞
//       → 广告牌渲染（圆形光斑+软边缘）+ 多帧动画输出
// =============================================================

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>

// ============================================================
// 常量配置（调参入口，修改后重新编译即可）
// ============================================================
constexpr int WIDTH = 512;          // 画布宽度（像素）
constexpr int HEIGHT = 512;         // 画布高度（像素）
constexpr float GRAVITY = 9.8f;     // 重力加速度（越大下落越快）
constexpr float DRAG = 0.5f;        // 空气阻力（0.2~1.0，越大减速越快）
constexpr float GROUND_Y = -1.8f;   // 地面高度（碰撞平面）
constexpr int EMIT_RATE = 10;       // 每帧发射粒子数
constexpr int MAX_PARTICLES = 1000; // 粒子上限（防爆内存）
constexpr int TOTAL_FRAMES = 60;    // 动画总帧数（30fps × 2s）
constexpr float DT = 1.0f / 30.0f;  // 每帧时间步长（对应 30fps）

// ============================================================
// Vec3 —— 三维向量
// ============================================================
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(float s) const { return Vec3(x / s, y / s, z / s); }
    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
};

// ============================================================
// Framebuffer —— 帧缓冲（RGB 24bit）
// ============================================================
struct Framebuffer {
    // 像素数据，data[y][x][0=R,1=G,2=B]
    unsigned char data[HEIGHT][WIDTH][3];

    // 清空为全黑
    void clear() {
        for (int y = 0; y < HEIGHT; ++y)
            for (int x = 0; x < WIDTH; ++x)
                data[y][x][0] = data[y][x][1] = data[y][x][2] = 0;
    }
};

// ============================================================
// Particle —— 粒子数据结构
// ============================================================
struct Particle {
    Vec3 position;   // 世界坐标位置
    Vec3 velocity;   // 速度（每帧方向与大小）
    float life;      // 剩余寿命（秒）
    float max_life;  // 初始寿命（用于计算生命比例）
    float size;      // 屏幕像素半径
};

// ============================================================
// 随机数工具
// ============================================================
static float random01() {
    return (float)std::rand() / (float)RAND_MAX;
}

static float random_range(float min, float max) {
    return min + random01() * (max - min);
}

// ============================================================
// 获取输出目录（基于 __FILE__ 宏，确保 PPM 输出在源码目录）
// ============================================================
static std::string get_output_dir() {
    std::string path(__FILE__);
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos + 1);
    }
    return "";
}

// ============================================================
// 发射粒子：在原点位置生成一个粒子，速度带随机性
// ============================================================
static void emit_particle(std::vector<Particle>& particles, const Vec3& origin) {
    if ((int)particles.size() >= MAX_PARTICLES) return;

    Particle p;
    p.position = origin;
    // 速度随机：向上喷射，水平散开
    p.velocity = Vec3(
        random_range(-2.0f, 2.0f),   // 水平随机
        random_range(2.0f, 5.0f),    // 向上
        random_range(-1.0f, 1.0f)    // Z 方向（小范围）
    );
    p.life = random_range(1.0f, 3.0f);   // 寿命 1~3 秒
    p.max_life = p.life;
    p.size = random_range(3.0f, 6.0f);   // 半径 3~6 像素
    particles.push_back(p);
}

// ============================================================
// 更新所有粒子：物理（重力+阻尼+碰撞）和生死管理
// ============================================================
static void update_particles(std::vector<Particle>& particles, float dt) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        // 生命递减
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = particles.erase(it);  // 消亡 = 从数组移除
            continue;
        }

        // 重力：每帧给速度加一个向下增量
        it->velocity.y -= GRAVITY * dt;

        // 阻尼：空气阻力让速度缓慢衰减
        it->velocity *= (1.0f - DRAG * dt);

        // 运动：位置 += 速度 × 时间步长
        it->position += it->velocity * dt;

        // 地面碰撞反弹：y 分量翻转并衰减
        if (it->position.y < GROUND_Y) {
            it->position.y = GROUND_Y;               // 拉回地面
            it->velocity.y = -it->velocity.y * 0.6f; // 反弹（弹性系数 0.6）
        }

        ++it;
    }
}

// ============================================================
// 绘制所有粒子：广告牌渲染（圆形光斑 + 软边缘）
// 使用 additive blending（叠加混合），亮处更亮
// ============================================================
static void draw_particles(Framebuffer& fb, const std::vector<Particle>& particles) {
    for (const auto& p : particles) {
        // 正交投影：世界坐标 → 屏幕像素
        // 世界范围 x∈[-2,2] → [0,WIDTH]，y∈[-2,2] → [0,HEIGHT]（y 翻转）
        int sx = (int)((p.position.x + 2.0f) / 4.0f * WIDTH);
        int sy = (int)((2.0f - p.position.y) / 4.0f * HEIGHT);

        int radius = (int)p.size;
        if (radius < 1) radius = 1;

        // 生命比例：1.0（刚出生）→ 0.0（快死），控制颜色亮度
        float life_ratio = p.life / p.max_life;

        // 基色：蓝青色光斑
        float r = 100.0f * life_ratio;
        float g = 200.0f * life_ratio;
        float b = 255.0f;

        // 遍历粒子外接矩形内的所有像素
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int px = sx + dx;
                int py = sy + dy;
                if (px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT) continue;

                float dist = std::sqrt((float)(dx * dx + dy * dy));
                if (dist > radius) continue;

                // 软边缘：中心 1.0 → 边缘 0.0
                float edge_alpha = 1.0f - dist / radius;

                // 最终透明度 = 生命比例 × 边缘透明度
                float alpha = life_ratio * edge_alpha;

                // Additive blending：累加颜色（产生发光效果）
                float cr = (float)fb.data[py][px][0] + r * alpha;
                float cg = (float)fb.data[py][px][1] + g * alpha;
                float cb = (float)fb.data[py][px][2] + b * alpha;
                if (cr > 255.0f) cr = 255.0f;
                if (cg > 255.0f) cg = 255.0f;
                if (cb > 255.0f) cb = 255.0f;
                fb.data[py][px][0] = (unsigned char)cr;
                fb.data[py][px][1] = (unsigned char)cg;
                fb.data[py][px][2] = (unsigned char)cb;
            }
        }
    }
}

// ============================================================
// 输出 PPM 图像
// ============================================================
static void save_ppm(const Framebuffer& fb, const char* filename) {
    FILE* fp = std::fopen(filename, "wb");
    if (!fp) {
        std::fprintf(stderr, "无法写入文件: %s\n", filename);
        return;
    }
    std::fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    std::fwrite(fb.data, 1, WIDTH * HEIGHT * 3, fp);
    std::fclose(fp);
}

// ============================================================
// 主函数
// ============================================================
int main() {
    std::srand((unsigned int)std::time(nullptr));

    std::string out_dir = get_output_dir();
    std::vector<Particle> particles;
    Framebuffer fb;

    // 发射器位置：画布中心偏上
    Vec3 emitter_pos(0.0f, 1.5f, 0.0f);

    std::printf("Particle System Simulation\n");
    std::printf("Total frames: %d, FPS: %d, Duration: %.1f s\n",
                TOTAL_FRAMES, (int)(1.0f / DT), TOTAL_FRAMES * DT);
    std::printf("Output directory: %s\n\n", out_dir.c_str());

    // 主循环：按帧推进
    for (int frame = 0; frame < TOTAL_FRAMES; ++frame) {
        fb.clear();  // 清空画布

        // ① 发射新粒子
        for (int i = 0; i < EMIT_RATE; ++i) {
            emit_particle(particles, emitter_pos);
        }

        // ② 更新所有粒子
        update_particles(particles, DT);

        // ③ 绘制所有粒子
        draw_particles(fb, particles);

        // ④ 输出 PPM
        char filename[256];
        std::snprintf(filename, sizeof(filename), "%soutput_frame_%03d.ppm",
                      out_dir.c_str(), frame);
        save_ppm(fb, filename);

        std::printf("Frame %03d/%d: %zu particles\n",
                    frame + 1, TOTAL_FRAMES, particles.size());
    }

    std::printf("\nDone! Generated %d frames.\n", TOTAL_FRAMES);
    std::printf("Run 'make_gif.bat' to create GIF animation.\n");

    return 0;
}