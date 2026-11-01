"""Validate deliverable structure, resource resolution, alpha and separate identities."""
from pathlib import Path
import json
import uuid
import zipfile

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
            # The failed release's atlas and zero-alpha texture must not survive packaging.
            assert not any(n.startswith('textures/') for n in names), 'Obsolete alpha override remains'
            assert all(entry == {'blockshape': 'invisible'} for entry in blocks.values())
            for block in ['stone', 'deepslate', 'grass', 'dirt', 'netherrack', 'end_stone']:
                assert block in blocks, block
            assert manifest['header']['version'] == [1, 1, 0]
            assert manifest['modules'][0]['version'] == [1, 1, 0]
            assert 'capabilities' not in manifest and 'dependencies' not in manifest
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
