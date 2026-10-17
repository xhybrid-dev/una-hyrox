# Put a simulator capture into UNA's watch render, on a brand-ink canvas,
# with an optional caption. Usage: mockup.py <una-dir> <fonts> <screen.png> <out.png> [caption] [sub]
import json, sys
from PIL import Image, ImageDraw, ImageFont
una, fonts, screen, out = sys.argv[1:5]
cap = sys.argv[5] if len(sys.argv) > 5 else ''
sub = sys.argv[6] if len(sys.argv) > 6 else ''
STRAP = 'graphite'
meta = json.load(open(f'{una}/meta.json'))[STRAP]
watch = Image.open(f'{una}/{STRAP}.png').convert('RGBA')
cx, cy, r = meta['cx'], meta['cy'], meta['r']

# The screen, round, under the render's cut-out.
d = int(round(2 * r)) + 4
scr = Image.open(screen).convert('RGBA').resize((d, d), Image.LANCZOS)
mask = Image.new('L', (d * 4, d * 4), 0)
ImageDraw.Draw(mask).ellipse([0, 0, d * 4 - 1, d * 4 - 1], fill=255)
scr.putalpha(mask.resize((d, d), Image.LANCZOS))
base = Image.new('RGBA', watch.size, (0, 0, 0, 0))
base.alpha_composite(scr, (int(round(cx - d / 2)), int(round(cy - d / 2))))
base.alpha_composite(watch)

# Canvas: 1080 x 1350 (4:5), ink, watch centred in the upper part.
W, H = 1080, 1350
canvas = Image.new('RGBA', (W, H), (7, 8, 10, 255))
wh = 1060 if cap else 1250
ww = int(base.width * wh / base.height)
wimg = base.resize((ww, wh), Image.LANCZOS)
top = 30 if cap else (H - wh) // 2
canvas.alpha_composite(wimg, ((W - ww) // 2, top))
if cap:
    dr = ImageDraw.Draw(canvas)
    f1 = ImageFont.truetype(f'{fonts}/Poppins-SemiBold.ttf', 58)
    f2 = ImageFont.truetype(f'{fonts}/Poppins-Light.ttf', 34)
    w1 = dr.textlength(cap, font=f1)
    dr.text(((W - w1) / 2, 1120), cap, font=f1, fill=(255, 255, 255, 255))
    if sub:
        w2 = dr.textlength(sub, font=f2)
        dr.text(((W - w2) / 2, 1205), sub, font=f2, fill=(155, 161, 168, 255))
canvas.convert('RGB').save(out, quality=92)
