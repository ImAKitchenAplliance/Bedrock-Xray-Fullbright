"""Regenerate visibility textures from a local Mojang bedrock-samples checkout.
Usage: python3 tools/build-visibility.py /path/to/bedrock-samples
Requires Pillow 12.3.0. Normal packaging needs only tools/build.py.
"""
from pathlib import Path
import json
import sys
from PIL import Image
from build import ROOT, manifest, write_json, png, package

source = Path(sys.argv[1])
bright = ROOT / 'packs/Kitchen-Brighter-Blocks'
ores = ROOT / 'packs/Kitchen-Solid-Ores'
write_json(bright / 'manifest.json', manifest('Kitchen Brighter Blocks',
    'Lighter vanilla block colors. Standard graphics; does not bypass game lighting.',
    '3824842f-1630-44c3-8c31-3356794a2d51', '72456495-2fcb-4495-a7cb-66889ebd104b'))
write_json(ores / 'manifest.json', manifest('Kitchen Solid Ores',
    'High-visibility solid-color ores. Blue diamond, pink ancient debris. Not emissive.',
    '6df9a33a-0ce7-4ee8-9faa-1561b105b9a1', '3f390f9e-1631-4d0c-be13-b07d44df009b'))
count = 0
for path in sorted((source / 'resource_pack/textures/blocks').rglob('*')):
    if path.suffix not in ('.png', '.tga') or path.stem.endswith(('_mer', '_mers', '_normal', '_heightmap')):
        continue
    with Image.open(path) as original:
        image = original.convert('RGBA')
        red, green, blue, alpha = image.split()
        # Lift colors 40% toward white without changing alpha, frame layouts or size.
        lut = [round(c + (255 - c) * 0.4) for c in range(256)]
        output = Image.merge('RGBA', (red.point(lut), green.point(lut), blue.point(lut), alpha))
        target = bright / path.relative_to(source / 'resource_pack')
        target.parent.mkdir(parents=True, exist_ok=True)
        output.save(target)
    count += 1
assert count > 1000, 'Incomplete sample checkout'
palette = {
    'diamond': (30, 140, 255), 'emerald': (40, 255, 70),
    'gold': (255, 220, 30), 'iron': (255, 165, 95),
    'copper': (35, 255, 210), 'coal': (235, 235, 245),
    'redstone': (255, 35, 45), 'lapis': (165, 90, 255),
    'quartz': (255, 255, 255), 'netherite': (255, 75, 200),
}
# Direct texture replacements keep vanilla block definitions and ore drops intact.
paths = []
for kind, color in palette.items():
    if kind == 'netherite':
        names = ['ancient_debris_side', 'ancient_debris_top', 'netherite_block']
    elif kind == 'quartz':
        names = ['quartz_ore']
    else:
        names = [f'{kind}_ore', f'deepslate_{kind}_ore']
        if kind == 'gold':
            names += ['nether_gold_ore', 'gilded_blackstone']
    for name in names:
        # Fail rather than silently ship an ineffective misspelled override.
        originals = list((source / 'resource_pack/textures/blocks').rglob(name + '.*'))
        originals = [p for p in originals if p.suffix in ('.png', '.tga')]
        assert originals, f'Unknown vanilla texture: {name}'
        for original in originals:
            target = ores / original.relative_to(source / 'resource_pack')
            target.parent.mkdir(parents=True, exist_ok=True)
            with Image.open(original) as image:
                solid = Image.new('RGBA', image.size, (*color, 255))
                solid.save(target)
            paths.append(target.relative_to(ores).as_posix())
for pack, color in [(bright, (255, 245, 160)), (ores, (30, 140, 255))]:
    png(pack / 'pack_icon.png', 64, 64, lambda x, y: (*color, 255) if 12 <= x < 52 and 12 <= y < 52 else (24, 28, 38, 255))
    if pack == ores:
        package(pack)
write_json(ROOT / 'tools/visibility-report.json', {'block_textures': count, 'ore_textures': paths, 'palette': palette})
(bright / 'ASSET-NOTICE.txt').write_text(
    'Modified Minecraft sample textures. Original assets (c) Mojang AB. All rights reserved.\n'
    'Source: https://github.com/Mojang/bedrock-samples\n'
    'Snapshot: 15701fa9c13923f12662984c32c124327e40e3d3\n'
    'Use subject to Minecraft EULA: https://www.minecraft.net/en-us/eula\n')
package(bright)
print(f'Brightened {count} textures; generated {len(paths)} solid ore textures.')
