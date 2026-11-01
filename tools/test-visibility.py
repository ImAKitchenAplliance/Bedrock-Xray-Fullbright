"""Validate generated textures against their originals. Requires Pillow and sample checkout."""
import json
from pathlib import Path
import sys
from PIL import Image, ImageChops
from build import ROOT

source = Path(sys.argv[1]) / 'resource_pack'
bright = ROOT / 'packs/Kitchen-Brighter-Blocks'
report = json.loads((ROOT / 'tools/visibility-report.json').read_text())
count = 0
for path in (bright / 'textures/blocks').rglob('*'):
    if path.suffix not in ('.png', '.tga'):
        continue
    with Image.open(source / path.relative_to(bright)) as original, Image.open(path) as generated:
        original, generated = original.convert('RGBA'), generated.convert('RGBA')
        assert original.size == generated.size, path
        assert ImageChops.difference(original.getchannel('A'), generated.getchannel('A')).getbbox() is None, path
        for channel in 'RGB':
            assert ImageChops.subtract(original.getchannel(channel), generated.getchannel(channel)).getbbox() is None, path
    count += 1
assert count == report['block_textures'] and count > 1000
for name in report['ore_textures']:
    with Image.open(ROOT / 'packs/Kitchen-Solid-Ores' / name) as image:
        colors = image.convert('RGBA').getcolors(2)
        assert colors is not None and len(colors) == 1 and colors[0][1][3] == 255, name
        assert list(colors[0][1][:3]) in report['palette'].values(), name
print(f'PASS {count} brighter textures: dimensions, alpha preservation, non-darkening; {len(report["ore_textures"])} solid-color ore textures')

for name, color in [('diamond_ore', (30, 140, 255, 255)), ('deepslate/deepslate_diamond_ore', (30, 140, 255, 255)), ('ancient_debris_top', (255, 75, 200, 255)), ('ancient_debris_side', (255, 75, 200, 255))]:
    with Image.open(ROOT / 'packs/Kitchen-Solid-Ores/textures/blocks' / (name + '.png')) as image:
        assert image.convert('RGBA').getpixel((0, 0)) == color, name
print('PASS blue diamonds and pink ancient debris on both face textures')
