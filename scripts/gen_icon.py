#!/usr/bin/env python3
"""Generate Phograph app icon: a node with inputs on top, output on bottom (matching the program)."""

from PIL import Image, ImageDraw, ImageFont
import os

def draw_icon(size):
    """Draw the icon at a given pixel size and return the Image."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    s = size  # shorthand

    # Background: rounded rectangle
    bg_color = (30, 30, 42)
    draw.rounded_rectangle([0, 0, s - 1, s - 1], radius=s * 0.18, fill=bg_color)
    border_color = (60, 60, 80)
    draw.rounded_rectangle([0, 0, s - 1, s - 1], radius=s * 0.18, outline=border_color, width=max(1, s // 128))

    # -- Node box -- centered, wider than tall to fit 2 pins across top
    nw = int(s * 0.56)
    nh = int(s * 0.34)
    nx = (s - nw) // 2
    ny = int(s * 0.26)
    node_radius = s * 0.04
    node_bg = (50, 55, 72)
    node_border = (100, 130, 200)
    node_border_w = max(1, s // 150)

    # Node shadow
    shadow_off = max(2, s // 100)
    draw.rounded_rectangle(
        [nx + shadow_off, ny + shadow_off, nx + nw + shadow_off, ny + nh + shadow_off],
        radius=node_radius, fill=(15, 15, 22)
    )
    # Node body
    draw.rounded_rectangle(
        [nx, ny, nx + nw, ny + nh],
        radius=node_radius, fill=node_bg, outline=node_border, width=node_border_w
    )

    # -- Title bar at top of node --
    title_h = int(nh * 0.30)
    title_bg = (65, 75, 110)
    draw.rounded_rectangle(
        [nx, ny, nx + nw, ny + title_h],
        radius=node_radius, fill=title_bg
    )
    # Fix bottom corners of title bar
    draw.rectangle(
        [nx, ny + title_h - int(node_radius), nx + nw, ny + title_h],
        fill=title_bg
    )

    # "+" label in title bar
    try:
        plus_font_size = int(title_h * 0.75)
        font = ImageFont.truetype("/System/Library/Fonts/HelveticaNeue.ttc", plus_font_size)
    except:
        font = ImageFont.load_default()
    plus_text = "+"
    bbox = draw.textbbox((0, 0), plus_text, font=font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    tx = nx + (nw - tw) // 2
    ty = ny + (title_h - th) // 2 - bbox[1]
    draw.text((tx, ty), plus_text, fill=(220, 225, 255), font=font)

    # -- Pin dimensions --
    pin_r = max(2, int(s * 0.022))
    pin_outer_r = pin_r + max(1, s // 200)
    wire_w = max(1, int(s * 0.012))

    # Colors matching the actual program
    in_color = (51, 128, 230)    # blue inputs (rgb 0.2, 0.5, 0.9)
    out_color = (230, 77, 51)    # red-orange output (rgb 0.9, 0.3, 0.2)

    # -- Input pins on TOP of node --
    # 2 inputs evenly spaced across node width
    in_x1 = nx + int(nw * 0.33)
    in_x2 = nx + int(nw * 0.67)
    pin_y_top = ny  # top edge of node

    # -- Output pin on BOTTOM of node --
    out_x = nx + nw // 2
    pin_y_bot = ny + nh  # bottom edge of node

    # -- Input wires (from top of icon down to pins) --
    wire_start_y = int(s * 0.08)
    # Bezier-like vertical wires: just straight vertical for the icon
    draw.line([(in_x1, wire_start_y), (in_x1, pin_y_top)], fill=in_color, width=wire_w)
    draw.line([(in_x2, wire_start_y), (in_x2, pin_y_top)], fill=in_color, width=wire_w)

    # -- Output wire (from pin down to bottom area) --
    wire_end_y = int(s * 0.80)
    draw.line([(out_x, pin_y_bot), (out_x, wire_end_y)], fill=out_color, width=wire_w)

    # -- Input pin circles (on top edge) --
    for px in (in_x1, in_x2):
        draw.ellipse(
            [px - pin_outer_r, pin_y_top - pin_outer_r, px + pin_outer_r, pin_y_top + pin_outer_r],
            fill=(40, 45, 55), outline=in_color, width=max(1, s // 200)
        )
        draw.ellipse(
            [px - pin_r, pin_y_top - pin_r, px + pin_r, pin_y_top + pin_r],
            fill=in_color
        )

    # -- Output pin circle (on bottom edge) --
    draw.ellipse(
        [out_x - pin_outer_r, pin_y_bot - pin_outer_r, out_x + pin_outer_r, pin_y_bot + pin_outer_r],
        fill=(40, 45, 55), outline=out_color, width=max(1, s // 200)
    )
    draw.ellipse(
        [out_x - pin_r, pin_y_bot - pin_r, out_x + pin_r, pin_y_bot + pin_r],
        fill=out_color
    )

    # -- Pin labels inside node body --
    try:
        label_font_size = max(8, int(nh * 0.16))
        label_font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", label_font_size)
    except:
        label_font = ImageFont.load_default()
    label_color = (160, 165, 185)
    label_margin = int(s * 0.02)

    # Input labels just below the top edge, inside the body area
    body_top = ny + title_h
    for label, lx in [("a", in_x1), ("b", in_x2)]:
        bb = draw.textbbox((0, 0), label, font=label_font)
        lw = bb[2] - bb[0]
        draw.text((lx - lw // 2, body_top + label_margin - bb[1]), label, fill=label_color, font=label_font)

    # Output label just above the bottom edge
    out_label = "out"
    bb = draw.textbbox((0, 0), out_label, font=label_font)
    lw = bb[2] - bb[0]
    lh = bb[3] - bb[1]
    draw.text((out_x - lw // 2, pin_y_bot - label_margin - lh - bb[1]), out_label, fill=label_color, font=label_font)

    # -- App name at bottom --
    try:
        name_font_size = int(s * 0.085)
        name_font = ImageFont.truetype("/System/Library/Fonts/HelveticaNeue.ttc", name_font_size)
    except:
        name_font = ImageFont.load_default()
    app_name = "phograph"
    bb = draw.textbbox((0, 0), app_name, font=name_font)
    tw = bb[2] - bb[0]
    text_x = (s - tw) // 2
    text_y = int(s * 0.85)
    draw.text((text_x, text_y), app_name, fill=(160, 170, 210), font=name_font)

    return img


out_dir = os.path.join(os.path.dirname(__file__), "..", "Phograph", "Assets.xcassets", "AppIcon.appiconset")

# All required sizes: (filename, pixel_size)
mac_sizes = [
    ("icon_16x16.png", 16),
    ("icon_16x16@2x.png", 32),
    ("icon_32x32.png", 32),
    ("icon_32x32@2x.png", 64),
    ("icon_128x128.png", 128),
    ("icon_128x128@2x.png", 256),
    ("icon_256x256.png", 256),
    ("icon_256x256@2x.png", 512),
    ("icon_512x512.png", 512),
    ("icon_512x512@2x.png", 1024),
]

ios_sizes = [
    ("ios_20x20@2x.png", 40),
    ("ios_20x20@3x.png", 60),
    ("ios_29x29@2x.png", 58),
    ("ios_29x29@3x.png", 87),
    ("ios_40x40@2x.png", 80),
    ("ios_40x40@3x.png", 120),
    ("ios_60x60@2x.png", 120),
    ("ios_60x60@3x.png", 180),
    ("ios_76x76.png", 76),
    ("ios_76x76@2x.png", 152),
    ("ios_83.5x83.5@2x.png", 167),
    ("ios_1024x1024.png", 1024),
]

all_sizes = mac_sizes + ios_sizes

# Render at 1024 and downscale for quality
master = draw_icon(1024)

for filename, px in all_sizes:
    path = os.path.join(out_dir, filename)
    if px == 1024:
        master.save(path, "PNG")
    else:
        resized = master.resize((px, px), Image.LANCZOS)
        resized.save(path, "PNG")
    print(f"  {filename} ({px}x{px})")

print(f"\nDone — {len(all_sizes)} icons generated.")
