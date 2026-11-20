"""Export editable eight-way chopping poses and three axe tiers through LibreSprite."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout
from build_cast_pilot import PALETTE
from animation_common import load_timing, write_timing_header, retime_gif

ART = REPO / 'assets/embervale/woodcutting-pilot'
DIRECTIONS = ['s', 'w', 'e', 'n', 'sw', 'se', 'nw', 'ne']


def build(editor, regenerate=False, rebuild_axes=False):
    sizes = {'player': (768, 768), 'axes': (768, 2304)}
    timing = load_timing()
    write_timing_header()
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in sizes):
        raise RuntimeError('Missing woodcutting masters; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-6000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-woodcutting-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {**sizes, 'board': (1152, 576), 'contact': (1536, 1536), 'tiers': (864, 288)}.items():
            target = temp / f'{name}.png'
            blank_png(target, *size)
            fixtures[name] = target.as_posix()
        cfg = {'regenerate': regenerate, 'rebuildAxes': rebuild_axes, 'palette': PALETTE, 'fixtures': fixtures,
               'player': (REPO / 'assets/embervale/cast-pilot/source/player.ase').as_posix(),
               'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in sizes},
               'frames': [(temp / f'chop-{i:02}.png').as_posix() for i in range(8)],
               'contact': (ART / 'previews/chop-contact.png').as_posix(),
               'tiers': (ART / 'previews/axe-tiers.png').as_posix()}
        helpers = Path(__file__).with_name('cast_pilot.js').read_text(encoding='utf-8').split('var player, mara, furnace;')[0]
        script = temp / 'woodcutting.js'
        rig = Path(__file__).with_name('player_rig.js').read_text(encoding='utf-8')
        script.write_text((helpers + '\n' + rig + '\n' + Path(__file__).with_name('woodcutting_pilot.js').read_text(encoding='utf-8'))
                          .replace('__CONFIG__', json.dumps(cfg)), encoding='utf-8')
        output = run('--script', script)
        if 'WOODCUTTING_PILOT_OK:' not in output:
            raise RuntimeError('Woodcutting authoring did not complete')
        report = json.loads(next(line.split('=', 1)[1] for line in output.splitlines()
                                 if line.startswith('WOODCUTTING_VALIDATION_JSON=')))
        (ART / 'previews/validation.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
        run(cfg['frames'][0], '--save-as', ART / 'previews/chop-cycle.gif',
            '--data', temp / 'preview.json', '--sheet', temp / 'preview.png')
        if len(json.loads((temp / 'preview.json').read_text(encoding='utf-8'))['frames']) != 8:
            raise RuntimeError('Preview must contain all eight poses')
        retime_gif(ART / 'previews/chop-cycle.gif', timing['chopping_ms'])
    images = {}
    for name, size in sizes.items():
        source, image, metadata = (ART / f'source/{name}.ase', ART / f'textures/{name}.png',
                                   ART / f'textures/{name}.export.json')
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != size:
            raise RuntimeError('Wrong woodcutting texture geometry')
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png', 'size': list(size),
                        'editor_metadata': f'textures/{name}.export.json',
                        'source_sha256': checksum(source), 'image_sha256': checksum(image)}
    assets = []
    for facing, direction in enumerate(DIRECTIONS):
        for pose in range(8):
            geometry = {'pivot': [48, 64], 'intended_ground_point_in_tile': [16, 24],
                        'facing': facing, 'animation_frame': pose}
            assets.append({'id': f'character.woodcutting.{direction}.{pose}', 'image': 'player',
                           'rect': [pose * 96, facing * 96, 96, 96], **geometry})
            for tier, name in enumerate(['bronze', 'iron', 'steel']):
                assets.append({'id': f'tool.axe.{name}.woodcutting.{direction}.{pose}', 'image': 'axes',
                               'rect': [pose * 96, (tier * 8 + facing) * 96, 96, 96], 'variant': tier, **geometry})
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'woodcutting-pilot',
                'native_tile_size': 32, 'editor': 'LibreSprite 1.1-dev', 'palette_rgb': PALETTE,
                'status': 'Eight-way woodcutting action poses and three axe tiers',
                'facing_order': DIRECTIONS, 'cycle_seconds': 1.2, 'poses_per_direction': 8,
                'frame_durations_ms': timing['chopping_ms'], 'timing_source': '../player-animation.json',
                'images': images, 'assets': assets,
                'provenance': {'creator': 'Codex-assisted LibreSprite authoring for Adam Holland',
                               'source': '../cast-pilot/source/player.ase',
                               'source_sha256': checksum(REPO / 'assets/embervale/cast-pilot/source/player.ase'),
                               'notes': 'Original player outfit retained; action poses, arms and axes authored in LibreSprite JavaScript.',
                               'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
                'previews': {name: {'path': f'previews/{name}', 'sha256': checksum(ART / f'previews/{name}')}
                             for name in ['chop-cycle.gif', 'chop-contact.png', 'axe-tiers.png']}}
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    write_renderer_layout(json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8')))
    print(json.dumps({'body_frames': 64, 'axe_frames': 192, 'preview_frames': 8, 'failed_frames': report['failedFrames']}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--regenerate', action='store_true', help='Explicitly recreate action masters from the current player outfit')
    mode.add_argument('--rebuild-axes', action='store_true', help='Rebuild only axe overlays; preserve the saved player action master')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate, args.rebuild_axes)
