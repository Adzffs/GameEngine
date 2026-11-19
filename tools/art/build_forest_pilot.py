"""Build the authoring pilot through LibreSprite; no game runtime dependency."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib


REPO = Path(__file__).resolve().parents[2]
ART = REPO / 'assets/embervale/forest-pilot'
TREE_PALETTE = [
    '17271e', '203823', '30492a', '40592d', '52672f', '657838',
    '7c8c40', '959f49', 'b2b254', 'd0c56d', 'e1d388',
    '2f211d', '463026', '603e2b', '7b4e2e', '985f35', 'b97c43',
    'd59b5a', 'ebba78', 'f4d69b',
]


def blank_png(path, width, height):
    """An empty technical canvas. LibreSprite draws all visible artwork."""
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'
                     + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress((b'\0' + b'\0' * width * 4) * height))
                     + chunk(b'IEND', b''))


def checksum(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def png_size(path):
    data = path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise RuntimeError(f'Not a PNG: {path}')
    return struct.unpack('>II', data[16:24])


def write_renderer_layout(manifest):
    """Compile this small authoring record into a checked, dependency-free binding."""
    manifests = [manifest]
    for name in ['cast-pilot', 'environment-pilot', 'service-pilot', 'building-pilot']:
        batch_path = REPO / f'assets/embervale/{name}/manifest.json'
        if batch_path.exists():
            manifests.append(json.loads(batch_path.read_text(encoding='utf-8')))
    images = [(batch, name, image) for batch in manifests for name, image in batch['images'].items()]
    assets = [(batch, asset) for batch in manifests for asset in batch['assets']]
    walk_cycle = float(next((batch['walk']['cycle_distance_tiles'] for batch in manifests if 'walk' in batch), 1.0))
    if not 0.0 < walk_cycle <= 8.0:
        raise RuntimeError('Invalid player walk-cycle distance')
    if len({asset['id'] for _, asset in assets}) != len(assets):
        raise RuntimeError('Duplicate asset ID')
    lines = [
        '// Generated from world-art batch manifests by tools/art/build_*_pilot.py.',
        '#pragma once', '#include <SDL3/SDL.h>', '#include <array>',
        '#include <cstddef>', '#include <string_view>', '', 'namespace ForestPilotLayout', '{',
        '    struct Image { std::string_view filename; int width, height; std::string_view directory; };',
        '    struct Frame { std::string_view id; std::size_t image; SDL_FRect source;',
        '        SDL_FPoint pivot, ground; int connectionMask, grassVariant, facing, animationFrame; };',
        f'    inline constexpr int TileSize = {manifest["native_tile_size"]};',
        f'    inline constexpr float PlayerWalkCycleTiles = {walk_cycle}f;',
        f'    inline constexpr std::array<Image, {len(images)}> Images{{{{',
    ]
    for batch, name, image in images:
        filename = Path(image['image']).name
        width, height = image['size']
        directory = f'assets/embervale/{batch["id"]}/textures'
        lines.append(f'        {{{json.dumps(filename)}, {width}, {height}, {json.dumps(directory)}}},')
    lines += ['    }};', f'    inline constexpr std::array<Frame, {len(assets)}> Frames{{{{']
    def point(values):
        return '{' + ', '.join(f'{value}.0f' for value in values) + '}'
    for batch, asset in assets:
        x, y, w, h = asset['rect']
        width, height = batch['images'][asset['image']]['size']
        if min(x, y) < 0 or min(w, h) <= 0 or x + w > width or y + h > height:
            raise RuntimeError('Frame outside texture: ' + asset['id'])
        lines.append('        {' + json.dumps(asset['id']) + ', '
                     + str(next(i for i, (owner, name, _) in enumerate(images)
                                if owner is batch and name == asset['image'])) + ', ' + point(asset['rect'])
                     + ', ' + point(asset['pivot']) + ', '
                     + point(asset.get('intended_ground_point_in_tile', [0, 0])) + ', '
                     + str(asset.get('connection_mask', -1)) + ', '
                     + str(asset.get('variant', -1)) + ', '
                     + str(asset.get('facing', -1)) + ', '
                     + str(asset.get('animation_frame', 0)) + '},')
    lines += ['    }};', '}', '']
    header = REPO / 'src/Graphics/ForestPilotLayout.h'
    content = '\n'.join(lines).encode('utf-8')
    if not header.exists() or header.read_bytes() != content:
        header.write_bytes(content)


def build(editor, regenerate=False):
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file()
                              for name in ['terrain', 'oak', 'stump']):
        raise RuntimeError('Editable sources are missing. Use --regenerate to author the initial batch.')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-5000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-art-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {'terrain': (128, 160), 'resource': (64, 96),
                           'scene': (640, 384), 'large': (1280, 768)}.items():
            path = temp / f'{name}.png'
            blank_png(path, *size)
            fixtures[name] = path.as_posix()
        config = {
            'regenerate': regenerate,
            'reference': (ART / 'reference/oak-and-stump.png').as_posix(),
            'treePalette': TREE_PALETTE,
            'fixtures': fixtures,
            'sources': {name: (ART / f'source/{name}.ase').as_posix()
                        for name in ['terrain', 'oak', 'stump']},
            'previews': {'native': (ART / 'previews/forest-pilot.png').as_posix(),
                         'large': (ART / 'previews/forest-pilot-2x.png').as_posix()},
        }
        script = temp / 'forest-pilot.js'
        template = Path(__file__).with_name('forest_pilot.js').read_text(encoding='utf-8')
        script.write_text(template.replace('__CONFIG__', json.dumps(config)), encoding='utf-8')
        output = run('--script', script)
        if 'FOREST_PILOT_OK:' not in output:
            raise RuntimeError('Editor did not complete the art script')
        print(output[output.index('FOREST_PILOT_OK:'):].strip())

    images = {}
    for name, size in [('terrain', (128, 160)), ('oak', (64, 96)), ('stump', (64, 96))]:
        source = ART / f'source/{name}.ase'
        image = ART / f'textures/{name}.png'
        metadata = ART / f'textures/{name}.export.json'
        # A separate process reopens the editable source before exporting.
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != size or data['meta']['size'] != dict(zip(['w', 'h'], size)):
            raise RuntimeError(f'Export geometry mismatch: {name}')
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png',
                        'editor_metadata': f'textures/{name}.export.json',
                        'size': list(size), 'source_sha256': checksum(source),
                        'image_sha256': checksum(image)}

    assets = []
    for index in range(19):
        if index < 3:
            identity = f'terrain.grass.{index + 1:02}'
            extra = {'variant': index}
        else:
            mask = index - 3
            directions = ''.join(letter for bit, letter in [(1, 'n'), (2, 'e'), (4, 's'), (8, 'w')] if mask & bit)
            identity = 'terrain.path.' + (directions or 'isolated')
            extra = {'connection_mask': mask}
        assets.append({'id': identity, 'image': 'terrain',
                       'rect': [(index % 4) * 32, (index // 4) * 32, 32, 32],
                       'pivot': [0, 0], 'tile_size': [32, 32], **extra})
    for name, state in [('oak', 'intact'), ('stump', 'depleted')]:
        assets.append({'id': f'resource.tree.normal.{state}', 'image': name,
                       'rect': [0, 0, 64, 96], 'pivot': [32, 88],
                       'intended_ground_point_in_tile': [16, 24],
                       'intended_gameplay_footprint_tiles': [1, 1], 'state': state})
    manifest = {
        'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'forest-pilot',
        'status': 'runtime pilot: grass, dirt paths and normal tree states; other art pending', 'native_tile_size': 32,
        'editor': 'LibreSprite 1.1-dev', 'lighting': 'upper left',
        'path_mask_bits': {'north': 1, 'east': 2, 'south': 4, 'west': 8},
        'resource_palette_rgb': TREE_PALETTE, 'images': images, 'assets': assets,
        'provenance': {
            'creator': 'OpenAI image generation and Codex-assisted LibreSprite authoring for Adam Holland',
            'reference': 'reference/oak-and-stump.png',
            'reference_sha256': checksum(ART / 'reference/oak-and-stump.png'),
            'reference_notes': 'Original generated tree/stump reference; no third-party image input.',
            'modifications': 'Shared scale and ground pivot, nearest-neighbour sampling, 20-colour tree palette, binary alpha; grass and paths drawn in LibreSprite JavaScript.',
            'licence': 'No separate asset licence grant declared. Follow the repository distribution policy.',
        },
        'previews': {},
    }
    for name, size in [('forest-pilot.png', (640, 384)), ('forest-pilot-2x.png', (1280, 768))]:
        image = ART / 'previews' / name
        if png_size(image) != size:
            raise RuntimeError(f'Preview geometry mismatch: {name}')
        manifest['previews'][name] = {'path': f'previews/{name}', 'size': list(size), 'sha256': checksum(image)}
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    write_renderer_layout(manifest)
    print(json.dumps({'assets': len(assets), 'editable_sources': len(images),
                      'manifest': str(ART / 'manifest.json')}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path, help='Path to libresprite.exe with a writable settings location')
    parser.add_argument('--regenerate', action='store_true',
                        help='Replace editable masters with the reference/script-generated originals; default exports existing masters')
    args = parser.parse_args()
    editor = args.editor.resolve(strict=True)
    build(editor, regenerate=args.regenerate)
