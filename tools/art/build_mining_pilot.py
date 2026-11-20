"""Export editable eight-way mining poses and three pickaxe tiers through LibreSprite."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile
from animation_common import load_timing, write_timing_header, retime_gif

from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout
from build_cast_pilot import PALETTE

ART = REPO / 'assets/embervale/mining-pilot'
DIRECTIONS = ['s', 'w', 'e', 'n', 'sw', 'se', 'nw', 'ne']


def build(editor, regenerate=False, rebuild_pickaxes=False):
    timing = load_timing()
    write_timing_header()
    sizes = {'player': (384, 640), 'pickaxes': (384, 1920)}
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in sizes):
        raise RuntimeError('Missing mining masters; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-6000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-mining-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {**sizes, 'board': (768, 480), 'contact': (768, 1280), 'tiers': (768, 320)}.items():
            target = temp / f'{name}.png'
            blank_png(target, *size)
            fixtures[name] = target.as_posix()
        cfg = {'regenerate': regenerate, 'rebuildPickaxes': rebuild_pickaxes, 'palette': PALETTE, 'fixtures': fixtures,
               'player': (REPO / 'assets/embervale/cast-pilot/source/player.ase').as_posix(),
               'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in sizes},
               'frames': [(temp / f'mine-{i:02}.png').as_posix() for i in range(6)],
               'contact': (ART / 'previews/mine-contact.png').as_posix(),
               'tiers': (ART / 'previews/pickaxe-tiers.png').as_posix()}
        helpers = Path(__file__).with_name('cast_pilot.js').read_text(encoding='utf-8').split('var player, mara, furnace;')[0]
        action_helpers = Path(__file__).with_name('mining_geometry.js').read_text(encoding='utf-8').split('function axePose(')[0]
        script = temp / 'mining.js'
        script.write_text((helpers + '\n' + action_helpers + '\n' + Path(__file__).with_name('mining_pilot.js').read_text(encoding='utf-8'))
                          .replace('__CONFIG__', json.dumps(cfg)), encoding='utf-8')
        output = run('--script', script)
        if 'MINING_PILOT_OK:' not in output:
            raise RuntimeError('Mining authoring did not complete')
        report = json.loads(next(line.split('=', 1)[1] for line in output.splitlines()
                                 if line.startswith('MINING_VALIDATION_JSON=')))
        (ART / 'previews/validation.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
        run(cfg['frames'][0], '--save-as', ART / 'previews/mine-cycle.gif',
            '--data', temp / 'preview.json', '--sheet', temp / 'preview.png')
        retime_gif(ART / 'previews/mine-cycle.gif', timing['mining_ms'])
        if len(json.loads((temp / 'preview.json').read_text(encoding='utf-8'))['frames']) != 6:
            raise RuntimeError('Preview must contain all six poses')
    images = {}
    for name, size in sizes.items():
        source, image, metadata = (ART / f'source/{name}.ase', ART / f'textures/{name}.png',
                                   ART / f'textures/{name}.export.json')
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != size:
            raise RuntimeError('Wrong mining texture geometry')
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png', 'size': list(size),
                        'editor_metadata': f'textures/{name}.export.json',
                        'source_sha256': checksum(source), 'image_sha256': checksum(image)}
    assets = []
    for facing, direction in enumerate(DIRECTIONS):
        for pose in range(6):
            geometry = {'pivot': [32, 52], 'intended_ground_point_in_tile': [16, 24],
                        'facing': facing, 'animation_frame': pose}
            assets.append({'id': f'character.mining.{direction}.{pose}', 'image': 'player',
                           'rect': [pose * 64, facing * 80, 64, 80], **geometry})
            for tier, name in enumerate(['bronze', 'iron', 'steel']):
                assets.append({'id': f'tool.pickaxe.{name}.mining.{direction}.{pose}', 'image': 'pickaxes',
                               'rect': [pose * 64, (tier * 8 + facing) * 80, 64, 80], 'variant': tier, **geometry})
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'mining-pilot',
                'native_tile_size': 32, 'editor': 'LibreSprite 1.1-dev', 'palette_rgb': PALETTE,
                'status': 'Eight-way mining action poses and three pickaxe tiers',
                'facing_order': DIRECTIONS, 'cycle_seconds': 1.2, 'poses_per_direction': 6, 'frame_milliseconds': timing['mining_ms'],
                'images': images, 'assets': assets,
                'provenance': {'creator': 'Codex-assisted LibreSprite authoring for Adam Holland',
                               'source': '../cast-pilot/source/player.ase',
                               'source_sha256': checksum(REPO / 'assets/embervale/cast-pilot/source/player.ase'),
                               'notes': 'Original player outfit retained; action poses, arms and pickaxes authored in LibreSprite JavaScript.',
                               'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
                'previews': {name: {'path': f'previews/{name}', 'sha256': checksum(ART / f'previews/{name}')}
                             for name in ['mine-cycle.gif', 'mine-contact.png', 'pickaxe-tiers.png']}}
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    write_renderer_layout(json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8')))
    print(json.dumps({'body_frames': 48, 'pickaxe_frames': 144, 'preview_frames': 6, 'failed_frames': report['failedFrames']}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--regenerate', action='store_true', help='Explicitly recreate action masters from the current player outfit')
    mode.add_argument('--rebuild-pickaxes', action='store_true', help='Rebuild only pickaxe overlays; preserve the saved player action master')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate, args.rebuild_pickaxes)
