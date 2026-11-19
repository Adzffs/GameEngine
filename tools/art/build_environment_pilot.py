"""Author/export River Crossing water, stone, copper/tin and depleted rubble."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout

ART = REPO / 'assets/embervale/environment-pilot'
PALETTE = ['354038', '4b554d', '677165', '858d7a', 'adb299',
           '76452f', 'ae693e', 'd49458', 'e2ba7c', '8e9b99', 'b9c5bd', 'dde1c9']
BANKS = ['n', 'e', 's', 'w', 'outer-nw', 'outer-ne', 'outer-sw', 'outer-se',
         'inner-nw', 'inner-ne', 'inner-sw', 'inner-se']
SIZES = {'terrain': (256, 192), 'copper': (32, 40), 'tin': (32, 40), 'rubble': (32, 40)}


def build(editor, regenerate=False):
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in SIZES):
        raise RuntimeError('Environment masters are missing; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-5000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-environment-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {**SIZES, 'board': (768, 384)}.items():
            path = temp / f'{name}.png'
            blank_png(path, *size)
            fixtures[name] = path.as_posix()
        config = {'regenerate': regenerate, 'palette': PALETTE, 'fixtures': fixtures,
                  'reference': (ART / 'reference/mining-reference.png').as_posix(),
                  'forest': (REPO / 'assets/embervale/forest-pilot/textures/terrain.png').as_posix(),
                  'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in SIZES},
                  'preview': (ART / 'previews/environment-pilot-2x.png').as_posix()}
        script = temp / 'environment.js'
        template = Path(__file__).with_name('environment_pilot.js').read_text(encoding='utf-8')
        script.write_text(template.replace('__CONFIG__', json.dumps(config)), encoding='utf-8')
        output = run('--script', script)
        if 'ENVIRONMENT_PILOT_OK:' not in output:
            raise RuntimeError('Environment authoring did not complete')
        print(output[output.index('ENVIRONMENT_PILOT_OK:'):].strip())

    images = {}
    for name, size in SIZES.items():
        source = ART / f'source/{name}.ase'
        image = ART / f'textures/{name}.png'
        metadata = ART / f'textures/{name}.export.json'
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != size or data['meta']['size'] != dict(zip(['w', 'h'], size)):
            raise RuntimeError('Incorrect environment export: ' + name)
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png',
                        'size': list(size), 'editor_metadata': f'textures/{name}.export.json',
                        'source_sha256': checksum(source), 'image_sha256': checksum(image)}
    terrain_ids = [f'terrain.water.{i + 1:02}' for i in range(3)]
    terrain_ids += [f'terrain.stone.{i + 1:02}' for i in range(3)]
    terrain_ids += [f'terrain.bank.{family}.{edge}' for family in ['earth', 'stone', 'dirt'] for edge in BANKS]
    assets = [{'id': identity, 'image': 'terrain', 'rect': [(i % 8) * 32, (i // 8) * 32, 32, 32],
               'pivot': [0, 0]} for i, identity in enumerate(terrain_ids)]
    for name, identity in [('copper', 'resource.rock.copper.intact'),
                           ('tin', 'resource.rock.tin.intact'), ('rubble', 'resource.rock.depleted')]:
        assets.append({'id': identity, 'image': name, 'rect': [0, 0, 32, 40], 'pivot': [16, 32],
                       'intended_ground_point_in_tile': [16, 24],
                       'intended_gameplay_footprint_tiles': [1, 1]})
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'environment-pilot',
                'native_tile_size': 32, 'editor': 'LibreSprite 1.1-dev',
                'status': 'Static river water/banks, stone floors, copper/tin and depleted rubble runtime pilot',
                'lighting': 'upper left', 'resource_palette_rgb': PALETTE,
                'bank_order': BANKS, 'images': images, 'assets': assets,
                'provenance': {'creator': 'OpenAI image generation and Codex-assisted LibreSprite authoring for Adam Holland',
                               'reference': 'reference/mining-reference.png',
                               'reference_sha256': checksum(ART / 'reference/mining-reference.png'),
                               'notes': 'Original three-panel rock reference, no third-party input. LibreSprite samples at shared scale, quantizes a twelve-colour palette and enforces binary alpha. Water, pavers and bank overlays are drawn in LibreSprite JavaScript.',
                               'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
                'previews': {'environment-pilot-2x.png': {'path': 'previews/environment-pilot-2x.png',
                              'sha256': checksum(ART / 'previews/environment-pilot-2x.png')}}}
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    forest = json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8'))
    write_renderer_layout(forest)
    print(json.dumps({'assets': len(assets), 'sources': len(images)}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    parser.add_argument('--regenerate', action='store_true', help='Replace editable masters with the authored originals')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate)
