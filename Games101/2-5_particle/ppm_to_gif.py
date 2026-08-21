#!/usr/bin/env python3
"""
PPM 序列转 GIF 脚本
功能：读取按 output_frame_%03d.ppm 命名规则的 PPM 文件，合并为一个 GIF 动画
帧率：默认 30fps，对应 1~2 秒时长（30~60 帧）
依赖：需要安装 Pillow 库：pip install pillow
"""

import os
import re
from PIL import Image

# 配置参数
FPS = 30  # GIF 帧率（每秒多少帧），按要求使用 30fps
PATTERN = r'output_frame_(\d+)\.ppm'  # PPM 文件命名模式
OUTPUT_GIF = 'particle_animation.gif'  # 输出 GIF 文件名

def main():
    # 获取当前目录下所有匹配的 PPM 文件
    ppm_files = []
    for filename in os.listdir('.'):
        match = re.match(PATTERN, filename)
        if match:
            frame_num = int(match.group(1))
            ppm_files.append((frame_num, filename))
    
    if not ppm_files:
        print(f"错误：当前目录没有找到匹配 '{PATTERN}' 的 PPM 文件")
        print("请先运行 ray_particle.exe 生成 PPM 序列，再运行此脚本")
        return
    
    # 按帧序号排序
    ppm_files.sort(key=lambda x: x[0])
    print(f"找到 {len(ppm_files)} 个 PPM 文件")
    
    # 读取所有帧
    frames = []
    for i, (frame_num, filename) in enumerate(ppm_files):
        try:
            img = Image.open(filename)
            # PPM 是 RGB，不需要转换
            frames.append(img)
            if (i + 1) % 10 == 0:
                print(f"已读取 {i + 1}/{len(ppm_files)} 帧")
        except Exception as e:
            print(f"读取 {filename} 失败：{e}")
            continue
    
    if not frames:
        print("错误：没有成功读取任何 PPM 文件")
        return
    
    # 计算每帧持续时间（毫秒）
    duration = int(1000 / FPS)
    print(f"开始生成 GIF，帧率 {FPS}fps，每帧持续 {duration}ms...")
    
    # 保存为 GIF
    try:
        # 第一帧保存，后续帧追加
        frames[0].save(
            OUTPUT_GIF,
            save_all=True,
            append_images=frames[1:],
            duration=duration,
            loop=0  # 0 表示无限循环
        )
        print(f"GIF 生成完成：{OUTPUT_GIF}")
        print(f"总帧数：{len(frames)}，总时长：{len(frames)/FPS:.1f} 秒")
    except Exception as e:
        print(f"生成 GIF 失败：{e}")

if __name__ == '__main__':
    main()
