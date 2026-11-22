"""Assemble one editable axe shape and hand track with multiple material palettes."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from animation_common import retime_gif
from build_forest_pilot import REPO, blank_png, checksum
from modular_equipment import load_recipe, require
from world_art_resources import json_file, read_regular

PILOT = REPO / 'assets/embervale/equipment-modular-pilot'
SOURCES = ('source/body.ase', 'source/axe.ase', 'source/metal-mask.ase')
TIMING = REPO / 'assets/embervale/player-animation.json'
COMPILER_FILES = ('build_modular_axe_pilot.py', 'modular_equipment.py', 'modular_axe_pilot.js',
                  'cast_pilot.js', 'animation_common.py', 'build_forest_pilot.py', 'world_art_resources.py')


def text_checksum(path, limit=1024 * 1024):
    # Git can check text out as CRLF on Windows. Only text line endings are
    # canonicalized; masters, PNGs and GIFs retain exact binary hashes.
    return hashlib.sha256(read_regular(path, limit).replace(b'\r\n', b'\n')).hexdigest()


def output_checksum(path):
    return text_checksum(path, 65536) if path.suffix == '.json' else hashlib.sha256(read_regular(path, 16 * 1024 * 1024)).hexdigest()


def compiler_inputs():
    return {'compiler/' + name: text_checksum(Path(__file__).with_name(name))
            for name in COMPILER_FILES}


def fingerprint(output):
    return {**{name: hashlib.sha256(read_regular(output / name, 4 * 1024 * 1024)).hexdigest() for name in SOURCES},
            'recipe.json': text_checksum(output / 'recipe.json', 65536),
            'shared_timing': text_checksum(TIMING), **compiler_inputs()}


def check(output):
    load_recipe(output / 'recipe.json')
    report = json_file(output / 'manifest.json', 65536)
    require(report.get('schema') == 'embervale.modular-equipment-proof' and
            type(report.get('version')) is int and report['version'] == 1, 'Unsupported proof receipt')
    require(report['inputs'] == fingerprint(output), 'An editable input changed; export is required')
    expected_outputs = {'textures/' + name + '.png' for name in ('body', 'axe', 'metal-mask', 'tools', 'composites')}
    expected_outputs.update({'previews/contact.png', 'previews/grips.png', 'previews/occlusion.png',
                             'previews/materials.gif', 'previews/validation.json'})
    require(set(report['outputs']) == expected_outputs, 'Proof receipt has missing or unknown outputs')
    for name, expected in report['outputs'].items():
        require(output_checksum(output / name) == expected,
                'Generated output changed or is missing: ' + name)
    return report


def build(editor, output=PILOT, import_legacy=False):
    output = output.resolve()
    recipe = load_recipe(output / 'recipe.json')
    for name in SOURCES:
        if import_legacy:
            require(not (output / name).exists(), 'Bootstrap refuses to overwrite an existing editable master')
        else:
            read_regular(output / name, 4 * 1024 * 1024)
    legacy = REPO / 'assets/embervale/woodcutting-pilot/source/player.ase'
    # Read all inputs before any output replacement. All candidate files are
    # created in one private staging tree; invalid art leaves the pilot intact.
    if import_legacy:
        read_regular(legacy, 4 * 1024 * 1024)
    before = None if import_legacy else fingerprint(output)
    expected_inputs = {'recipe.json': text_checksum(output / 'recipe.json', 65536), 'shared_timing': text_checksum(TIMING),
                       **compiler_inputs()}
    if before:
        expected_inputs.update(before)

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        log = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', log):
            raise RuntimeError(log[-6000:])
        return log

    with tempfile.TemporaryDirectory(prefix='embervale-modular-axe-') as scratch:
        stage = Path(scratch)
        for folder in ('source', 'textures', 'previews'):
            (stage / folder).mkdir()
        count = len(recipe['materials'])
        sizes = {'body': [768, 96], 'shape': [32, 40], 'mask': [32, 40],
                 'tools': [768, count * 96], 'composites': [768, count * 96],
                 'comparison': [count * 96 * 3, 108 * 3], 'contact': [1536, count * 108 * 2],
                 'grips': [1536, 216]}
        fixtures = {}
        for name, size in sizes.items():
            target = stage / f'blank-{name}.png'
            blank_png(target, *size)
            fixtures[name] = target.as_posix()
        cfg = {'recipe': recipe, 'importLegacy': import_legacy, 'legacyBody': legacy.as_posix(),
               'sources': [(stage / name if import_legacy else output / name).as_posix() for name in SOURCES],
               'fixtures': fixtures, 'stage': stage.as_posix(),
               'loopFrames': [(stage / f'preview-{p:02}.png').as_posix() for p in range(8)]}
        helpers = Path(__file__).with_name('cast_pilot.js').read_text(encoding='utf-8').split('var player, mara, furnace;')[0]
        cfg['palette'] = []
        script = stage / 'compile.js'
        script.write_text((helpers + '\n' + Path(__file__).with_name('modular_axe_pilot.js').read_text(encoding='utf-8'))
                          .replace('__CONFIG__', json.dumps(cfg)), encoding='utf-8')
        log = run('--script', script)
        validation = json.loads(next(line.split('=', 1)[1] for line in log.splitlines()
                                     if line.startswith('MODULAR_AXE_JSON=')))
        run(cfg['loopFrames'][0], '--save-as', stage / 'previews/materials.gif',
            '--sheet', stage / 'unused-preview-sheet.png', '--data', stage / 'preview.json')
        require(len(json.loads((stage / 'preview.json').read_text())['frames']) == 8,
                'Comparison GIF must contain eight shared moving poses')
        retime_gif(stage / 'previews/materials.gif', recipe['frame_durations_ms'])
        (stage / 'previews/validation.json').write_text(json.dumps(validation, indent=2) + '\n', encoding='utf-8')
        require(text_checksum(output / 'recipe.json', 65536) == expected_inputs['recipe.json'], 'Recipe changed during export')
        require(text_checksum(TIMING) == expected_inputs['shared_timing'], 'Shared timing changed during export')
        require(all(expected_inputs[key] == value for key, value in compiler_inputs().items()),
                'Compiler inputs changed during export')
        if before:
            require(before == fingerprint(output), 'An editable master changed during export')
        inputs = {name: checksum(stage / name if import_legacy else output / name) for name in SOURCES}
        inputs['recipe.json'] = expected_inputs['recipe.json']
        inputs['shared_timing'] = expected_inputs['shared_timing']
        inputs.update(compiler_inputs())
        generated = sorted([*stage.glob('textures/*.png'), *stage.glob('previews/*')])
        report = {'schema': 'embervale.modular-equipment-proof', 'version': 1,
                  'status': 'East-facing authoring proof; outside runtime catalogue; existing body quality unaccepted',
                  'inputs': inputs, 'outputs': {p.relative_to(stage).as_posix(): output_checksum(p) for p in generated},
                  'coverage': {'facings': ['east'], 'body_poses': 8, 'shape_views': 1, 'materials': count,
                               'tool_pose_bindings': 8 * count},
                  'timing_ms': recipe['frame_durations_ms'], 'validation': validation,
                  'review_texture_rgba_bytes': sum(sizes[name][0] * sizes[name][1] * 4
                                            for name in ('body', 'shape', 'mask', 'tools', 'composites'))}
        if import_legacy:
            report['body_origin_sha256'] = checksum(legacy)
        else:
            prior = output / 'manifest.json'
            if prior.is_file():
                origin = json.loads(read_regular(prior, 65536)).get('body_origin_sha256')
                if origin:
                    report['body_origin_sha256'] = origin
        # Each replacement is atomic; the receipt is committed last. No runtime
        # consumer reads this prototype and no legacy batch is rewritten.
        publish = ([stage / name for name in SOURCES] if import_legacy else []) + generated
        for source in publish:
            destination = output / source.relative_to(stage)
            destination.parent.mkdir(parents=True, exist_ok=True)
            temporary = destination.with_name(destination.name + '.pending')
            shutil.copyfile(source, temporary)
            temporary.replace(destination)
        manifest = output / 'manifest.json'
        pending = manifest.with_name('manifest.json.pending')
        pending.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        pending.replace(manifest)
    return check(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', type=Path)
    parser.add_argument('--output', type=Path, default=PILOT)
    parser.add_argument('--import-legacy', action='store_true', help='Create missing proof masters once; never regenerate the body')
    parser.add_argument('--check', action='store_true', help='Verify the current recipe, masters and exported receipts without writes')
    args = parser.parse_args()
    if args.check:
        require(not args.import_legacy, '--check does not import or replace assets')
        result = check(args.output.resolve())
    else:
        require(args.editor is not None, '--editor is required for export')
        result = build(args.editor.resolve(strict=True), args.output, args.import_legacy)
    print(json.dumps(result, indent=2))
