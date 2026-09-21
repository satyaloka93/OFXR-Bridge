"""Convert captured RGBA/BGRA UNORM8 color textures to lossless RGB PPM previews."""
import argparse
import json
from pathlib import Path


def convert(path):
    meta = json.loads(Path(str(path) + ".json").read_text())
    fmt = meta["format"]
    if fmt not in (27, 28, 29, 87, 90, 91):
        raise ValueError(f"DXGI format {fmt} is not supported for preview; raw data is preserved")
    data = path.read_bytes()
    for index, sub in enumerate(meta["subresources"]):
        width, height, pitch, offset = (sub[k] for k in ("width", "height", "pitch", "offset"))
        if pitch < width * 4 or offset + (height - 1) * pitch + width * 4 > len(data):
            raise ValueError("Invalid footprint or truncated capture")
        rgb = bytearray()
        for y in range(height):
            row = data[offset + y * pitch:offset + y * pitch + width * 4]
            for x in range(0, len(row), 4):
                rgb.extend(row[x:x + 3] if fmt in (27, 28, 29) else row[x:x + 3][::-1])
        output = path.with_name(f"{path.stem}-{index}.ppm")
        output.write_bytes(f"P6\n{width} {height}\n255\n".encode() + rgb)
        print(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    convert(parser.parse_args().capture)
