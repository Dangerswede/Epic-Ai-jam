"""
Surface texture builder (runs outside Unreal: plain Python 3 + numpy + Pillow).

For each surface in ArtSource/Surfaces/ writes T_Surf<Name>_BaseColor.png, _Normal.png (DirectX), _ORM.png
(R = AO, G = Roughness, B = Metallic) at 1024 px, all seamlessly tileable. Sources, per surface:
  1. ArtSource/Surfaces/Raw/<Name>/  - a PBR set (e.g. from PBRgen): files whose names contain
     color/albedo/diffuse, normal, rough, ao/occlusion, metal. Normal is assumed OpenGL unless the name
     contains "dx"/"directx". Missing maps are derived from the color image.
  2. ArtSource/Surfaces/Raw/<Name>.png|.jpg - a single color image (AI-generated); made tileable, the
     other maps are derived from it.
  3. Otherwise a procedural fallback (Wood, Metal, Asphalt, Painted, Concrete, Plywood, Brick).
Then run Scripts/setup_surfaces.py in the editor to import and assign them.

  python Scripts/make_surface_textures.py [Name ...]
"""
import os
import sys
import numpy as np
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SURF = os.path.join(ROOT, "ArtSource", "Surfaces")
RAW = os.path.join(SURF, "Raw")
N = 1024
# name: (normal strength, roughness base, roughness from luminance, metallic)
PARAMS = {
    "Concrete": (3.0, 0.75, 0.2, 0.0),
    "Plywood":  (2.0, 0.70, 0.15, 0.0),
    "Brick":    (5.0, 0.85, 0.1, 0.0),
    "Wood":     (4.0, 0.55, 0.25, 0.0),
    "Metal":    (3.0, 0.35, 0.25, 1.0),
    "Asphalt":  (4.0, 0.85, 0.1, 0.0),
    "Painted":  (1.5, 0.80, 0.1, 0.0),
}
rng = np.random.default_rng(1977)


# ---------------------------------------------------------------------------- helpers (all wrap-around)
def blur(a, r):
    """Separable wrap-around box blur, 3 passes ~ gaussian."""
    if r < 1:
        return a
    for _ in range(3):
        for ax in (0, 1):
            c = np.cumsum(np.concatenate([a.take(range(-r - 1, 0), axis=ax), a, a.take(range(0, r), axis=ax)], axis=ax), axis=ax)
            hi = c.take(range(2 * r + 1, c.shape[ax]), axis=ax)
            lo = c.take(range(0, c.shape[ax] - 2 * r - 1), axis=ax)
            a = (hi - lo) / (2 * r + 1)
    return a


def noise(scale, octaves=5, seed=None):
    """Tileable fractal value noise in [0, 1]: sum of upscaled random grids with wrap."""
    g = np.random.default_rng(seed) if seed is not None else rng
    out = np.zeros((N, N))
    amp, total = 1.0, 0.0
    cells = scale
    for _ in range(octaves):
        grid = g.random((cells, cells))
        img = Image.fromarray((grid * 255).astype(np.uint8))
        # wrap-pad so bicubic is seamless, then crop
        pad = np.pad(grid, 2, mode="wrap")
        big = np.array(Image.fromarray(pad.astype(np.float32), mode="F").resize(
            ((cells + 4) * N // cells, (cells + 4) * N // cells), Image.BICUBIC))
        o = 2 * N // cells
        out += amp * big[o:o + N, o:o + N]
        total += amp
        amp *= 0.5
        cells = min(cells * 2, N)
    out /= total
    return (out - out.min()) / (np.ptp(out) + 1e-6)


def lum(rgb):
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def make_tileable(rgb):
    """Offset-and-blend: edges come from the half-shifted copy, the middle from the original."""
    h, w = rgb.shape[:2]
    shifted = np.roll(rgb, (h // 2, w // 2), axis=(0, 1))
    y = np.abs(np.linspace(-1, 1, h))[:, None]
    x = np.abs(np.linspace(-1, 1, w))[None, :]
    d = np.maximum(x, y)
    m = np.clip((0.85 - d) / 0.45, 0, 1) ** 1.5
    if rgb.ndim == 3:
        m = m[..., None]
    return rgb * m + shifted * (1 - m)


def feather_seams(rgb, band=24):
    """For images that already repeat (periodic crop): cross-fade a thin band across each wrap seam."""
    out = rgb.copy()
    for ax in (0, 1):
        n = out.shape[ax]
        w = np.linspace(0, 1, 2 * band)
        lo = out.take(range(0, band), axis=ax)
        hi = out.take(range(n - band, n), axis=ax)
        seam = np.concatenate([hi, lo], axis=ax)           # pixels around the wrap
        mean = (seam + np.flip(seam, axis=ax)) * 0.5
        shape = [1, 1, 1][:out.ndim]
        shape[ax] = 2 * band
        k = (1 - np.abs(w * 2 - 1)).reshape(shape)          # 0 far from seam, 1 at seam
        blended = seam * (1 - k) + mean * k
        idx_hi, idx_lo = list(range(n - band, n)), list(range(0, band))
        if ax == 0:
            out[idx_hi], out[idx_lo] = blended[:band], blended[band:]
        else:
            out[:, idx_hi], out[:, idx_lo] = blended[:, :band], blended[:, band:]
    return out


def prep_brick(src):
    """AI brick image (8 rows, ~254 px per brick at 1280x720): crop exactly 3 bricks x 4 rows (one bond period
    each way), stack it twice vertically -> 8 rows per tile, so it repeats without blending."""
    im = Image.open(src).convert("RGB")
    a = np.asarray(im, dtype=np.float64)
    col = a.mean(axis=(1, 2))
    # row period from autocorrelation of the column profile
    c = col - col.mean()
    per = max(range(60, 130), key=lambda k: np.dot(c[:-k], c[k:]) / (len(c) - k))
    rowv = a.mean(axis=2)
    r = rowv.mean(axis=0) - rowv.mean()
    hper = max(range(200, 320), key=lambda k: np.dot(r[:-k], r[k:]) / (len(r) - k))
    y0 = int(np.argmin(np.abs(np.arange(len(col)) - per)))  # start at a mortar line ~1 row down
    y0 = per // 2
    crop = im.crop((100, y0, 100 + 3 * hper, y0 + 4 * per)).resize((N, N // 2), Image.LANCZOS)
    arr = np.asarray(crop, dtype=np.float64) / 255.0
    print("  brick periods: row %d px, brick %d px" % (per, hper))
    return np.concatenate([arr, arr], axis=0)


def normal_from_height(hgt, strength):
    """DirectX normal map (green = up in image)."""
    dx = (np.roll(hgt, -1, 1) - np.roll(hgt, 1, 1)) * 0.5 * strength * N / 256
    dy = (np.roll(hgt, -1, 0) - np.roll(hgt, 1, 0)) * 0.5 * strength * N / 256
    nx, ny, nz = -dx, dy, np.ones_like(hgt)
    l = np.sqrt(nx * nx + ny * ny + nz * nz)
    return np.stack([nx / l, ny / l, nz / l], -1) * 0.5 + 0.5


def ao_from_height(hgt):
    a = 1.0 - np.clip((blur(hgt, 12) - hgt) * 3.0, 0, 1)
    return np.clip(a, 0.35, 1)


def save(arr, name):
    path = os.path.join(SURF, name)
    Image.fromarray((np.clip(arr, 0, 1) * 255 + 0.5).astype(np.uint8)).save(path, optimize=True)
    print("  wrote", os.path.relpath(path, ROOT))


def load(path, mode="RGB"):
    return np.asarray(Image.open(path).convert(mode).resize((N, N), Image.LANCZOS), dtype=np.float64) / 255.0


# ---------------------------------------------------------------------------- procedural fallbacks
def proc_wood():
    planks = 6
    rows = (np.arange(N)[:, None] * planks) // N
    v = (np.arange(N)[:, None] * planks / N) % 1.0
    u = np.arange(N)[None, :] / N
    tone = rng.uniform(0.75, 1.15, planks)[rows]
    offs = rng.random(planks)[rows]
    grain = noise(4, 6)
    streak = np.sin((v * 9 + grain * 6 + offs * 20) * np.pi) * 0.5 + 0.5
    fine = noise(64, 3)
    hgt = 0.6 + 0.15 * streak + 0.1 * fine
    gap = (v < 0.025) | (v > 0.975)
    # butt joints at a random column per plank
    joint = np.abs(((u + offs) % 1.0) - 0.5) < 0.0025
    hgt = np.where(gap | joint, 0.0, hgt)
    base = np.array([0.42, 0.24, 0.12])
    col = base * (tone * (0.75 + 0.35 * streak + 0.15 * fine))[..., None]
    col = np.where((gap | joint)[..., None], col * 0.25, col)
    return col, hgt


def proc_metal():
    brushed = blur(np.random.default_rng(3).random((N, N)), 0)
    brushed = np.mean([np.roll(brushed, k, 1) for k in range(-24, 25, 4)], 0)  # horizontal streaks
    brushed = (brushed - brushed.min()) / np.ptp(brushed)
    blot = noise(4, 4)
    u = np.arange(N)[None, :] % (N // 2)
    v = np.arange(N)[:, None] % (N // 2)
    seam = (np.minimum(u, (N // 2) - u) < 3) | (np.minimum(v, (N // 2) - v) < 3)
    seam = np.broadcast_to(seam, (N, N))
    hgt = 0.5 + 0.05 * brushed
    hgt = np.where(seam, 0.2, hgt)
    # rivets
    for cx in range(0, N, N // 2):
        for t in range(16, N // 2, 48):
            for (px, py) in ((cx + 14, t), (cx + t, 14)):
                yy, xx = np.ogrid[:N, :N]
                dxw = (xx - px + N // 2) % N - N // 2
                dyw = (yy - py + N // 2) % N - N // 2
                d2 = dxw * dxw + dyw * dyw
                hgt = np.maximum(hgt, np.where(d2 < 36, 0.5 + 0.3 * np.sqrt(np.clip(1 - d2 / 36, 0, 1)), 0))
    g = 0.55 + 0.12 * brushed - 0.12 * blot
    col = np.stack([g, g * 1.01, g * 1.04], -1)
    col = np.where(seam[..., None], col * 0.45, col)
    return col, hgt


def proc_asphalt():
    grit = np.random.default_rng(5).random((N, N))
    grit = blur(grit, 1)
    grit = (grit - grit.min()) / np.ptp(grit)
    stains = noise(3, 5)
    hgt = grit * 0.8 + noise(32, 3) * 0.2
    g = 0.11 + 0.10 * grit - 0.05 * np.clip(stains - 0.55, 0, 1) * 4
    pebbles = (grit > 0.82)
    g = np.where(pebbles, g + 0.08, g)
    col = np.stack([g, g, g * 1.03], -1)
    return col, hgt


def proc_painted():
    roller = noise(48, 4)
    blot = noise(4, 5)
    hgt = roller * 0.6 + noise(128, 2) * 0.4
    g = 0.78 + 0.05 * roller - 0.025 * blot
    col = np.stack([g * 1.0, g * 0.98, g * 0.93], -1)
    return col, hgt


def proc_concrete():
    n = noise(6, 6)
    fine = noise(128, 2)
    hgt = n * 0.4 + fine * 0.6
    g = 0.38 + 0.12 * n + 0.04 * fine
    col = np.stack([g, g, g * 1.02], -1)
    return col, hgt


def proc_plywood():
    grain = noise(3, 6)
    streak = np.sin((np.arange(N)[None, :] / N * 14 + grain * 5) * np.pi) * 0.5 + 0.5
    hgt = 0.5 + 0.1 * streak + 0.1 * noise(64, 2)
    col = np.array([0.78, 0.62, 0.42]) * (0.85 + 0.2 * streak)[..., None]
    return col, hgt


def proc_brick():
    rows = 8
    bh = N // rows
    bw = N // 4
    y = np.arange(N)[:, None]
    x = np.arange(N)[None, :]
    r = y // bh
    xo = (x + (r % 2) * (bw // 2)) % N
    mortar = ((y % bh) < 10) | ((xo % bw) < 10)
    brick_id = r * 8 + xo // bw
    tone = rng.uniform(0.7, 1.15, rows * 8)[brick_id]
    n = noise(32, 4)
    hgt = np.where(mortar, 0.1, 0.7 + 0.2 * n)
    col = np.array([0.50, 0.20, 0.13]) * (tone * (0.8 + 0.35 * n))[..., None]
    col = np.where(mortar[..., None], np.array([0.45, 0.43, 0.40]) * (0.8 + 0.3 * n)[..., None], col)
    return col, hgt


PROC = {"Wood": proc_wood, "Metal": proc_metal, "Asphalt": proc_asphalt, "Painted": proc_painted,
        "Concrete": proc_concrete, "Plywood": proc_plywood, "Brick": proc_brick}


# ---------------------------------------------------------------------------- main
def find_raw_set(name):
    d = os.path.join(RAW, name)
    if not os.path.isdir(d):
        return None
    files = {}
    for f in sorted(os.listdir(d)):
        low = f.lower()
        if not low.endswith((".png", ".jpg", ".jpeg", ".tga")):
            continue
        p = os.path.join(d, f)
        if "normal" in low or low.endswith(("_n.png", "_nor.png")):
            files["normal"] = p
        elif "rough" in low:
            files["rough"] = p
        elif "occlusion" in low or "_ao" in low or low.startswith("ao"):
            files["ao"] = p
        elif "metal" in low:
            files["metal"] = p
        elif any(k in low for k in ("color", "albedo", "diffuse", "basecolor", "base_color")):
            files["color"] = p
        elif "height" in low or "disp" in low:
            files["height"] = p
    return files if "color" in files else None


def build(name):
    strength, rbase, rlum, metal = PARAMS.get(name, (3.0, 0.7, 0.2, 0.0))
    rawset = find_raw_set(name)
    single = next((os.path.join(RAW, name + e) for e in (".png", ".jpg", ".jpeg") if os.path.isfile(os.path.join(RAW, name + e))), None)
    normal = rough = ao = met = None
    if rawset:
        print(name, ": PBR set from", os.path.relpath(os.path.join(RAW, name), ROOT))
        col = load(rawset["color"])
        hgt = load(rawset["height"], "L") if "height" in rawset else blur(lum(col), 1)
        if "normal" in rawset:
            normal = load(rawset["normal"])
            if not any(k in os.path.basename(rawset["normal"]).lower() for k in ("dx", "directx")):
                normal[..., 1] = 1.0 - normal[..., 1]   # OpenGL -> DirectX
        rough = load(rawset["rough"], "L") if "rough" in rawset else None
        ao = load(rawset["ao"], "L") if "ao" in rawset else None
        met = load(rawset["metal"], "L") if "metal" in rawset else None
    elif name == "Brick" and os.path.isfile(os.path.join(RAW, "Brick_src.png")):
        print(name, ": periodic crop of Raw/Brick_src.png")
        col = feather_seams(prep_brick(os.path.join(RAW, "Brick_src.png")), 8)
        l = lum(col)
        hgt = blur(l, 2) - blur(l, 30) * 0.5
        hgt = (hgt - hgt.min()) / (np.ptp(hgt) + 1e-6)
    elif single:
        print(name, ": single image", os.path.relpath(single, ROOT), "-> tileable + derived maps")
        col = make_tileable(load(single))
        l = lum(col)
        hgt = l - blur(l, 24) * 0.7   # keep detail, drop big lighting gradients
        hgt = (hgt - hgt.min()) / (np.ptp(hgt) + 1e-6)
        # even out large-scale brightness differences in the AI image
        lowf = blur(l, 64)[..., None]
        col = np.clip(col * (lowf.mean() / (lowf + 1e-3)) ** 0.6, 0, 1)
    else:
        print(name, ": procedural")
        col, hgt = PROC[name]()
    if normal is None:
        normal = normal_from_height(hgt, strength)
    if rough is None:
        l = lum(col)
        rough = np.clip(rbase + rlum * (0.5 - (l - l.mean()) / (l.std() * 4 + 1e-6)), 0.05, 1)
    if ao is None:
        ao = ao_from_height(hgt)
    if met is None:
        met = np.full((N, N), metal)
    save(col, "T_Surf%s_BaseColor.png" % name)
    save(normal, "T_Surf%s_Normal.png" % name)
    save(np.stack([ao, rough, met], -1), "T_Surf%s_ORM.png" % name)


if __name__ == "__main__":
    for nm in (sys.argv[1:] or list(PARAMS)):
        build(nm)
