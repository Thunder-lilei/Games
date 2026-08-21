@echo off
chcp 65001 > nul
echo 正在将 PPM 序列合并为 GIF 动画...
echo 确保已安装 Pillow 库（pip install pillow）
echo.
python ppm_to_gif.py
echo.
echo 按任意键退出...
pause > nul