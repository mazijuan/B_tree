import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import os
import platform
import time

# ===================== 配置参数 =====================
VIDEO_PATH = r"Z:\输出\ps\gund@m\黑星\anime\744304582.mp4"
OUTPUT_FRAME_DIR = r"Z:\输出\jiaoben"
M_GRID = 108       # 网格行数 m
N_GRID = 198       # 网格列数 n
CHAR_FONT_SIZE = 18
# 灰度字符：从暗到亮
GRAY_CHARS = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/|()1{}[]?-_+~<>i!lI;:,\"^`'. "
FONT_PATH = "simhei.ttf"
SKIP_FRAME = 1
TERMINAL_PLAY = True  # 开启PowerShell终端实时播放
FRAME_DELAY = 0.03    # 帧间隔，控制播放速度
# ====================================================

if not os.path.exists(OUTPUT_FRAME_DIR):
    os.makedirs(OUTPUT_FRAME_DIR)

# Windows清屏 / Linux/Mac清屏指令
def clear_console():
    if platform.system() == "Windows":
        os.system("cls")
    else:
        os.system("clear")

def get_avg_gray(block: np.ndarray) -> float:
    gray_block = cv2.cvtColor(block, cv2.COLOR_BGR2GRAY)
    return float(np.mean(gray_block))

def gray_to_char(gray_val: float, char_list: list) -> str:
    idx = int((gray_val / 255) * (len(char_list) - 1))
    return char_list[idx]

def frame_to_terminal_text(frame: np.ndarray, m: int, n: int) -> str:
    """只生成终端打印用的字符文本，不生成PIL图片"""
    h, w = frame.shape[:2]
    block_h = h / m
    block_w = w / n
    output_lines = []

    for row in range(m):
        line_buf = []
        y_s = int(row * block_h)
        y_e = int((row + 1) * block_h)
        for col in range(n):
            x_s = int(col * block_w)
            x_e = int((col + 1) * block_w)
            tile = frame[y_s:y_e, x_s:x_e]
            avg_g = get_avg_gray(tile)
            c = gray_to_char(avg_g, GRAY_CHARS)
            line_buf.append(c)
        output_lines.append("".join(line_buf))
    return "\n".join(output_lines)

def frame_to_char_image(frame: np.ndarray, m: int, n: int, font, font_size: int):
    h, w = frame.shape[:2]
    block_h = h / m
    block_w = w / n
    char_matrix = []
    for row in range(m):
        row_chars = []
        y_start = int(row * block_h)
        y_end = int((row + 1) * block_h)
        for col in range(n):
            x_start = int(col * block_w)
            x_end = int((col + 1) * block_w)
            tile = frame[y_start:y_end, x_start:x_end]
            avg_g = get_avg_gray(tile)
            c = gray_to_char(avg_g, GRAY_CHARS)
            row_chars.append(c)
        char_matrix.append("".join(row_chars))
    
    canvas_w = n * font_size
    canvas_h = m * font_size
    img = Image.new("RGB", (canvas_w, canvas_h), "white")
    draw = ImageDraw.Draw(img)
    for line_idx, line_str in enumerate(char_matrix):
        y_pos = line_idx * font_size
        draw.text((0, y_pos), line_str, font=font, fill="black")
    return img

def main():
    # 加载字体
    try:
        font = ImageFont.truetype(FONT_PATH, CHAR_FONT_SIZE)
    except Exception:
        font = ImageFont.load_default()
        print("未找到指定字体，使用系统默认字体")

    cap = cv2.VideoCapture(VIDEO_PATH)
    if not cap.isOpened():
        print(f"无法打开视频 {VIDEO_PATH}")
        return
    
    frame_count = 0
    save_idx = 0
    print("开始处理视频，按 Ctrl+C 随时终止播放\n")
    time.sleep(1)

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            if frame_count % SKIP_FRAME != 0:
                frame_count += 1
                continue
            
            # 终端实时播放逻辑
            if TERMINAL_PLAY:
                text_frame = frame_to_terminal_text(frame, M_GRID, N_GRID)
                clear_console()
                print(text_frame)
                time.sleep(FRAME_DELAY)

            # 保存字符图片（原有功能保留）
            char_pil_img = frame_to_char_image(frame, M_GRID, N_GRID, font, CHAR_FONT_SIZE)
            save_path = os.path.join(OUTPUT_FRAME_DIR, f"frame_{save_idx:06d}.png")
            char_pil_img.save(save_path)

            frame_count += 1
            save_idx += 1
            if save_idx % 30 == 0:
                print(f"已保存 {save_idx} 帧图片")

    except KeyboardInterrupt:
        # 用户按 Ctrl+C 退出
        clear_console()
        print("\n播放已手动终止")
    finally:
        cap.release()
        print(f"\n处理完成！字符图片保存在 {OUTPUT_FRAME_DIR}")

if __name__ == "__main__":
    main()