"""Validate deliverable structure, resource resolution, alpha and separate identities."""
from pathlib import Path
import json
import struct
import uuid
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
identities = set()
for path in sorted((ROOT / 'dist').glob('*.mcpack')):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None, path
        names = set(archive.namelist())
        assert 'manifest.json' in names
        assert all(not n.startswith('/') and '..' not in Path(n).parts for n in names)
        for name in names:
            if name.endswith('.json'):
                json.loads(archive.read(name))
        manifest = json.loads(archive.read('manifest.json'))
        for entry in [manifest['header'], *manifest['modules']]:
            identity = str(uuid.UUID(entry['uuid']))
            assert identity not in identities, 'Pack/module identity collision'
            identities.add(identity)
        module = manifest['modules'][0]
        assert module['type'] in ('resources', 'script')
        if module['type'] == 'script':
            assert module['entry'] in names
            assert archive.read(module['entry']) == (ROOT / 'src/night-vision/main.js').read_bytes()
            assert manifest['dependencies'] == [{'module_name': '@minecraft/server', 'version': '2.0.0'}]
            assert 'capabilities' not in manifest
            print(f'PASS {path.name}: archive, JSON, identities, and script entry')
            continue
        if 'blocks.json' in names:
            blocks = json.loads(archive.read('blocks.json'))
            assert len(blocks) > 30
            assert not any('ore' in name or name in ('ancient_debris', 'chest', 'lava', 'water', 'bedrock')
                           for name in blocks if name != 'format_version')
            atlas = json.loads(archive.read('textures/terrain_texture.json'))['texture_data']
            for block, definition in blocks.items():
                if block == 'format_version':
                    continue
                texture = atlas[definition['textures']]['textures'] + '.png'
                assert texture in names, texture
            png = archive.read(texture)
            assert png[:8] == b'\x89PNG\r\n\x1a\n'
            pos, compressed = 8, b''
            while pos < len(png):
                size = struct.unpack('>I', png[pos:pos + 4])[0]
                kind, data = png[pos + 4:pos + 8], png[pos + 8:pos + 8 + size]
                crc = struct.unpack('>I', png[pos + 8 + size:pos + 12 + size])[0]
                assert zlib.crc32(kind + data) & 0xffffffff == crc
                if kind == b'IDAT':
                    compressed += data
                pos += size + 12
            pixels = zlib.decompress(compressed)
            assert len(pixels) == 16 * (1 + 16 * 4)
            assert not any(pixels), 'Terrain texture is not fully transparent'
        elif 'lighting/global.json' not in names:
            textures = [n for n in names if n.startswith('textures/blocks/') and n.endswith(('.png', '.tga'))]
            assert len(textures) >= 20
            assert 'capabilities' not in manifest and 'dependencies' not in manifest
        else:
            assert 'pbr' in manifest['capabilities']
            lighting = json.loads(archive.read('lighting/global.json'))['minecraft:lighting_settings']
            assert lighting['description']['identifier'] == 'minecraft:default_lighting'
            assert lighting['ambient']['illuminance'] > 0.02
        print(f'PASS {path.name}: archive, JSON, identities, and resources')
assert len(identities) == 10, 'Expected four resource packs and one behavior pack'
