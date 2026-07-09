# Video ASCII Art

这个项目将视频的每一帧分割成 `m*n` 网格，计算每个网格的灰度值，并将灰度映射到字符集。处理像素矩阵使用 `numpy`，绘制帧图像使用 `PIL`。

## 文件

- `video_to_ascii.py` - 主脚本，用于加载视频、处理帧、生成 ASCII 图像并保存为 PNG。

## 依赖

- Python 3
- numpy
- pillow
- opencv-python

## 使用

```powershell
python video_to_ascii.py --input path\to\video.mp4 --output output.png --grid_rows 48 --grid_cols 80
```

可选参数：`--invert`, `--chars`。
