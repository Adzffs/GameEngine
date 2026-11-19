"""Author/export the bank chest and crafting table through LibreSprite."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout

ART = REPO / 'assets/embervale/service-pilot'
PALETTE = ['282d2c', '30211d', '463026', '603e2b', '7b4e2e', '985f35',
           'b97c43', 'd59b5a', 'ebba78', '404542', '555d59', '8e9b99',
           'b9c5bd', '84622c', 'c8a653', 'e3c073', 'dbc79a', 'efe0b7']
NAMES = ['bank', 'crafting-table']
PREVIEWS = {'service-pilot-2x.png': (640, 256), 'service-inspection-4x.png': (384, 192)}


def build(editor, regenerate=False):
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in NAMES):
        raise RuntimeError('Service masters are missing; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-5000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-service-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {'sprite': (48, 48), **PREVIEWS}.items():
            path = temp / name.replace('.png', '')
            path = path.with_suffix('.png')
            blank_png(path, *size)
            fixtures[name] = path.as_posix()
        config = {'regenerate': regenerate, 'palette': PALETTE, 'fixtures': fixtures,
                  'reference': (ART / 'reference/service-reference.png').as_posix(),
                  'forest': (REPO / 'assets/embervale/forest-pilot/textures/terrain.png').as_posix(),
                  'environment': (REPO / 'assets/embervale/environment-pilot/textures/terrain.png').as_posix(),
                  'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in NAMES},
                  'previews': {name: (ART / 'previews' / name).as_posix() for name in PREVIEWS}}
        script = temp / 'service.js'
        template = Path(__file__).with_name('service_pilot.js').read_text(encoding='utf-8')
        script.write_text(template.replace('__CONFIG__', json.dumps(config)), encoding='utf-8')
        output = run('--script', script)
        if 'SERVICE_PILOT_OK:' not in output:
            raise RuntimeError('Service authoring did not complete')
        print(output[output.index('SERVICE_PILOT_OK:'):].strip())

    images = {}
    for name in NAMES:
        source = ART / f'source/{name}.ase'
        image = ART / f'textures/{name}.png'
        metadata = ART / f'textures/{name}.export.json'
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != (48, 48) or data['meta']['size'] != {'w': 48, 'h': 48}:
            raise RuntimeError('Incorrect service export: ' + name)
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png',
                        'size': [48, 48], 'editor_metadata': f'textures/{name}.export.json',
                        'source_sha256': checksum(source), 'image_sha256': checksum(image)}
    assets = [{'id': identity, 'image': name, 'rect': [0, 0, 48, 48], 'pivot': [24, 40],
               'intended_ground_point_in_tile': [16, 24], 'intended_gameplay_footprint_tiles': [1, 1],
               'state': 'static'}
              for name, identity in [('bank', 'station.bank.idle'), ('crafting-table', 'station.crafting_table.idle')]]
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'service-pilot',
                'native_tile_size': 32, 'editor': 'LibreSprite 1.1-dev', 'lighting': 'upper left',
                'status': 'Static bank chest and crafting table runtime pilot', 'palette_rgb': PALETTE,
                'images': images, 'assets': assets,
                'provenance': {'creator': 'OpenAI image generation and Codex-assisted LibreSprite authoring for Adam Holland',
                               'reference': 'reference/service-reference.png',
                               'reference_sha256': checksum(ART / 'reference/service-reference.png'),
                               'notes': 'Original two-panel reference, no third-party image input. LibreSprite samples at native scale, quantizes the shared eighteen-colour palette, preserves leg gaps and enforces binary alpha.',
                               'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
                'previews': {name: {'path': 'previews/' + name, 'size': list(size),
                                   'sha256': checksum(ART / 'previews' / name)} for name, size in PREVIEWS.items()}}
    for name, size in PREVIEWS.items():
        if png_size(ART / 'previews' / name) != size:
            raise RuntimeError('Incorrect service preview: ' + name)
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    forest = json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8'))
    write_renderer_layout(forest)
    print(json.dumps({'assets': len(assets), 'editable_sources': len(images)}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    parser.add_argument('--regenerate', action='store_true', help='Replace the two editable masters with the reference-derived originals')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate)
