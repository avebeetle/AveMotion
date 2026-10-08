"""Render original README artwork, not AveMotion output or a benchmark.

Requires Pillow and the Windows Segoe UI fonts. No downloaded sticker assets.
Run from any directory; writes avemotion-motion.gif beside this script.
"""

from pathlib import Path
import math

from PIL import Image, ImageDraw, ImageFont


def render():
    scale = 2
    width, height = 960, 300
    background = Image.new("RGB", (width * scale, height * scale))
    draw = ImageDraw.Draw(background)
    for y in range(height * scale):
        blend = y / (height * scale - 1)
        color = tuple(round(a + (b - a) * blend) for a, b in zip((22, 25, 39), (12, 16, 27)))
        draw.line((0, y, width * scale, y), fill=color)

    def font(size, bold=False):
        name = "seguisb.ttf" if bold else "segoeui.ttf"
        return ImageFont.truetype(str(Path("C:/Windows/Fonts") / name), size * scale)

    draw.text((48 * scale, 62 * scale), "AveMotion", font=font(58, True), fill="#f0f4ff")
    draw.text((51 * scale, 143 * scale), "Animated stickers. Native UI motion.",
              font=font(21), fill="#adb8d1")
    draw.text((52 * scale, 229 * scale), "C++20  /  VECTOR ANIMATION  /  WINDOWS",
              font=font(12, True), fill="#7e8ba8")

    frames = []
    for index in range(48):
        phase = math.tau * index / 48
        image = background.copy()
        draw = ImageDraw.Draw(image)

        def box(values):
            return tuple(round(value * scale) for value in values)

        # Three original geometric sticker characters with a quiet floating loop.
        for number, (center_x, center_y, color, radius) in enumerate((
            (608, 158, "#bba0ff", 48),
            (746, 138, "#63dbe5", 43),
            (858, 180, "#ffd08a", 39),
        )):
            center_y += 8 * math.sin(phase + number * 1.6)
            draw.ellipse(box((center_x-radius, 248, center_x+radius, 257)), fill="#0b0f1b")
            bounds = box((center_x-radius, center_y-radius, center_x+radius, center_y+radius))
            if number == 0:
                draw.rounded_rectangle(bounds, radius=round(19 * scale), fill=color)
            else:
                draw.ellipse(bounds, fill=color)
            blink = math.cos(phase + number * 0.7) > 0.97
            for eye in (-12, 12):
                if blink:
                    draw.line(box((center_x+eye-3, center_y-7, center_x+eye+3, center_y-7)),
                              fill="#253047", width=3 * scale)
                else:
                    draw.ellipse(box((center_x+eye-3, center_y-12, center_x+eye+3, center_y-4)),
                                 fill="#253047")
            draw.arc(box((center_x-12, center_y-4, center_x+12, center_y+15)),
                     16, 164, fill="#253047", width=3 * scale)

        # Small dots are decorative accents, not counters or engine diagnostics.
        for number, (x, y) in enumerate(((548, 91), (686, 218), (809, 78), (919, 116))):
            y += 4 * math.sin(phase + number)
            draw.ellipse(box((x-3, y-3, x+3, y+3)), fill="#63718e")
        frames.append(image.resize((width, height), Image.Resampling.LANCZOS))

    palette = frames[0].quantize(colors=128)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    target = Path(__file__).with_name("avemotion-motion.gif")
    indexed[0].save(target, save_all=True, append_images=indexed[1:], duration=60,
                    loop=0, optimize=True, disposal=1)
    return target


if __name__ == "__main__":
    print(render())
