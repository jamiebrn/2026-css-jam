#!/usr/bin/env python3
"""Generates the island layout (ASCII terrain grid) from the sketch.

Everything is defined in *sketch pixel* coordinates (the 1536x1024 island
sketch) and converted to tiles with S/OX/OY, so you can nudge any area by
editing the numbers below and re-running:  python3 generate_island_layout.py
"""
import math, random, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage as ndi

W, H = 160, 110                 # map size in tiles
S = 140 / 1536.0                # tiles per sketch pixel
OX, OY = 10, 8                  # water margin (tiles) left/top of the sketch
OUT_DIR = sys.argv[1] if len(sys.argv) > 1 else "."
rng = np.random.default_rng(11)
YY, XX = np.mgrid[0:H, 0:W]

def T(p):    return (p[0] * S + OX, p[1] * S + OY)
def tile(px, py): return (int(math.floor(px * S + OX)), int(math.floor(py * S + OY)))

def smooth_noise(cx, cy, seed):
    r = np.random.default_rng(seed)
    a = (r.random((cy + 1, cx + 1)) * 255).astype(np.uint8)
    n = np.asarray(Image.fromarray(a).resize((W, H), Image.BICUBIC), dtype=np.float32) / 255.0
    return (n - 0.5) * 2

def poly_mask(pts, ss=4, blur=0.0):
    img = Image.new("L", (W * ss, H * ss), 0)
    ImageDraw.Draw(img).polygon([(T(p)[0] * ss, T(p)[1] * ss) for p in pts], fill=255)
    img = img.resize((W, H), Image.BOX)
    if blur: img = img.filter(ImageFilter.GaussianBlur(blur))
    return np.asarray(img, dtype=np.float32) / 255.0

def ellipse_mask(cx, cy, rx, ry, ss=4, blur=0.0):
    img = Image.new("L", (W * ss, H * ss), 0)
    c = T((cx, cy))
    ImageDraw.Draw(img).ellipse([(c[0] - rx * S) * ss, (c[1] - ry * S) * ss,
                                 (c[0] + rx * S) * ss, (c[1] + ry * S) * ss], fill=255)
    img = img.resize((W, H), Image.BOX)
    if blur: img = img.filter(ImageFilter.GaussianBlur(blur))
    return np.asarray(img, dtype=np.float32) / 255.0

def draw_path(pts, width=2, seed=0):
    r = np.random.default_rng(seed)
    P = [T(p) for p in pts]
    dense = [P[0]]
    for a, b in zip(P[:-1], P[1:]):
        n = max(1, int(math.dist(a, b) / 5))
        for i in range(1, n):
            t = i / n
            dense.append((a[0] + (b[0] - a[0]) * t + r.normal(0, 0.55),
                          a[1] + (b[1] - a[1]) * t + r.normal(0, 0.55)))
        dense.append(b)
    img = Image.new("L", (W, H), 0)
    ImageDraw.Draw(img).line(dense, fill=255, width=width, joint="curve")
    return np.asarray(img) > 0

# --------------------------------------------------------------------------
# Terrain codes (one character per tile)
#   ~ deep water   , shallow water   s sand        c cliff       g grass
#   f dark grass   T tree            p dirt path   k rock        M mountain
#   o stone floor  q mossy cobble    W stone wall  S stairs      E door/cave
#   l torch/candle r rune/portal     i pillar      b wooden dock B boat
#   x plane wreck  d debris
# --------------------------------------------------------------------------
g = np.full((H, W), "~", dtype="<U1")

# 1. Island silhouette -------------------------------------------------------
land_pts = [(140,250),(185,215),(260,185),(330,150),(400,130),(470,95),(540,45),(620,10),
 (700,-5),(780,-5),(850,40),(900,100),(960,160),(1030,185),(1100,215),(1200,240),(1290,290),
 (1370,360),(1430,430),(1480,510),(1505,600),(1490,690),(1440,770),(1370,835),(1280,890),
 (1180,935),(1080,955),(980,960),(880,950),(800,955),(700,965),(600,960),(500,945),(400,915),
 (320,870),(255,805),(210,730),(180,650),(160,585),(115,535),(60,495),(25,440),(40,380),(90,310)]
land = (poly_mask(land_pts, blur=1.4) + 0.30 * smooth_noise(16, 11, 1)) > 0.5
lab, n = ndi.label(land)
if n > 1:
    sizes = ndi.sum(land, lab, range(1, n + 1))
    land = lab == (1 + int(np.argmax(sizes)))
land = ndi.binary_fill_holes(land)
din = ndi.distance_transform_edt(land)
dout = ndi.distance_transform_edt(~land)

g[(~land) & (dout + 0.8 * smooth_noise(20, 14, 2) <= 4.5)] = ","
g[land] = "g"

# 2. Beach and cliffs ----------------------------------------------------------
beach_w = 3.2 + 1.2 * smooth_noise(18, 12, 3)
g[land & (din <= beach_w)] = "s"
cliff_zones = [
    [(120,500),(260,520),(420,700),(700,790),(700,1000),(150,1000),(100,700)],   # south-west
    [(1020,770),(1300,750),(1520,700),(1520,1000),(1020,1000)],                   # south-east
]
cz = np.max([poly_mask(z, blur=1.0) for z in cliff_zones], axis=0)
cliff_w = 3.6 + 0.9 * smooth_noise(18, 12, 4)
g[land & (cz > 0.5) & (din <= cliff_w)] = "c"

# 3. Forest zones (dark grass + trees) ------------------------------------------
forest_zones = [(400,210,150,90),(560,230,120,100),(870,190,100,120),(480,450,160,80),
                (620,400,90,60),(840,660,200,160),(1290,490,150,90),(1000,820,120,80),
                (1420,600,70,70)]
fz = ndi.gaussian_filter(np.max([ellipse_mask(*z) for z in forest_zones], axis=0), 1.5)
inland = land & (din > beach_w) & (g != "c")
g[inland & (fz > 0.45)] = "f"
p = fz + 0.30 * smooth_noise(26, 18, 5) + 0.22 * (rng.random((H, W)) * 2 - 1)
g[inland & (fz > 0.4) & (p > 0.95)] = "T"
g[(g == "g") & inland & (rng.random((H, W)) > 0.985)] = "T"      # lone trees in the open

# 4. Mountain ---------------------------------------------------------------------
mountain = [(455,200),(480,150),(520,95),(580,50),(650,15),(700,0),(760,0),(820,30),(880,100),
            (930,170),(960,215),(900,265),(840,272),(790,265),(730,250),(670,262),(610,268),
            (540,258),(490,235)]
g[land & (poly_mask(mountain, blur=0.6) > 0.5)] = "M"
cx, cy = tile(540, 490)                                    # small purple rock cave
g[land & (np.hypot(XX - cx, YY - cy) <= 3.2)] = "M"

# 5. Dirt paths ------------------------------------------------------------------------
paths = [
 [(340,380),(420,355),(500,345),(590,350),(680,345),(730,335)],            # crash site -> mountain
 [(310,420),(340,470),(360,520),(400,560),(430,585)],                      # crash site -> luminous room
 [(730,335),(760,380),(790,430),(800,500),(805,565)],                      # mountain -> forest ruins
 [(795,455),(860,490),(960,500),(1030,480),(1046,495)],                    # -> maze entrance
 [(1046,495),(1040,560),(1060,610),(1070,640)],                            # maze -> arena
 [(930,650),(1000,635),(1068,640)],                                        # forest ruins -> arena
 [(530,650),(600,655),(680,640),(735,655)],                                # luminous room -> forest ruins
 [(830,765),(805,790),(790,812),(772,835)],                                # forest ruins -> dock
 [(330,350),(350,290),(410,230),(470,190)],                                # crash site -> north-west woods
]
pm = np.zeros((H, W), bool)
for i, pts in enumerate(paths):
    pm |= draw_path(pts, 2, seed=i)
g[pm & land & ~np.isin(g, ["M", "c"])] = "p"

# 6. Plane crash site (spawn) -------------------------------------------------------------
patch = (ellipse_mask(285, 365, 135, 75, blur=1.0) + 0.25 * smooth_noise(14, 10, 6)) > 0.5
g[patch & land & (g != "c")] = "p"
wreck = ellipse_mask(270, 335, 85, 32) > 0.5
g[wreck & land] = "x"
g[patch & land & (g == "p") & (rng.random((H, W)) < 0.06)] = "d"
spawn = tile(330, 425)

# 7. Luminous room (word puzzle) ----------------------------------------------------------------
cx, cy = tile(430, 640)
d = np.hypot(XX - cx, YY - cy); ang = np.arctan2(YY - cy, XX - cx)
g[(d <= 7.2) & land] = "o"
ring = (d >= 8.2) & (d <= 9.6) & (ang < -0.15) & (ang > -math.pi + 0.15)
ring &= ~((np.abs(XX - cx) <= 1))                                   # doorway to the north
ring &= rng.random((H, W)) > 0.12                                   # crumbled gaps
g[ring & land] = "W"
for k in range(6):
    a = math.radians(60 * k)
    g[cy + round(6 * math.sin(a)), cx + round(6 * math.cos(a))] = "i"
    b = a + math.radians(30)
    g[cy + round(6.4 * math.sin(b)), cx + round(6.4 * math.cos(b))] = "l"
g[d <= 2.2] = "r"
luminous = (cx, cy)

# 8. Forest ruins (puzzle 2) ------------------------------------------------------------------------
cx, cy = tile(830, 660)
d = np.hypot(XX - cx, YY - cy)
cob = (d <= 9) & ((d <= 7) | (rng.random((H, W)) > 0.3))
g[cob & land] = "q"
rr = random.Random(5)
for _ in range(6):
    a = rr.uniform(0, 2 * math.pi); r_ = rr.uniform(4, 7.5)
    g[cy + round(r_ * math.sin(a)), cx + round(r_ * math.cos(a))] = "i"
for a in (0.5, 2.4, 4.4):                                           # broken wall pieces
    wx, wy = cx + round(8.6 * math.cos(a)), cy + round(8.6 * math.sin(a))
    g[wy, wx - 1:wx + 2] = "W"
for a in (math.radians(90), math.radians(210), math.radians(330)):  # puzzle objects
    g[cy + round(2.5 * math.sin(a)), cx + round(2.5 * math.cos(a))] = "r"
ruins = (cx, cy)
sx, sy = tile(790, 790)                                              # stairs down to the dock
g[sy:sy + 3, sx:sx + 2] = "S"

# 9. Mountain entrance (puzzle 1) -----------------------------------------------------------------------
x0, y0 = tile(640, 250); x1, y1 = tile(800, 285)
g[y0:y1 + 1, x0:x1 + 1] = "o"                                        # platform
ex, ey = tile(730, 225)
g[ey:ey + 2, ex - 1:ex + 2] = "E"                                    # cave mouth
g[y0 + 1, x0 + 4] = "l"; g[y0 + 1, x1 - 4] = "l"                     # torches
g[y1 + 1:y1 + 5, ex - 1:ex + 3] = "S"                                # stairs
mountain_door = (ex, ey)

# 10. Maze (puzzle 3) ---------------------------------------------------------------------------------------
cols, rows, pitch = 8, 6, 3
mw, mh = pitch * cols + 1, pitch * rows + 1
mx, my = tile(905, 262)
g[my:my + mh, mx:mx + mw] = "W"
mr = random.Random(21)
seen = {(0, 0)}; stack = [(0, 0)]
def carve_cell(c, r_):
    g[my + 1 + pitch * r_: my + 3 + pitch * r_, mx + 1 + pitch * c: mx + 3 + pitch * c] = "o"
carve_cell(0, 0)
while stack:
    c, r_ = stack[-1]
    nbrs = [(c + dc, r_ + dr) for dc, dr in ((1,0),(-1,0),(0,1),(0,-1))
            if 0 <= c + dc < cols and 0 <= r_ + dr < rows and (c + dc, r_ + dr) not in seen]
    if not nbrs:
        stack.pop(); continue
    nc, nr = mr.choice(nbrs)
    seen.add((nc, nr)); stack.append((nc, nr)); carve_cell(nc, nr)
    if nc != c:   # horizontal passage
        g[my + 1 + pitch * r_: my + 3 + pitch * r_, mx + 1 + pitch * max(c, nc) - 1] = "o"
    else:         # vertical passage
        g[my + 1 + pitch * max(r_, nr) - 1, mx + 1 + pitch * c: mx + 3 + pitch * c] = "o"
ecol = cols // 2
dx = mx + 1 + pitch * ecol
g[my + mh - 1, dx:dx + 2] = "E"
g[my + mh - 1, dx - 1] = "l"; g[my + mh - 1, dx + 2] = "l"
g[my + mh:my + mh + 3, dx:dx + 2] = "S"
maze = (mx, my, mw, mh)

# 11. Chronos's arena ---------------------------------------------------------------------------------------------
cx, cy = tile(1210, 640)
ax0, ax1, ay0, ay1 = cx - 13, cx + 12, cy - 10, cy + 9
g[ay0:ay1 + 1, ax0:ax1 + 1] = "W"
g[ay0 + 2:ay1 - 1, ax0 + 2:ax1 - 1] = "o"
g[cy - 2:cy + 2, ax0:ax0 + 2] = "E"                                  # west gate
g[cy - 3, ax0] = "l"; g[cy + 2, ax0] = "l"
for (px_, py_) in ((ax0+4, ay0+4),(ax1-4, ay0+4),(ax0+4, ay1-4),(ax1-4, ay1-4),(cx, ay0+4),(cx, ay1-4)):
    g[py_, px_] = "i"
d = np.hypot(XX - cx, YY - cy)
g[d <= 2.7] = "r"
for k in range(8):
    a = math.radians(45 * k)
    g[cy + round(6.5 * math.sin(a)), cx + round(6.5 * math.cos(a))] = "l"
arena = (ax0, ay0, ax1 - ax0 + 1, ay1 - ay0 + 1)

# 12. Dock and boat ------------------------------------------------------------------------------------------------
dx0, dy0 = tile(760, 835); _, dy1 = tile(760, 915)
g[dy0:dy1 + 1, dx0 - 1:dx0 + 2] = "b"
bx, by = tile(730, 968)
g[by:by + 2, bx - 1:bx + 2] = "B"

# 13. Rocks ---------------------------------------------------------------------------------------------------------------
for (px_, py_) in [(45,425),(105,385),(95,590),(180,225),(360,150),(1380,625),(1240,930),(335,880),
                   (1040,170),(1440,540),(60,640),(1160,970),(560,990)]:
    tx, ty = tile(px_, py_)
    if g[ty, tx] in "~,":
        g[ty:ty + 2, tx:tx + 2] = "k"
free = np.isin(g, ["g", "f"])
g[free & (rng.random((H, W)) < 0.004)] = "k"

# --------------------------------------------------------------------------
# Checks
rows_txt = ["".join(r) for r in g]
for name, (rx, ry, rw, rh) in {"maze": maze, "arena": arena}.items():
    if not land[ry:ry + rh, rx:rx + rw].all():
        print("WARNING:", name, "touches water")
print("spawn tile", spawn, "| luminous", luminous, "| ruins", ruins, "| mountain door", mountain_door)
print("maze", maze, "| arena", arena)
from collections import Counter
print(dict(Counter("".join(rows_txt))))

# --------------------------------------------------------------------------
# C++ header
with open(f"{OUT_DIR}/island_layout.h", "w") as f:
    f.write("#pragma once\n\n// Generated by generate_island_layout.py. One character per tile, row by row.\n")
    f.write("// ~ deep water  , shallow water  s sand  c cliff  g grass  f dark grass  T tree  p dirt path\n")
    f.write("// k rock  M mountain  o stone floor  q mossy cobble  W stone wall  S stairs  E door/cave mouth\n")
    f.write("// l torch/candle  r rune/portal  i pillar  b wooden dock  B boat  x plane wreck  d debris\n\n")
    f.write(f"constexpr int ISLAND_WIDTH = {W};\nconstexpr int ISLAND_HEIGHT = {H};\n")
    f.write(f"constexpr int ISLAND_SPAWN_X = {spawn[0]};   // tile where the player starts (plane crash site)\n")
    f.write(f"constexpr int ISLAND_SPAWN_Y = {spawn[1]};\n\n")
    f.write("static const char* const ISLAND_LAYOUT[ISLAND_HEIGHT] = {\n")
    for r in rows_txt:
        f.write(f'    "{r}",\n')
    f.write("};\n")

# --------------------------------------------------------------------------
# Preview image
C = {"~":(8,14,40),",":(26,56,104),"s":(196,170,120),"c":(96,72,116),"g":(62,104,56),"f":(30,64,44),
     "T":(30,64,44),"p":(156,120,78),"k":(120,120,130),"M":(92,70,126),"o":(112,112,128),"q":(84,104,82),
     "W":(54,54,76),"S":(176,166,150),"E":(255,190,80),"l":(255,214,60),"r":(176,96,255),"i":(160,160,180),
     "b":(124,88,52),"B":(170,116,72),"x":(214,214,220),"d":(132,112,92)}
TS = 8; M = 26
img = Image.new("RGB", (W * TS + M, H * TS + M), (20, 20, 24))
dr = ImageDraw.Draw(img)
for y in range(H):
    for x in range(W):
        ch = g[y, x]
        X, Y = M + x * TS, M + y * TS
        dr.rectangle([X, Y, X + TS - 1, Y + TS - 1], fill=C[ch])
        if ch == "T":
            dr.ellipse([X, Y - 1, X + TS - 1, Y + TS - 1], fill=(14, 44, 30), outline=(52, 100, 62))
font = ImageFont.load_default()
for t in range(0, W + 1, 10):
    dr.line([M + t * TS, M, M + t * TS, M + H * TS], fill=(255, 255, 255), width=1) if t % 50 == 0 else None
    dr.text((M + t * TS - 6, 8), str(t), fill=(200, 200, 200), font=font)
for t in range(0, H + 1, 10):
    dr.text((2, M + t * TS - 5), str(t), fill=(200, 200, 200), font=font)
ov = Image.new("RGBA", img.size, (0, 0, 0, 0)); od = ImageDraw.Draw(ov)
for t in range(0, W + 1, 10): od.line([M + t * TS, M, M + t * TS, M + H * TS], fill=(255, 255, 255, 38))
for t in range(0, H + 1, 10): od.line([M, M + t * TS, M + W * TS, M + t * TS], fill=(255, 255, 255, 38))
img = Image.alpha_composite(img.convert("RGBA"), ov).convert("RGB"); dr = ImageDraw.Draw(img)
def label(px, py, txt):
    x, y = tile(px, py); X, Y = M + x * TS, M + y * TS
    for ox_, oy_ in ((-1,0),(1,0),(0,-1),(0,1)): dr.text((X + ox_, Y + oy_), txt, fill=(0,0,0), font=font)
    dr.text((X, Y), txt, fill=(255, 255, 255), font=font)
for px_, py_, t in [(150,455,"1 Plane crash (spawn)"),(600,110,"Mountain entrance"),(1000,250,"Maze"),
                    (330,720,"Luminous room"),(760,760,"Forest ruins"),(1100,730,"Chronos arena"),(700,975,"Dock")]:
    label(px_, py_, t)
sx_, sy_ = M + spawn[0] * TS + TS // 2, M + spawn[1] * TS + TS // 2
dr.line([sx_ - 7, sy_ - 7, sx_ + 7, sy_ + 7], fill=(255, 40, 40), width=2)
dr.line([sx_ - 7, sy_ + 7, sx_ + 7, sy_ - 7], fill=(255, 40, 40), width=2)
img.save(f"{OUT_DIR}/island_layout_preview.png")
print("done")
