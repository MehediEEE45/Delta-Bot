"""Off-device preview of the Delta-Bot animated face.

Mirrors the geometry in FaceAnimator::paramsFor() and FaceRenderer::drawEye()
so the expression shapes can be eyeballed without flashing hardware. This is a
one-off visual check, not a test -- it duplicates the drawing math rather than
sharing it.
"""
import math

W, H = 128, 64


class Canvas:
    def __init__(self):
        self.b = [[0] * W for _ in range(H)]

    def px(self, x, y, v=1):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < W and 0 <= y < H:
            self.b[y][x] = v

    def hline(self, x, y, w, v=1):
        for i in range(int(w)):
            self.px(x + i, y, v)

    def rect(self, x, y, w, h, v=1):
        for j in range(int(h)):
            self.hline(x, y + j, w, v)

    def round_rect(self, x, y, w, h, r, v=1):
        w, h = int(w), int(h)
        r = max(0, min(int(r), min(w, h) // 2))
        for j in range(h):
            for i in range(w):
                dx = dy = 0
                if i < r:
                    dx = r - i - 1
                elif i >= w - r:
                    dx = i - (w - r)
                if j < r:
                    dy = r - j - 1
                elif j >= h - r:
                    dy = j - (h - r)
                if dx * dx + dy * dy <= r * r:
                    self.px(x + i, y + j, v)

    def circle(self, cx, cy, r, v=1):
        for j in range(int(cy - r), int(cy + r) + 1):
            for i in range(int(cx - r), int(cx + r) + 1):
                if (i - cx) ** 2 + (j - cy) ** 2 <= r * r:
                    self.px(i, j, v)

    def line(self, x0, y0, x1, y1, v=1):
        n = int(max(abs(x1 - x0), abs(y1 - y0))) or 1
        for i in range(n + 1):
            self.px(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, v)

    def tri(self, x0, y0, x1, y1, x2, y2, v=1):
        def sign(ax, ay, bx, by, cx, cy):
            return (ax - cx) * (by - cy) - (bx - cx) * (ay - cy)
        xs = [x0, x1, x2]
        ys = [y0, y1, y2]
        for j in range(int(min(ys)), int(max(ys)) + 1):
            for i in range(int(min(xs)), int(max(xs)) + 1):
                d1 = sign(i, j, x0, y0, x1, y1)
                d2 = sign(i, j, x1, y1, x2, y2)
                d3 = sign(i, j, x2, y2, x0, y0)
                neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
                pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
                if not (neg and pos):
                    self.px(i, j, v)

    def arc(self, cx, cy, rx, ry, start_deg, sweep_deg, dotted=False):
        steps = max(2, int(abs(sweep_deg) / 3.0) * max(1, max(rx, ry) // 22))
        for i in range(steps + 1):
            if dotted and i % 2:
                continue
            a = math.radians(start_deg + sweep_deg * i / steps)
            self.px(cx + math.cos(a) * rx, cy + math.sin(a) * ry)

    def show(self):
        # Two vertical pixels per character row.
        glyphs = {(0, 0): " ", (1, 0): "▀", (0, 1): "▄", (1, 1): "█"}
        out = []
        for y in range(0, H, 2):
            out.append("".join(glyphs[(self.b[y][x], self.b[y + 1][x])] for x in range(W)))
        return "\n".join(out)


# --- mirrors FaceAnimator::paramsFor() -------------------------------------
D = dict(eyeW=27, eyeH=21, eyeRadius=6, eyeGap=66, eyeY=26,
         browLift=0, browAngle=0, mouthW=20, mouthCurve=1.5, mouthOpen=0, mouthY=48)


def P(**kw):
    p = dict(D)
    p.update(kw)
    return p


EMOTIONS = [
    ("Idle",      "Block",    P()),
    ("Happy",     "HappyArc", P(eyeH=16, eyeRadius=7, browLift=5, browAngle=2, mouthW=28, mouthCurve=6)),
    ("Love",      "Heart",    P(eyeW=24, eyeH=22, mouthW=20, mouthCurve=5)),
    ("Excited",   "Star",     P(eyeW=26, eyeH=24, eyeY=26, browLift=4, browAngle=3, mouthW=18, mouthOpen=11)),
    ("Cool",      "Shades",   P(eyeH=14, eyeRadius=3, mouthW=24, mouthCurve=3, mouthY=50)),
    ("Sad",       "Block",    P(eyeH=17, eyeY=29, browLift=5, browAngle=5, mouthW=22, mouthCurve=-6, mouthY=52)),
    ("Angry",     "Block",    P(eyeH=16, eyeRadius=3, browLift=5, browAngle=-5, mouthW=20, mouthCurve=-5)),
    ("Surprised", "Hollow",   P(eyeW=24, eyeH=26, eyeRadius=11, eyeY=27, browLift=4, browAngle=2, mouthW=14, mouthOpen=13, mouthY=49)),
    ("Sleep",     "Line",     P(eyeH=3, eyeRadius=1, eyeY=30, mouthW=0)),
]


def draw_eye(c, p, shape, cx, cy, gaze=(0.0, 0.0), openness=1.0):
    w = round(p["eyeW"])
    full = round(p["eyeH"])
    h = max(2, round(p["eyeH"] * openness))
    bottom = cy + full // 2
    top = bottom - h
    r = min(round(p["eyeRadius"]), min(w, h) // 2)

    if shape == "HappyArc":
        rx = w // 2
        ry = max(4, int(h * 0.62))
        for k in range(3):
            c.arc(cx, bottom - 1, rx - k, ry - k, 180.0, 180.0)
    elif shape == "Heart":
        hr = max(3, w // 4)
        c.circle(cx - hr + 1, top + hr, hr)
        c.circle(cx + hr - 1, top + hr, hr)
        c.tri(cx - 2 * hr + 1, top + hr + 1, cx + 2 * hr - 1, top + hr + 1, cx, bottom)
    elif shape == "Star":
        a, b = w // 2, h // 2
        c.tri(cx, cy - b, cx - a // 2, cy, cx + a // 2, cy)
        c.tri(cx, cy + b, cx - a // 2, cy, cx + a // 2, cy)
        c.tri(cx - a, cy, cx, cy - b // 2, cx, cy + b // 2)
        c.tri(cx + a, cy, cx, cy - b // 2, cx, cy + b // 2)
    elif shape == "Hollow":
        c.round_rect(cx - w // 2, top, w, h, r)
        iw, ih = max(2, w - 8), max(2, h - 8)
        c.round_rect(cx - iw // 2, top + (h - ih) // 2, iw, ih, max(1, r - 4), 0)
        c.circle(cx + gaze[0] * 3, cy + gaze[1] * 2, 2)
    elif shape == "Line":
        c.rect(cx - w // 2, cy, w, max(2, h))
    else:  # Block
        c.round_rect(cx - w // 2, top, w, h, r)
        pw, ph = max(4, w // 3), max(4, h // 3)
        px = cx + round(gaze[0] * (w / 2 - pw / 2 - 3))
        py = cy + round(gaze[1] * (h / 2 - ph / 2 - 3))
        c.round_rect(px - pw // 2, py - ph // 2, pw, ph, 2, 0)


def draw_brow(c, p, cx, cy, left):
    if p["browLift"] <= 0.5:
        return
    w = round(p["eyeW"])
    tilt = round(p["browAngle"])
    eye_top = cy - round(p["eyeH"] / 2)
    base = eye_top - round(p["browLift"])
    base = max(base, 1 + abs(tilt))
    base = min(base, eye_top - 2 - abs(tilt))
    if base < 1:
        return
    ix = cx + w // 2 if left else cx - w // 2
    ox = cx - w // 2 if left else cx + w // 2
    c.line(ix, base - tilt, ox, base + tilt)
    c.line(ix, base - tilt + 1, ox, base + tilt + 1)


def draw_mouth(c, p, cx, base):
    w = round(p["mouthW"])
    if w < 4:
        return
    if p["mouthOpen"] > 1:
        oh = round(p["mouthOpen"])
        c.round_rect(cx - w // 2, base - oh // 2, w, oh, min(w, oh) // 2)
        return
    for x in range(-w // 2, w // 2 + 1):
        n = (2.0 * x) / w
        y = base + round(p["mouthCurve"] * (1 - n * n))
        c.px(cx + x, y)
        c.px(cx + x, y + 1)


def render(name, shape, p, gaze=(0.0, 0.0), openness=1.0):
    c = Canvas()
    cy = round(p["eyeY"])
    half = round(p["eyeGap"] / 2)
    lx, rx = 64 - half, 64 + half
    if shape == "Shades":
        h = max(3, round(p["eyeH"] * openness))
        top = cy + round(p["eyeH"] / 2) - h
        w = round(p["eyeW"])
        c.round_rect(lx - w // 2, top, w, h, 3)
        c.round_rect(rx - w // 2, top, w, h, 3)
        c.rect(lx + w // 2, top + h // 3, rx - lx - w, 3)
    else:
        draw_eye(c, p, shape, lx, cy, gaze, openness)
        draw_eye(c, p, shape, rx, cy, gaze, openness)
        draw_brow(c, p, lx, cy, True)
        draw_brow(c, p, rx, cy, False)
    draw_mouth(c, p, 64, round(p["mouthY"]))
    return c


# --- mirrors FaceRenderer::showSplash() ------------------------------------
GLYPH = {
    "D": ["1110", "1001", "1001", "1001", "1110"],
    "E": ["1111", "1000", "1110", "1000", "1111"],
    "L": ["1000", "1000", "1000", "1000", "1111"],
    "T": ["1111", "0110", "0110", "0110", "0110"],
    "A": ["0110", "1001", "1111", "1001", "1001"],
    "0": ["1111", "1001", "1001", "1001", "1111"],
    "1": ["0010", "0110", "0010", "0010", "0111"],
    "2": ["1111", "0001", "1111", "1000", "1111"],
    "3": ["1111", "0001", "1111", "0001", "1111"],
    "4": ["1001", "1001", "1111", "0001", "0001"],
    "5": ["1111", "1000", "1111", "0001", "1111"],
    "6": ["1111", "1000", "1111", "1001", "1111"],
    "7": ["1111", "0001", "0010", "0100", "0100"],
    "8": ["1111", "1001", "1111", "1001", "1111"],
    "9": ["1111", "1001", "1111", "0001", "1111"],
    ":": ["0000", "0100", "0000", "0100", "0000"],
}


def ease_out_cubic(t):
    t = max(0.0, min(1.0, t))
    return 1.0 - (1.0 - t) ** 3


def stage(value, start, span):
    if span <= 0:
        return 1.0 if value >= start else 0.0
    return max(0.0, min(1.0, (value - start) / span))


def draw_glyph(c, ch, x, y, size=2):
    # Stand-in for the GFX font: blocky enough to check placement, not shape.
    rows = GLYPH.get(ch.upper())
    if not rows:
        return
    for j, row in enumerate(rows):
        for i, bit in enumerate(row):
            if bit == "1":
                c.rect(x + i * size, y + j * size, size, size)


def draw_delta_mark(c, cx, base_y, size, progress):
    if progress <= 0:
        return
    scale = ease_out_cubic(progress)
    half = round((size / 2.0) * scale)
    height = round(size * 0.88 * scale)
    if half < 1 or height < 1:
        return
    c.line(cx, base_y - height, cx - half, base_y)
    c.line(cx - half, base_y, cx + half, base_y)
    c.line(cx + half, base_y, cx, base_y - height)


def render_splash(t, name="DELTA", tagline="desk buddy"):
    c = Canvas()
    draw_delta_mark(c, 64, 20, 20, stage(t, 0.0, 0.28))

    name_width = len(name) * 12
    left = (128 - name_width) // 2
    for i, ch in enumerate(name):
        drop = ease_out_cubic(stage(t, 0.20 + i * 0.06, 0.30))
        y = 26 - round((1.0 - drop) * 42.0)
        draw_glyph(c, ch, left + i * 12, y)

    half = round(ease_out_cubic(stage(t, 0.60, 0.20)) * (name_width // 2))
    if half > 0:
        c.hline(64 - half, 45, half * 2)

    typed = int(stage(t, 0.76, 0.22) * len(tagline))
    for i in range(typed):
        draw_glyph(c, tagline[i], (128 - len(tagline) * 6) // 2 + i * 6, 51, size=1)
    return c


# --- mirrors FaceRenderer::drawTimeDateScreen() -----------------------------
def render_clock(hour, minute, second, use24=True):
    c = Canvas()
    cx, cy, rx, ry = 64, 24, 58, 17

    c.arc(cx, cy, rx, ry, 0, 360, dotted=True)
    sweep = 360.0 * (second / 60.0)
    if sweep > 0:
        c.arc(cx, cy, rx, ry, -90, sweep)
        c.arc(cx, cy, rx - 1, ry - 1, -90, sweep)
    head = math.radians(-90 + sweep)
    c.circle(cx + math.cos(head) * rx, cy + math.sin(head) * ry, 2)

    display_hour = hour if use24 else (hour % 12 or 12)
    time_text = f"{display_hour:02d}:{minute:02d}:{second:02d}"
    left = cx - 48
    for i, ch in enumerate(time_text):
        draw_glyph(c, ch, left + i * 12, cy - 8, size=2)

    if not use24:
        for i, ch in enumerate("PM" if hour >= 12 else "AM"):
            draw_glyph(c, ch, 104 + i * 4, 1, size=1)

    date_text = "FRI 28 AUG"
    left = (128 - len(date_text) * 6) // 2
    for i, ch in enumerate(date_text):
        draw_glyph(c, ch, left + i * 6, 55, size=1)
    return c


if __name__ == "__main__":
    import sys
    which = sys.argv[1] if len(sys.argv) > 1 else None

    if which and which.lower() == "splash":
        for t in (0.15, 0.35, 0.55, 0.75, 1.0):
            print(f"\n=== splash  t={t:.2f} " + "=" * 40)
            print(render_splash(t).show())
        raise SystemExit

    if which and which.lower() == "clock":
        for h, m, s, use24 in ((14, 7, 3, True), (14, 7, 45, False), (0, 0, 0, False)):
            print(f"\n=== clock  {h:02d}:{m:02d}:{s:02d}  {'24h' if use24 else '12h'} " + "=" * 20)
            print(render_clock(h, m, s, use24).show())
        raise SystemExit

    for name, shape, p in EMOTIONS:
        if which and which.lower() != name.lower():
            continue
        print(f"\n=== {name}  ({shape}) " + "=" * 40)
        print(render(name, shape, p).show())
