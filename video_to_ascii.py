import argparse
import os
from typing import Sequence

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ASCII_CHARS = "@%#*+=-:. "


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert video frames into ASCII art images using a grid-based grayscale mapping."
    )
    parser.add_argument("--input", required=True, help="Input video file path.")
    parser.add_argument("--output", required=True, help="Output image file path.")
    parser.add_argument("--grid_rows", type=int, default=40, help="Number of grid rows for each frame.")
    parser.add_argument("--grid_cols", type=int, default=80, help="Number of grid columns for each frame.")
    parser.add_argument("--frame_index", type=int, default=0, help="Index of frame to convert (0-based)."
    )
    parser.add_argument("--invert", action="store_true", help="Invert grayscale mapping.")
    parser.add_argument(
        "--chars",
        default=ASCII_CHARS,
        help="Characters to use for mapping from dark to bright.")
    parser.add_argument(
        "--font_size",
        type=int,
        default=10,
        help="Font size used to draw ASCII characters.")
    parser.add_argument(
        "--font_path",
        default=None,
        help="Optional path to a TTF font file.")
    return parser.parse_args()


def get_ascii_char(value: float, chars: Sequence[str], invert: bool) -> str:
    if invert:
        value = 255.0 - value
    index = int((value / 255.0) * (len(chars) - 1))
    return chars[index]


def frame_to_grid_ascii(frame: np.ndarray, rows: int, cols: int, chars: Sequence[str], invert: bool) -> Sequence[str]:
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    height, width = gray.shape
    cell_h = max(height // rows, 1)
    cell_w = max(width // cols, 1)

    ascii_lines = []
    for row in range(rows):
        start_y = row * cell_h
        end_y = start_y + cell_h if row < rows - 1 else height
        line_chars = []
        for col in range(cols):
            start_x = col * cell_w
            end_x = start_x + cell_w if col < cols - 1 else width
            cell = gray[start_y:end_y, start_x:end_x]
            if cell.size == 0:
                avg = 0.0
            else:
                avg = float(np.mean(cell))
            line_chars.append(get_ascii_char(avg, chars, invert))
        ascii_lines.append("".join(line_chars))
    return ascii_lines


def draw_ascii_image(ascii_lines: Sequence[str], font_size: int, font_path: str | None) -> Image.Image:
    font = ImageFont.truetype(font_path, font_size) if font_path else ImageFont.load_default()
    max_width = max(len(line) for line in ascii_lines)
    image_width = max_width * font_size
    image_height = len(ascii_lines) * int(font_size * 1.1)
    image = Image.new("RGB", (image_width, image_height), color="white")
    draw = ImageDraw.Draw(image)
    for idx, line in enumerate(ascii_lines):
        draw.text((0, idx * int(font_size * 1.1)), line, fill="black", font=font)
    return image


def main() -> None:
    args = parse_args()
    if not os.path.isfile(args.input):
        raise FileNotFoundError(f"Input video not found: {args.input}")

    capture = cv2.VideoCapture(args.input)
    if not capture.isOpened():
        raise RuntimeError(f"Unable to open video: {args.input}")

    total_frames = int(capture.get(cv2.CAP_PROP_FRAME_COUNT))
    if args.frame_index < 0 or args.frame_index >= total_frames:
        raise ValueError(
            f"frame_index must be between 0 and {total_frames - 1}, got {args.frame_index}"
        )

    capture.set(cv2.CAP_PROP_POS_FRAMES, args.frame_index)
    success, frame = capture.read()
    capture.release()
    if not success:
        raise RuntimeError(f"Unable to read frame {args.frame_index} from video")

    ascii_lines = frame_to_grid_ascii(
        frame,
        args.grid_rows,
        args.grid_cols,
        args.chars,
        args.invert,
    )
    image = draw_ascii_image(ascii_lines, args.font_size, args.font_path)
    image.save(args.output)
    print(f"Saved ASCII image: {args.output}")


if __name__ == "__main__":
    main()
