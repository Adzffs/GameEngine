"""Reject an invalid private source before copy; repair private packaged outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--cmake', type=Path, required=True)
parser.add_argument('--make-program', type=Path, required=True)
parser.add_argument('--sdl-dir', type=Path, required=True)
parser.add_argument('--python', type=Path, required=True)
args = parser.parse_args()
repo = args.source.resolve()

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def run(*command, success=True):
    result = subprocess.run(list(map(str, command)), capture_output=True, text=True, timeout=60)
    if success and result.returncode:
        raise AssertionError('Command failed: ' + repr(command) + '\n' + result.stdout + result.stderr)
    if not success and not result.returncode:
        raise AssertionError('Invalid source unexpectedly reached the copy target')
    return result

with tempfile.TemporaryDirectory(prefix='embervale art package ') as directory:
    root = Path(directory)
    source = root / 'private source'
    shutil.copytree(repo / 'src', source / 'src')
    shutil.copy2(repo / 'CMakeLists.txt', source / 'CMakeLists.txt')
    for name in ['check_world_art.py', 'world_art_resources.py']:
        target = source / 'tools/art' / name; target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(repo / 'tools/art' / name, target)
    files = [Path('assets/embervale/world-art-limits.json')]
    textures = []
    for manifest in sorted((repo / 'assets/embervale').glob('*/manifest.json')):
        data = json.loads(manifest.read_text())
        if data.get('schema') == 'embervale.authoring-batch':
            files.append(manifest.relative_to(repo))
            for image in data['images'].values():
                texture = manifest.parent.relative_to(repo) / image['image']
                textures.append(texture)
                files.extend([texture, manifest.parent.relative_to(repo) / image['source']])
    audio = sorted(p.relative_to(repo) for p in (repo / 'assets/embervale/audio-pilot').glob('*.wav'))
    files.extend(audio)
    for relative in files:
        target = source / relative; target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(repo / relative, target)
    expected = {p: digest(repo / p) for p in textures + audio}
    original = {p: digest(repo / p) for p in files}

    def configure(build, generator):
        run(args.cmake, '-S', source, '-B', build, '-G', generator,
            '-DBUILD_TESTING=OFF', '-DGAMEENGINE_BUILD_GRAPHICS_TESTS=OFF', '-DGAMEENGINE_BUILD_CLIENT=ON',
            '-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_MAKE_PROGRAM=' + str(args.make_program),
            '-DSDL3_DIR=' + str(args.sdl_dir), '-DPython3_EXECUTABLE=' + str(args.python))
    def build_art(build, success=True):
        return run(args.cmake, '--build', build, '--target', 'GameEngineWorldArt', '--config', 'Release', '-j4', success=success)
    def hashes(folder):return {p: digest(folder / p) for p in expected}

    build = root / 'single config output'
    configure(build, 'Ninja')
    build_art(build)
    assert hashes(build) == expected, 'Initial copied bytes do not match source'
    victim = source / textures[0]
    valid = victim.read_bytes(); victim.write_bytes(b'broken private PNG')
    result = build_art(build, success=False)
    assert 'World art rejected:' in result.stdout + result.stderr, 'Failure did not come from asset preflight'
    assert hashes(build) == expected, 'Rejected source modified the previous packaged output'
    victim.write_bytes(valid)
    (build / textures[0]).write_bytes(b'private output corruption')
    (build / textures[-1]).unlink()
    build_art(build)
    assert hashes(build) == expected, 'No-relink copy did not repair corrupt/deleted outputs'
    assert not list(build.rglob('*.o')), 'Art-only copy unexpectedly compiled C++'
    multi = root / 'multi config output'
    configure(multi, 'Ninja Multi-Config')
    build_art(multi)
    assert hashes(multi / 'Release') == expected, 'Multi-config Release copied to the wrong path/bytes'
    assert original == {p: digest(repo / p) for p in files}, 'Owned private test changed real source assets'

print(f'PASS: {len(textures)} textures/{len(audio)} sounds, invalid-source refusal, preserved output, '
      'no-relink repair, spaced paths and multi-config Release')
