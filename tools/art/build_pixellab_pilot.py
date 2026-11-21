"""Import complete PixelLab walks once; normal export preserves the editable master."""
import argparse
import json
import re
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

from animation_common import retime_gif
from build_forest_pilot import blank_png, checksum, png_size, write_renderer_layout
from world_art_resources import load_limits, load_manifests

REPO = Path(__file__).resolve().parents[2]
ART = REPO / 'assets/embervale/pixellab-pilot'
DIRECTIONS = ['south', 'west', 'east', 'north', 'south-west', 'south-east', 'north-west', 'north-east']
DURATIONS = [80, 70, 80, 70, 80, 70, 80, 70]


def build(editor, import_zip=None):
    for name in ['source', 'textures', 'previews']:
        (ART / name).mkdir(parents=True, exist_ok=True)
    source = ART / 'source/player.ase'
    if import_zip is None and not source.is_file():
        raise ValueError('No PixelLab master exists; use --import-zip with all eight completed walks')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, text=True, timeout=180)
        output = result.stdout + result.stderr
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-5000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-pixellab-') as scratch:
        temp = Path(scratch)
        inputs = {}
        if import_zip is not None:
            if import_zip.stat().st_size > 64 * 1024 * 1024:
                raise ValueError('Character archive exceeds the import limit')
            with zipfile.ZipFile(import_zip) as archive:
                entries = archive.infolist()
                if len(entries) > 1024 or len({entry.filename for entry in entries}) != len(entries):
                    raise ValueError('Character archive contains too many entries')
                if not 0 < archive.getinfo('metadata.json').file_size <= 2 * 1024 * 1024:
                    raise ValueError('Character metadata exceeds the import limit')
                metadata = json.loads(archive.read('metadata.json'))
                states = metadata.get('states', [])
                if len(states) != 1:
                    raise ValueError('Import one selected character state at a time')
                state = states[0]
                walking = state['frames']['animations']['Walking']
                rotations = state['frames']['rotations']
                if set(walking) != set(DIRECTIONS) or any(len(walking[d]) != 8 for d in DIRECTIONS):
                    raise ValueError('All eight directions must have exactly eight completed walking frames')
                from world_art_resources import validate_png
                for facing, direction in enumerate(DIRECTIONS):
                    names = [rotations[direction], *walking[direction]]
                    inputs[direction] = []
                    for pose, name in enumerate(names):
                        entry = archive.getinfo(name)
                        if not 0 < entry.file_size <= 1024 * 1024:
                            raise ValueError('Unexpected frame size in character archive')
                        data = archive.read(entry)
                        validate_png(data, [64, 64], load_limits())
                        # LibreSprite auto-opens numbered siblings as a sequence.
                        # Isolate each input so idle plus eight poses remain cells.
                        path = temp / 'inputs' / direction / str(pose) / 'image.png'
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_bytes(data)
                        inputs[direction].append(path.as_posix())
            # Retain only fully checked inputs; archive paths never become local paths.
            retained = ART / 'generated/walking'
            retained.mkdir(parents=True, exist_ok=True)
            for direction in DIRECTIONS:
                destination = retained / direction
                destination.mkdir(exist_ok=True)
                for pose, path in enumerate(inputs[direction]):
                    shutil.copyfile(path, destination / ('idle.png' if pose == 0 else f'frame-{pose-1:02}.png'))
            (retained / 'metadata.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
            retained_archive = ART / 'generated/walking-all-directions.zip'
            if import_zip.resolve() != retained_archive.resolve():
                shutil.copyfile(import_zip, retained_archive)

        fixtures = {}
        for name, size in [('master', (576, 512)), ('contacts', (1152, 1024)), ('loop', (768, 384))]:
            path = temp / f'{name}.png'
            blank_png(path, *size)
            fixtures[name] = path.as_posix()
        cfg = {'importing': import_zip is not None, 'inputs': inputs, 'directions': DIRECTIONS,
               'source': source.as_posix(), 'fixtures': fixtures,
               'contacts': (ART / 'previews/walk-contact-sheet.png').as_posix(),
               'loopFrames': [(temp / f'loop-{pose:02}.png').as_posix() for pose in range(8)]}
        script = temp / 'pixellab.js'
        script.write_text(Path(__file__).with_name('pixellab_pilot.js').read_text(encoding='utf-8')
                          .replace('__CONFIG__', json.dumps(cfg)), encoding='utf-8')
        output = run('--script', script)
        if 'PIXELLAB_WALK_OK:' not in output:
            raise RuntimeError('PixelLab walk import/export did not complete')
        report = json.loads(output.split('PIXELLAB_WALK_JSON=')[1].splitlines()[0])
        if import_zip is not None:
            (ART / 'generated/walking/import-validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        (ART / 'previews/validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        run(cfg['loopFrames'][0], '--save-as', ART / 'previews/walk-cycle.gif',
            '--sheet', temp / 'loop-sheet.png', '--data', temp / 'loop.json')
        if len(json.loads((temp / 'loop.json').read_text())['frames']) != 8:
            raise ValueError('Walk preview must have eight moving frames')
        retime_gif(ART / 'previews/walk-cycle.gif', DURATIONS)

    image = ART / 'textures/player.png'
    metadata_path = ART / 'textures/player.export.json'
    run(source, '--sheet', image, '--data', metadata_path)
    data = json.loads(metadata_path.read_text())
    data['meta']['image'] = image.name
    metadata_path.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
    if png_size(image) != (576, 512):
        raise ValueError('Player master/export geometry differs')
    assets = [{'id': f'character.locomotion.{direction}.{pose:02}', 'image': 'player',
               'rect': [pose * 64, facing * 64, 64, 64], 'pivot': [32, 48],
               'intended_ground_point_in_tile': [16, 24], 'facing': facing, 'animation_frame': pose}
              for facing, direction in enumerate(DIRECTIONS) for pose in range(9)]
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'pixellab-pilot',
        'native_tile_size': 32, 'status': 'eight-way empty-handed walking; equipped/actions retain their own batches',
        'editor': 'LibreSprite 1.1-dev',
        'images': {'player': {'source': 'source/player.ase', 'image': 'textures/player.png',
            'editor_metadata': 'textures/player.export.json', 'size': [576, 512],
            'source_sha256': checksum(source), 'image_sha256': checksum(image)}}, 'assets': assets,
        'locomotion': {'idle_frame': 0, 'walk_frames': list(range(1, 9)),
            'cycle_distance_tiles': 1.0, 'frame_durations_ms': DURATIONS, 'directions': DIRECTIONS},
        'provenance': {'service': 'PixelLab', 'model': 'PixMiniMax',
            'character_id': '73253a5c-9983-4f58-8d67-cd31a1c928d5',
            'input_archive': 'generated/walking-all-directions.zip',
            'input_archive_sha256': checksum(ART / 'generated/walking-all-directions.zip'),
            'modifications': 'Lossless atlas assembly, separate supplied idle, integer preview scaling; no pose redraw or deformation.'},
        'previews': {name: {'path': 'previews/' + name, 'sha256': checksum(ART / 'previews' / name)}
            for name in ['walk-contact-sheet.png', 'walk-cycle.gif', 'validation.json']}}
    (ART / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    # The usual complete read-only gate owns generated runtime bindings.
    manifests = load_manifests()
    write_renderer_layout(manifests[0])
    print(json.dumps({'directions': 8, 'idle_and_walk_cells': 72, 'editable_master': str(source)}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    parser.add_argument('--import-zip', type=Path, help='Explicitly replace the master using a complete character export')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.import_zip)
