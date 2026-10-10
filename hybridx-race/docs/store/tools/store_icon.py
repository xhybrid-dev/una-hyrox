# Store-listing icon: the brand X mark (option A: lemon/cyan arms) over the
# "HybridX Race" wordmark (SemiBold + Light, as promo/lib/brand.mjs sets it).
import sys
from PIL import Image, ImageDraw, ImageFont
fonts, out = sys.argv[1], sys.argv[2]
W_, T, TIP, GAP, CUT = 1.3346, 0.3347, 0.4054, 0.0812, 0.3489
yc = (W_ - T - (T + GAP)) / 2
chev = [(0,0),(T,0),(T+TIP,TIP),(T+TIP,1-TIP),(T,1),(0,1),(0.5,0.5)]
armU = [(W_-T,0),(W_,0),(W_-CUT,CUT),(T+CUT+GAP,CUT),(T+yc+GAP,yc)]
armD = [(x,1-y) for x,y in armU]
INK=(7,8,10,255); WHITE=(255,255,255,255); LEMON=(255,255,85,255); CYAN=(85,255,255,255)

def make(size, bg):
    N = size*4
    im = Image.new('RGBA', (N,N), bg); d = ImageDraw.Draw(im)
    mh = N*0.36; mw = mh*W_
    ox, oy = (N-mw)/2, N*0.17
    P = lambda pts: [(ox+x*mh, oy+y*mh) for x,y in pts]
    d.polygon(P(chev), fill=WHITE); d.polygon(P(armU), fill=LEMON); d.polygon(P(armD), fill=CYAN)
    fs = int(N*0.125)
    semi = ImageFont.truetype(f'{fonts}/Poppins-SemiBold.ttf', fs)
    light = ImageFont.truetype(f'{fonts}/Poppins-Light.ttf', fs)
    a, b = 'HybridX', ' Race'
    wa = d.textlength(a, font=semi); wb = d.textlength(b, font=light)
    x = (N-(wa+wb))/2; y = N*0.66
    d.text((x,y), a, font=semi, fill=WHITE)
    d.text((x+wa,y), b, font=light, fill=WHITE)
    return im.resize((size,size), Image.LANCZOS)

for sz in (1024, 512):
    make(sz, INK).save(f'{out}/hybridx-race-icon-{sz}.png')
make(1024, (0,0,0,0)).save(f'{out}/hybridx-race-icon-1024-transparent.png')
