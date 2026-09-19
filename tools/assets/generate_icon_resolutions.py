"""Generate deterministic per-cell, area-averaged default icon variants (requires Pillow)."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[2]
source = Image.open(root / 'resources/textures/default_iconset.png').convert('RGBA')
assert source.size == (320, 320)
for size in (16, 8):
    atlas = Image.new('RGBA', (size * 10, size * 10))
    for y in range(10):
        for x in range(10):
            cell = source.crop((x * 32, y * 32, (x + 1) * 32, (y + 1) * 32))
            # Premultiplied alpha avoids dark fringes; separate cells prevent atlas bleed.
            cell = cell.convert('RGBa').resize((size, size), Image.Resampling.BOX).convert('RGBA')
            atlas.paste(cell, (x * size, y * size))
    atlas.save(root / f'resources/textures/default_iconset_{size}.png')
