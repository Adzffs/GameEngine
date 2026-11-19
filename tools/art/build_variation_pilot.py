"""Author/export context-placed landscape and building-condition art in LibreSprite."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile
from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout

ART = REPO / 'assets/embervale/variation-pilot'
# name: (width, height, cell width, cell height, count, columns)
SHEETS = {
    'grass': (512, 352, 32, 32, 162, 16),
    'paths': (544, 384, 32, 32, 204, 17),
    'water': (512, 96, 32, 32, 48, 16),
    'stone': (384, 32, 32, 32, 12, 12),
    'flora': (320, 128, 32, 32, 40, 10),
    'walls': (384, 288, 32, 48, 72, 12),
    'roof': (384, 192, 32, 32, 72, 12),
    'floors': (288, 32, 32, 32, 9, 9),
    'trees': (192, 96, 64, 96, 3, 3),
    'stumps': (192, 96, 64, 96, 3, 3),
    'rocks': (288, 40, 32, 40, 9, 9),
}


def build(editor, regenerate=False):
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in SHEETS):
        raise RuntimeError('Variation masters missing; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-5000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-variation-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {**{k: v[:2] for k, v in SHEETS.items()}, 'contact': (1280, 768)}.items():
            path = temp / (name + '.png')
            blank_png(path, *size)
            fixtures[name] = path.as_posix()
        cfg = {
            'regenerate': regenerate, 'fixtures': fixtures,
            'sheets': {k: list(v) for k, v in SHEETS.items()},
            'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in SHEETS},
            'old': {name: (REPO / path).as_posix() for name, path in {
                'walls': 'assets/embervale/building-pilot/textures/walls.png',
                'roof': 'assets/embervale/building-pilot/textures/roof.png',
                'floors': 'assets/embervale/building-pilot/textures/floors.png',
                'tree': 'assets/embervale/forest-pilot/textures/oak.png',
                'stump': 'assets/embervale/forest-pilot/textures/stump.png',
                'copper': 'assets/embervale/environment-pilot/textures/copper.png',
                'tin': 'assets/embervale/environment-pilot/textures/tin.png',
                'rubble': 'assets/embervale/environment-pilot/textures/rubble.png',
            }.items()},
            'preview': (ART / 'previews/variation-contact.png').as_posix(),
        }
        script = temp / 'variation.js'
        script.write_text(Path(__file__).with_name('variation_pilot.js').read_text(encoding='utf-8')
                          .replace('__CONFIG__', json.dumps(cfg)), encoding='utf-8')
        output = run('--script', script)
        if 'VARIATION_PILOT_OK:' not in output:
            raise RuntimeError('Variation authoring did not complete: ' + output[-3000:])
        print(output[output.index('VARIATION_PILOT_OK:'):].strip())
    images, assets = {}, []
    for name, (w, h, cw, ch, count, columns) in SHEETS.items():
        source = ART / f'source/{name}.ase'
        image = ART / f'textures/{name}.png'
        metadata = ART / f'textures/{name}.export.json'
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8', newline='\n')
        if png_size(image) != (w, h) or data['meta']['size'] != {'w': w, 'h': h}:
            raise RuntimeError('Export geometry mismatch: ' + name)
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png',
                        'size': [w, h], 'editor_metadata': f'textures/{name}.export.json',
                        'source_sha256': checksum(source), 'image_sha256': checksum(image)}
        for i in range(count):
            pivot = [32, 88] if name in ['trees', 'stumps'] else [16, 32] if name == 'rocks' else [16, 40] if name == 'walls' else [0, 0]
            assets.append({'id': f'variation.{name}.{i:03}', 'image': name, 'variant': i,
                           'rect': [i % columns * cw, i // columns * ch, cw, ch],
                           'pivot': pivot,
                           'intended_ground_point_in_tile': [16, 24] if name in ['trees', 'stumps', 'rocks', 'walls'] else [0, 0]})
    manifest = {
        'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'variation-pilot',
        'native_tile_size': 32, 'editor': 'LibreSprite 1.1-dev JavaScript',
        'status': 'Runtime contextual landscape dressing and authored building condition',
        'lighting': 'upper left', 'images': images, 'assets': assets,
        'layout': {
            'grass': '81 ternary NW/NE/SW/SE corner patterns (meadow/lush/dry), two surfaces each',
            'paths': '(condition * 4 + surface) * 17 + N1/E2/S4/W8 mask; cell 16 is full dirt for wide-road interiors',
            'water': 'shared deep-water corner mask * 3 + ripple variant',
            'stone': 'clean 0..2, worn 3..5, repaired 6..8, mine ground 9..11',
            'flora': 'flowers 0..7, grass 8..15, reeds 16..19, leaf beds 20..27, debris 28..31, moss 32..35, sparse leaves 36..39',
            'walls': 'condition * 24 + panel variant * 12 + original kit part',
            'roof': 'material * 36 + condition * 12 + original kit part',
            'floors': 'condition * 3 + surface variant',
            'rocks': 'copper 0..2, tin 3..5, rubble 6..8',
        },
        'conditions': ['new', 'weathered', 'repaired'], 'roof_materials': ['terracotta', 'slate'],
        'building_profiles': {'building.workshop.001': {'condition': 'repaired', 'roof_material': 'terracotta',
            'repairs': 'localized fresh plaster, replacement beam and one roof patch'}},
        'provenance': {'creator': 'Codex-assisted native pixel authoring in LibreSprite for Adam Holland',
            'notes': 'New terrain and flora drawn at native resolution. Building and resource variants derive only from the original project masters; those originals are preserved.',
            'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
        'previews': {'variation-contact.png': {'path': 'previews/variation-contact.png', 'size': [1280, 768],
                                              'sha256': checksum(ART / 'previews/variation-contact.png')}},
    }
    (ART / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='\n')
    write_renderer_layout(json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8')))
    print(json.dumps({'frames': len(assets), 'masters': len(images)}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    parser.add_argument('--regenerate', action='store_true', help='Replace variation masters; default preserves and exports existing editor work')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate)
