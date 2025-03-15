from PIL import Image
import os

def convert_png_to_ico(png_file, ico_file):
    try:
        # 打开PNG图像
        img = Image.open(png_file)

        # 调整大小为16x16和32x32（常用的图标尺寸）
        img_16 = img.resize((16, 16), Image.LANCZOS)
        img_32 = img.resize((32, 32), Image.LANCZOS)

        # 保存为ICO文件，包含多个尺寸
        img_16.save(ico_file, format='ICO', sizes=[(16, 16), (32, 32)])

        print(f"成功将 {png_file} 转换为 {ico_file}")
        return True
    except Exception as e:
        print(f"转换失败: {e}")
        return False

# 转换图标
convert_png_to_ico("alert_running.png", "alert_running.ico")
convert_png_to_ico("alert_paused.png", "alert_paused.ico")

print("转换完成！")