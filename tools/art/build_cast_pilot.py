"""Author/export the player, Mara and furnace using LibreSprite 1.1-dev."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout

ART = REPO / 'assets/embervale/cast-pilot'
PALETTE = ['222b2b', '30211d', '563727', '7b4e2e', 'b27c48',
           '935b3d', 'be8054', 'd99d69', 'edbd89', 'a69a7e', 'dbc79a', 'efe0b7',
           '1f3345', '294b62', '37617d', '5c8395', '8da39c', '20293e', '293b55', '415778',
           '84622c', 'c8a653', 'e3c073', '282d2c', '404542', '555d59', '748057', 'aaad8b',
           '9e3f21', 'd76f27', 'e6943b', 'f5c05b', 'ffe08a']


def build(editor, regenerate=False, rebuild_walk=False):
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    sizes = {'player': (128, 192), 'mara': (32, 48), 'furnace': (48, 64)}
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in sizes):
        raise RuntimeError('Cast masters are missing; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-5000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-cast-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {**sizes, 'board': (640, 224), 'inspection': (640, 256), 'contacts': (640, 896)}.items():
            path = temp / f'{name}.png'
            blank_png(path, *size)
            fixtures[name] = path.as_posix()
        walk_frames = [(temp / f'walk-{i:02}.png').as_posix() for i in range(4)]
        inspection_frames = [(temp / f'inspection-{i:02}.png').as_posix() for i in range(4)]
        cfg = {'regenerate': regenerate, 'rebuildWalk': rebuild_walk, 'palette': PALETTE, 'fixtures': fixtures,
               'reference': (ART / 'reference/cast-reference.png').as_posix(),
               'terrain': (REPO / 'assets/embervale/forest-pilot/textures/terrain.png').as_posix(),
               'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in sizes},
               'previews': {'board': (ART / 'previews/cast-pilot-2x.png').as_posix(),
                            'contacts': (ART / 'previews/walk-contact-sheet.png').as_posix()},
               'walkFrames': walk_frames, 'inspectionFrames': inspection_frames}
        script = temp / 'cast-pilot.js'
        template = Path(__file__).with_name('cast_pilot.js').read_text(encoding='utf-8')
        walk_module = Path(__file__).with_name('player_walk.js').read_text(encoding='utf-8')
        script.write_bytes((walk_module + '\n' + template.replace('__CONFIG__', json.dumps(cfg))).encode('utf-8'))
        output = run('--script', script)
        if 'CAST_PILOT_OK:' not in output:
            raise RuntimeError('Cast authoring did not complete')
        print(output[output.index('CAST_PILOT_OK:'):].strip())
        prefix = 'WALK_VALIDATION_JSON='
        report = json.loads(next(line[len(prefix):] for line in output.splitlines() if line.startswith(prefix)))
        (ART / 'previews/walk-validation.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
        # LibreSprite loads numbered PNG files as a sequence in batch mode.
        for name, sequence in [('walk-cycle', walk_frames), ('walk-inspection', inspection_frames)]:
            run(sequence[0], '--save-as', ART / f'previews/{name}.gif',
                '--data', temp / f'{name}.json', '--sheet', temp / f'{name}.png')
            frames = json.loads((temp / f'{name}.json').read_text(encoding='utf-8'))['frames']
            if len(frames) != 4:
                raise RuntimeError('Walk preview did not load all four PNG sequence frames')

    images = {}
    for name, size in sizes.items():
        source = ART / f'source/{name}.ase'
        image = ART / f'textures/{name}.png'
        metadata = ART / f'textures/{name}.export.json'
        run(source, '--sheet', image, '--data', metadata)
        data = json.loads(metadata.read_text(encoding='utf-8'))
        data['meta']['image'] = image.name
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != size:
            raise RuntimeError('Incorrect cast export: ' + name)
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png',
                        'size': list(size), 'editor_metadata': f'textures/{name}.export.json',
                        'source_sha256': checksum(source), 'image_sha256': checksum(image)}
    assets = []
    for direction, name in enumerate(['s', 'w', 'e', 'n']):
        for phase in range(4):
            assets.append({'id': f'character.player.{name}.' + ('idle' if phase == 0 else f'walk{phase}'),
                           'image': 'player', 'rect': [phase * 32, direction * 48, 32, 48],
                           'pivot': [16, 40], 'intended_ground_point_in_tile': [16, 24],
                           'facing': direction, 'animation_frame': phase})
    assets += [
        {'id': 'character.mara.idle', 'image': 'mara', 'rect': [0, 0, 32, 48],
         'pivot': [16, 40], 'intended_ground_point_in_tile': [16, 24]},
        {'id': 'station.furnace.idle', 'image': 'furnace', 'rect': [0, 0, 48, 64],
         'pivot': [24, 56], 'intended_ground_point_in_tile': [16, 24]},
    ]
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'cast-pilot',
                'native_tile_size': 32, 'status': 'Player/Mara/furnace runtime pilot',
                'editor': 'LibreSprite 1.1-dev', 'palette_rgb': PALETTE,
                'facing_order': ['south', 'west', 'east', 'north'],
                'walk': {'frames_per_direction': 4, 'cycle_distance_tiles': 1.0,
                         'idle_frame': 0, 'validation': 'previews/walk-validation.json',
                         'notes': 'Two neutral and two alternating stride poses with a continuous pelvis; driven by visual travel distance.'},
                'images': images, 'assets': assets,
                'provenance': {'creator': 'OpenAI image generation and Codex-assisted LibreSprite authoring for Adam Holland',
                               'reference': 'reference/cast-reference.png',
                               'reference_sha256': checksum(ART / 'reference/cast-reference.png'),
                               'notes': 'Original six-panel reference; no third-party image input. LibreSprite performs native-size sampling, palette cleanup, binary-alpha cleanup and stride authoring.',
                               'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
                'previews': {name: {'path': 'previews/' + name, 'sha256': checksum(ART / 'previews' / name)}
                             for name in ['cast-pilot-2x.png', 'walk-cycle.gif', 'walk-inspection.gif', 'walk-contact-sheet.png']}}
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    forest = json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8'))
    write_renderer_layout(forest)
    print(json.dumps({'assets': len(assets), 'sources': len(images), 'gif_frames': 4}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--regenerate', action='store_true', help='Replace editable masters with the authored originals')
    mode.add_argument('--rebuild-walk', action='store_true',
                      help='Repair only stride cells from the saved neutral frames; preserve other artwork')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate, args.rebuild_walk)
