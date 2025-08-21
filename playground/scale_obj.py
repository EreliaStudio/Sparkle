#!/usr/bin/env python3
import argparse
import math
import re
from pathlib import Path
from typing import Tuple, Optional, List

Axis = Tuple[Optional[float], Optional[float], Optional[float]]

def parse_size(spec: str) -> Axis:
    """
    Parse a spec like '1/X/1' into (sx, sy, sz) where each item is either a float (constraint)
    or None (unconstrained).
    """
    parts = spec.strip().lower().replace('\\', '/').split('/')
    if len(parts) != 3:
        raise ValueError(f"Size spec must have 3 parts (X/Y/Z). Got: {spec!r}")
    out: List[Optional[float]] = []
    for p in parts:
        p = p.strip()
        if p in ('x', '*', 'any'):
            out.append(None)
        else:
            try:
                out.append(float(p))
            except ValueError:
                raise ValueError(f"Invalid size token {p!r} in {spec!r}. Use numbers or X.")
    return (out[0], out[1], out[2])

def compute_bbox(obj_lines: List[str]) -> Tuple[Tuple[float,float,float], Tuple[float,float,float]]:
    """
    Scan vertex lines 'v x y z' and compute (minx,miny,minz), (maxx,maxy,maxz).
    """
    minx = miny = minz =  math.inf
    maxx = maxy = maxz = -math.inf
    for ln in obj_lines:
        if ln.startswith('v '):
            # split with tolerance for extra spaces
            parts = ln.strip().split()
            if len(parts) < 4:
                continue
            try:
                x = float(parts[1]); y = float(parts[2]); z = float(parts[3])
            except ValueError:
                continue
            minx = min(minx, x); maxx = max(maxx, x)
            miny = min(miny, y); maxy = max(maxy, y)
            minz = min(minz, z); maxz = max(maxz, z)
    if not math.isfinite(minx):
        raise ValueError("No vertex lines found in OBJ (no 'v x y z').")
    return (minx, miny, minz), (maxx, maxy, maxz)

def safe_extent(minv: float, maxv: float) -> float:
    return maxv - minv

def choose_uniform_scale(ext: Tuple[float,float,float], target: Axis) -> float:
    """
    For each constrained axis, compute target/extent. Take the minimum of those ratios.
    Ignore unconstrained axes. If none are constrained, scale=1.
    If an extent is zero on a constrained axis and target>0, we cannot expand that axis
    by uniform scaling — treat its ratio as +inf (it doesn't limit).
    """
    ratios = []
    for e, t in zip(ext, target):
        if t is None:
            continue
        if e <= 0.0:
            # Zero extent: this axis doesn't constrain uniform scaling (ratio infinite)
            ratios.append(math.inf)
        else:
            ratios.append(t / e)
    if not ratios:
        return 1.0
    # we want the largest uniform scale that fits all constraints => min ratio
    s = min(ratios)
    # protect against degenerate negative or zero
    if s <= 0.0 or not math.isfinite(s):
        # If min is inf, it means no real constraint limited us; fallback 1.0
        if all(r == math.inf for r in ratios):
            return 1.0
        raise ValueError("Computed invalid uniform scale factor (<=0). Check size spec and model bbox.")
    return s

def choose_nonuniform_scales(ext: Tuple[float,float,float], target: Axis) -> Tuple[float,float,float]:
    """
    Per-axis scale: for constrained axes -> exact target/extent (or 1 if extent==0),
    for unconstrained axes -> 1 (leave unchanged).
    """
    scales = []
    for e, t in zip(ext, target):
        if t is None:
            scales.append(1.0)
        else:
            if e <= 0.0:
                # Cannot stretch a flat axis by finite scale; keep 1.0 and warn via exception?
                # We'll keep 1.0 to avoid division by zero and let the user decide if recentering is enough.
                scales.append(1.0)
            else:
                scales.append(t / e)
    return tuple(scales)  # type: ignore

def scale_and_optionally_center_vertices(
    obj_lines: List[str],
    scale: Tuple[float,float,float],
    center: bool,
    before_bbox: Tuple[Tuple[float,float,float], Tuple[float,float,float]]
) -> List[str]:
    """
    Apply scaling (sx, sy, sz) to all 'v' lines, and optionally recenter the model to origin (0,0,0) by
    subtracting the center of the *scaled* bbox.
    """
    sx, sy, sz = scale
    # First pass: scale and collect scaled bbox if we plan to center
    out_lines: List[str] = []
    scaled_min = [ math.inf,  math.inf,  math.inf]
    scaled_max = [-math.inf, -math.inf, -math.inf]

    # We'll store scaled vertices temporarily if centering, else write directly
    temp_vertices: List[Tuple[float,float,float]] = []

    for ln in obj_lines:
        if ln.startswith('v '):
            parts = ln.strip().split()
            if len(parts) < 4:
                out_lines.append(ln)
                continue
            try:
                x = float(parts[1]); y = float(parts[2]); z = float(parts[3])
            except ValueError:
                out_lines.append(ln)
                continue
            x *= sx; y *= sy; z *= sz
            temp_vertices.append((x,y,z))
            scaled_min[0] = min(scaled_min[0], x); scaled_max[0] = max(scaled_max[0], x)
            scaled_min[1] = min(scaled_min[1], y); scaled_max[1] = max(scaled_max[1], y)
            scaled_min[2] = min(scaled_min[2], z); scaled_max[2] = max(scaled_max[2], z)
        else:
            out_lines.append(ln)

    dx = dy = dz = 0.0
    if center and all(math.isfinite(v) for v in (*scaled_min, *scaled_max)):
        cx = (scaled_min[0] + scaled_max[0]) * 0.5
        cy = (scaled_min[1] + scaled_max[1]) * 0.5
        cz = (scaled_min[2] + scaled_max[2]) * 0.5
        dx, dy, dz = cx, cy, cz

    # Now write back the vertex lines with optional translation to center
    v_iter = iter(temp_vertices)
    new_lines: List[str] = []
    for ln in out_lines:
        new_lines.append(ln)

    # Reconstruct full sequence with scaled vertices in original order
    result: List[str] = []
    temp_idx = 0
    for ln in obj_lines:
        if ln.startswith('v '):
            x, y, z = temp_vertices[temp_idx]; temp_idx += 1
            if center:
                x -= dx; y -= dy; z -= dz
            # Preserve simple formatting
            result.append(f"v {x:.6f} {y:.6f} {z:.6f}\n")
        else:
            result.append(ln)
    return result

def main():
    ap = argparse.ArgumentParser(description="Scale an OBJ to fit inside a target size (e.g. 1/X/1).")
    ap.add_argument("input", type=Path, help="Path to input OBJ file")
    ap.add_argument("--size", required=True, help="Target size as X/Y/Z. Use X for unconstrained, e.g. 1/X/1")
    ap.add_argument("--output", type=Path, help="Path to output OBJ (default: alongside input with .scaled.obj)")
    ap.add_argument("--nonuniform", action="store_true",
                    help="Per-axis scaling: constrained axes match exact targets; unconstrained axes unchanged.")
    ap.add_argument("--center", action="store_true", help="Recenter the model to origin after scaling.")
    args = ap.parse_args()

    if not args.input.exists():
        raise SystemExit(f"Input file not found: {args.input}")

    size = parse_size(args.size)

    text = args.input.read_text(encoding="utf-8", errors="ignore")
    lines = [ln if ln.endswith('\n') else (ln + '\n') for ln in text.splitlines()]

    (minx, miny, minz), (maxx, maxy, maxz) = compute_bbox(lines)
    ext = (safe_extent(minx, maxx), safe_extent(miny, maxy), safe_extent(minz, maxz))

    if args.nonuniform:
        scales = choose_nonuniform_scales(ext, size)
    else:
        s = choose_uniform_scale(ext, size)
        scales = (s, s, s)

    new_lines = scale_and_optionally_center_vertices(lines, scales, args.center, ((minx,miny,minz), (maxx,maxy,maxz)))

    out_path = args.output if args.output else args.input.with_suffix('') .with_name(args.input.stem + '.scaled.obj')
    out_path.write_text(''.join(new_lines), encoding="utf-8")
    # Summary
    constrained = ['X' if v is None else str(v) for v in size]
    print(f"Input:  {args.input}")
    print(f"Output: {out_path}")
    print(f"Target size (X/Y/Z): {constrained}")
    print(f"Original extents: X={ext[0]:.6f}, Y={ext[1]:.6f}, Z={ext[2]:.6f}")
    if args.nonuniform:
        print(f"Applied per-axis scales: sx={scales[0]:.6f}, sy={scales[1]:.6f}, sz={scales[2]:.6f}")
    else:
        print(f"Applied uniform scale: s={scales[0]:.6f} (preserved proportions)")
    if args.center:
        print("Model was re-centered to origin.")

if __name__ == "__main__":
    main()
