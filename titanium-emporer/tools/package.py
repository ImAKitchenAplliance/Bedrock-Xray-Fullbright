"""Package compiled x64 Windows artifacts with source and build notes."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
binaries = Path(sys.argv[1]).resolve()
output = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else root.parent / 'dist/Titanium-Emporer-0.3-Windows.zip'
files = {name: (binaries / name).read_bytes() for name in ('Titanium-Emporer.exe', 'titanium-client.dll')}
for name, data in files.items():
    assert data[:2] == b'MZ', name
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe+4] == b'PE\0\0', name
    assert struct.unpack_from('<H', data, pe+4)[0] == 0x8664, 'Expected x64'
    if name.endswith('.exe'):
        assert struct.unpack_from('<H', data, pe+24+68)[0] == 2, 'Expected Windows GUI subsystem'
info = {
    'name': 'Titanium Emporer', 'version': '0.3.0', 'platform': 'Windows x64',
    'status': 'Injected UI preview; gameplay features not implemented',
    'compiler': 'Zig 0.15.2, C++17, x86_64-windows-gnu',
    'verified': ['Runtime activation contract tests', 'Windows x64 cross-compilation', 'PE GUI subsystem and DLL export/import inspection'],
    'not_verified': ['Windows GUI execution', 'Minecraft injection and UI overlay', 'Realm/local/server classification', 'Xray, NoFall, Fly (not implemented)', 'MSVC/CMake build'],
    'sha256': {name: hashlib.sha256(data).hexdigest() for name, data in files.items()},
}
output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    def write(name, data, mode=0o644):
        item = zipfile.ZipInfo('Titanium-Emporer/' + name, (2026, 11, 9, 0, 0, 0))
        item.compress_type = zipfile.ZIP_DEFLATED
        item.external_attr = mode << 16
        archive.writestr(item, data)
    for name, data in files.items():
        write(name, data)
    write('README.md', (root / 'README.md').read_bytes())
    write('BUILD-INFO.json', (json.dumps(info, indent=2) + '\n').encode())
    for path in sorted(root.rglob('*')):
        relative = path.relative_to(root)
        if path.is_file() and not any(p.startswith('build') or p in ('out', '__pycache__') for p in relative.parts[:-1]):
            write('source/' + relative.as_posix(), path.read_bytes(), 0o755 if path.suffix == '.sh' else 0o644)
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    assert 'Titanium-Emporer/source/scripts/build-cross.sh' in archive.namelist()
    assert 'Titanium-Emporer/source/scripts/build-windows.ps1' in archive.namelist()
    for name, digest in info['sha256'].items():
        assert hashlib.sha256(archive.read('Titanium-Emporer/' + name)).hexdigest() == digest
print(f'PASS {output.name}: Windows x64 GUI, source, hashes and ZIP integrity ({output.stat().st_size} bytes)')
