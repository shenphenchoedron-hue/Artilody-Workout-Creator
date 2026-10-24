#!/usr/bin/env python3
"""Generates the Workout Creator icon set from the source image
assets/icons/creator-icon-source.png (any size, ideally >= 1024 px, RGBA).

A non-square source is padded with transparency and centred. Sizes larger
than the source are upscaled (a warning is printed).

Needs Pillow. Writes assets/icons/{app-icon.png (1024), app-icon-256.png,
app-icon.ico} and packaging/linux/icons/<n>x<n>/artilody-workout-creator.png.
On macOS also AppIcon.icns (via iconutil).
"""
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "assets", "icons")
SRC = os.path.join(OUT, "creator-icon-source.png")
MASTER_SIZE = 1024


def load_source():
    img = Image.open(SRC).convert("RGBA")
    w, h = img.size
    if w != h:
        side = max(w, h)
        print("source is %dx%d: padding to %dx%d (transparent, centred)" % (w, h, side, side))
        canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
        canvas.paste(img, ((side - w) // 2, (side - h) // 2))
        img = canvas
    return img


def main():
    src = load_source()
    side = src.size[0]

    def sized(n):
        if n > side:
            print("warning: upscaling %d -> %d" % (side, n))
        return src if n == side else src.resize((n, n), Image.LANCZOS)

    os.makedirs(OUT, exist_ok=True)
    sized(MASTER_SIZE).save(os.path.join(OUT, "app-icon.png"), optimize=True)
    sized(256).save(os.path.join(OUT, "app-icon-256.png"), optimize=True)
    # Pillow derives every .ico entry from the image it is given (LANCZOS).
    sized(256).save(os.path.join(OUT, "app-icon.ico"),
                    sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    for n in (48, 64, 128, 256, 512):
        d = os.path.join(ROOT, "packaging", "linux", "icons", "%dx%d" % (n, n))
        os.makedirs(d, exist_ok=True)
        sized(n).save(os.path.join(d, "artilody-workout-creator.png"), optimize=True)
    if sys.platform == "darwin" and shutil.which("iconutil"):
        tmp = tempfile.mkdtemp()
        iconset = os.path.join(tmp, "AppIcon.iconset")
        os.makedirs(iconset)
        for n in (16, 32, 128, 256, 512):
            sized(n).save(os.path.join(iconset, "icon_%dx%d.png" % (n, n)))
            sized(2 * n).save(os.path.join(iconset, "icon_%dx%d@2x.png" % (n, n)))
        subprocess.check_call(["iconutil", "-c", "icns", iconset, "-o", os.path.join(OUT, "AppIcon.icns")])
        shutil.rmtree(tmp)
    print("icons written to", OUT)


if __name__ == "__main__":
    main()
