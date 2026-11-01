"""Build independent Bedrock packs with Python 3; no third-party dependencies."""
from pathlib import Path
import json
import struct
import zlib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = [1, 0, 0]


def write_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')


def png(path, width, height, pixel):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    rows = b''.join(b'\0' + b''.join(bytes(pixel(x, y)) for x in range(width)) for y in range(height))
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def manifest(name, description, header, module, pbr=False):
    result = {'format_version': 2, 'header': {'name': name, 'description': description,
              'uuid': header, 'version': VERSION, 'min_engine_version': [1, 21, 90]},
              'modules': [{'type': 'resources', 'uuid': module, 'version': VERSION}]}
    if pbr:
        result['capabilities'] = ['pbr']
    return result


def package(path):
    output = ROOT / 'dist' / (path.name + '.mcpack')
    output.parent.mkdir(exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
        for file in sorted(path.rglob('*')):
            if file.is_file():
                info = zipfile.ZipInfo(file.relative_to(path).as_posix(), (2026, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o644 << 16
                archive.writestr(info, file.read_bytes())
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        assert 'manifest.json' in archive.namelist()
    print(f'Built {output.relative_to(ROOT)} ({output.stat().st_size} bytes)')


def main():
    xray = ROOT / 'packs' / 'Kitchen-Xray'
    fullbright = ROOT / 'packs' / 'Kitchen-Fullbright-VV'
    write_json(xray / 'manifest.json', manifest('Kitchen X-ray',
        'Transparent natural terrain; ores retain vanilla textures. Enclosed faces may be culled.',
        '8876ef44-b1ab-4ce5-b22f-3ed383d9e8f4', 'a7cd3d67-8bd8-4ab8-a4b6-e457819deeba'))
    # Resource-pack block texture overrides, not behavior-pack block definitions.
    blocks = '''stone deepslate granite diorite andesite dirt coarse_dirt dirt_with_roots
    grass grass_block podzol mycelium gravel sand red_sand sandstone red_sandstone
    clay hardened_clay stained_hardened_clay mud packed_mud tuff calcite
    dripstone_block netherrack basalt smooth_basalt blackstone soul_sand soul_soil
    crimson_nylium warped_nylium end_stone infested_stone infested_deepslate'''.split()
    # grass is Bedrock's vanilla resource-pack key; grass_block is a Java name.
    blocks.remove('grass_block')
    write_json(xray / 'blocks.json', {'format_version': [1, 1, 0],
        **{name: {'textures': 'kitchen_xray_clear'} for name in blocks}})
    write_json(xray / 'textures' / 'terrain_texture.json', {
        'resource_pack_name': 'Kitchen X-ray', 'texture_name': 'atlas.terrain',
        'padding': 8, 'num_mip_levels': 4,
        'texture_data': {'kitchen_xray_clear': {'textures': 'textures/blocks/kitchen_xray_clear'}}})
    png(xray / 'textures' / 'blocks' / 'kitchen_xray_clear.png', 16, 16, lambda x, y: (0, 0, 0, 0))
    write_json(fullbright / 'manifest.json', manifest('Kitchen Fullbright (Vibrant Visuals)',
        'Raised white ambient lighting. Requires Vibrant Visuals; inactive in Simple/Fancy graphics.',
        '41675e76-31de-4fd0-9bb0-570696d1db72', 'b0810756-e57d-40e3-bc5c-5b395a67d51f', pbr=True))
    write_json(fullbright / 'lighting' / 'global.json', {
        'format_version': '1.21.80', 'minecraft:lighting_settings': {
            'description': {'identifier': 'minecraft:default_lighting'},
            'ambient': {'color': '#FFFFFF', 'illuminance': 1.0}}})
    nightvision = ROOT / 'packs' / 'Kitchen-Night-Vision'
    write_json(nightvision / 'manifest.json', {
        'format_version': 2,
        'header': {
            'name': 'Kitchen Night Vision (No Vibrant Visuals)',
            'description': 'Behavior pack: refreshes Night Vision for all players in this world.',
            'uuid': '3b50601b-92e8-4ce7-9a16-8e495fb76a93',
            'version': VERSION, 'min_engine_version': [1, 21, 90]},
        'modules': [{'type': 'script', 'language': 'javascript',
                     'uuid': 'ed74fe13-41b7-46c7-b284-c8b49991f38f',
                     'version': VERSION, 'entry': 'scripts/main.js'}],
        'dependencies': [{'module_name': '@minecraft/server', 'version': '2.0.0'}]})
    script = nightvision / 'scripts' / 'main.js'
    script.parent.mkdir(parents=True, exist_ok=True)
    script.write_bytes((ROOT / 'src' / 'night-vision' / 'main.js').read_bytes())
    png(nightvision / 'pack_icon.png', 64, 64,
        lambda x, y: (140, 240, 90, 255) if 12 <= x < 52 and 12 <= y < 52 else (24, 28, 38, 255))
    package(nightvision)
    for pack, color in [(xray, (40, 220, 230)), (fullbright, (255, 215, 70))]:
        png(pack / 'pack_icon.png', 64, 64,
            lambda x, y: (*color, 255) if 12 <= x < 52 and 12 <= y < 52 else (24, 28, 38, 255))
        package(pack)


if __name__ == '__main__':
    main()
