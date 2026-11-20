"""Export editable eight-way melee, equipped walking and weapon layers with LibreSprite."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout
from build_cast_pilot import PALETTE
from animation_common import load_timing, write_timing_header, retime_gif

ART = REPO / 'assets/embervale/melee-pilot'
DIRECTIONS = ['s', 'w', 'e', 'n', 'sw', 'se', 'nw', 'ne']
WEAPONS = ['bronze_sword', 'developer_godsword', 'bronze_axe', 'iron_axe', 'steel_axe',
           'bronze_pickaxe', 'iron_pickaxe', 'steel_pickaxe', 'wooden_shield']


def build(editor, regenerate=False, compare_with=None):
    sizes = {'player': (1536, 3840), 'equipment': (1536, 6912)}
    timing = load_timing()
    write_timing_header()
    for folder in ['source', 'textures', 'previews']:
        (ART / folder).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART / f'source/{name}.ase').is_file() for name in sizes):
        raise RuntimeError('Missing melee masters; use --regenerate for initial authoring')

    def run(*args):
        result = subprocess.run([str(editor), '-b', *map(str, args)], cwd=editor.parent,
                                capture_output=True, timeout=180)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        if result.returncode or re.search(r'(^|[\r\n])Error:', output):
            raise RuntimeError(output[-6000:])
        return output

    with tempfile.TemporaryDirectory(prefix='embervale-melee-') as scratch:
        temp = Path(scratch)
        fixtures = {}
        for name, size in {**sizes, 'board': (1152, 576), 'contact': (1536, 1536), 'gear': (1728, 192)}.items():
            target = temp / f'{name}.png'
            blank_png(target, *size)
            fixtures[name] = target.as_posix()
        cfg = {'regenerate': regenerate, 'palette': PALETTE, 'fixtures': fixtures,
               'player': (REPO / 'assets/embervale/cast-pilot/source/player.ase').as_posix(),
               'sources': {name: (ART / f'source/{name}.ase').as_posix() for name in sizes},
               'frames': [(temp / f'melee-{i:02}.png').as_posix() for i in range(8)],
               'contact': (ART / 'previews/melee-contact.png').as_posix(),
               'gear': (ART / 'previews/equipment.png').as_posix()}
        if compare_with:
            comparison_fixture = temp / 'comparison.png'
            blank_png(comparison_fixture, 1152, 672)
            cfg['comparison'] = {
                'player': (compare_with / 'melee-player.png').resolve(strict=True).as_posix(),
                'equipment': (compare_with / 'melee-equipment.png').resolve(strict=True).as_posix(),
                'fixture': comparison_fixture.as_posix(),
                'frames': [(temp / f'compare-{frame:03}.png').as_posix() for frame in range(75)]}
        helpers = Path(__file__).with_name('cast_pilot.js').read_text(encoding='utf-8').split('var player, mara, furnace;')[0]
        script = temp / 'melee.js'
        rig = Path(__file__).with_name('player_rig.js').read_text(encoding='utf-8')
        script.write_text((helpers + '\n' + rig + '\n' + Path(__file__).with_name('melee_pilot.js').read_text(encoding='utf-8'))
                          .replace('__CONFIG__', json.dumps(cfg)), encoding='utf-8')
        output = run('--script', script)
        if 'MELEE_PILOT_OK' not in output:
            raise RuntimeError('Melee authoring did not complete')
        report = json.loads(next(line.split('=', 1)[1] for line in output.splitlines()
                                 if line.startswith('MELEE_VALIDATION_JSON=')))
        packing = json.loads(next(line.split('=', 1)[1] for line in output.splitlines()
                                  if line.startswith('MELEE_PACKING_JSON=')))
        pack_config = {'palette': PALETTE, 'sources': cfg['sources'], 'packing': packing, 'fixtures': {}, 'outputs': {}}
        for name, layout in packing.items():
            fixture = temp / f'packed-{name}.png'
            blank_png(fixture, *layout['size'])
            pack_config['fixtures'][name] = fixture.as_posix()
            pack_config['outputs'][name] = (ART / f'textures/{name}.png').as_posix()
        pack_script = temp / 'pack-melee.js'
        pack_script.write_text(helpers.replace('__CONFIG__', json.dumps(pack_config)) + '''
for (var name in cfg.packing) {
  var layout=cfg.packing[name],source=read(cfg.sources[name]),out=canvas(layout.size[0],layout.size[1]);
  layout.frames.forEach(function(frame) {
    var r=frame.source;
    blit(crop(source,r[0],r[1],r[2],r[3]),out,frame.rect[0],frame.rect[1]);
  });
  save(out,cfg.fixtures[name],cfg.outputs[name]);
  var packed=read(cfg.outputs[name]);
  layout.frames.forEach(function(frame,index) {
    var left=index%16*96,top=Math.floor(index/16)*96;
    for(var y=0;y<96;y++) for(var x=0;x<96;x++) {
      var expected=source.pixels[(top+y)*source.width+left+x],actual=0;
      var px=left+x-frame.source[0],py=top+y-frame.source[1];
      if(px>=0&&py>=0&&px<frame.rect[2]&&py<frame.rect[3])
        actual=packed.pixels[(frame.rect[1]+py)*packed.width+frame.rect[0]+px];
      if(pc.rgbaA(expected)!==pc.rgbaA(actual)||(pc.rgbaA(expected)&&expected!==actual))
        throw new Error('Packed '+name+' frame '+index+' changes a visible pixel');
    }
  });
}
console.log('MELEE_PACKED_OK');
''', encoding='utf-8')
        if 'MELEE_PACKED_OK' not in run('--script', pack_script):
            raise RuntimeError('Melee runtime packing did not complete')
        report['packed_images'] = {name: layout['size'] for name, layout in packing.items()}
        (ART / 'previews/validation.json').write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))
        run(cfg['frames'][0], '--save-as', ART / 'previews/melee-cycle.gif',
            '--data', temp / 'preview.json', '--sheet', temp / 'preview.png')
        if len(json.loads((temp / 'preview.json').read_text(encoding='utf-8'))['frames']) != 8:
            raise RuntimeError('Preview must contain eight poses')
        retime_gif(ART / 'previews/melee-cycle.gif', [300, 130, 90] + timing['profiles'][1]['recovery_ms'])
        if compare_with:
            comparison = REPO / 'docs/embervale/06-development/art/2026-11-20/animation-polish/melee-before-after.gif'
            comparison.parent.mkdir(parents=True, exist_ok=True)
            run(cfg['comparison']['frames'][0], '--save-as', comparison)
            retime_gif(comparison, [20] * 75)
    images = {}
    for name, source_size in sizes.items():
        source, image, metadata = ART / f'source/{name}.ase', ART / f'textures/{name}.png', ART / f'textures/{name}.export.json'
        size = packing[name]['size']
        data = {'schema': 'embervale.packed-sprite', 'version': 1, 'meta': {'image': image.name, 'size': size,
                'source_size': list(source_size), 'cell_size': [96, 96], 'source_pivot': [48, 64]},
                'frames': packing[name]['frames']}
        metadata.write_bytes((json.dumps(data, indent=2) + '\n').encode('utf-8'))
        if png_size(image) != tuple(size):
            raise RuntimeError('Incorrect melee texture size')
        images[name] = {'source': f'source/{name}.ase', 'image': f'textures/{name}.png', 'size': list(size),
                        'editor_metadata': f'textures/{name}.export.json', 'source_sha256': checksum(source),
                        'image_sha256': checksum(image)}
    assets = []
    for image, names, prefix in [('player', ['unarmed', 'sword', 'two_handed', 'axe', 'pickaxe'], 'character'),
                                  ('equipment', WEAPONS, 'equipment')]:
        for variant, name in enumerate(names):
            for facing, direction in enumerate(DIRECTIONS):
                for pose in range(16):
                    packed = packing[image]['frames'][(variant * 8 + facing) * 16 + pose]
                    assets.append({'id': f'{prefix}.melee.{name}.{direction}.{pose}', 'image': image,
                                   'rect': packed['rect'], 'pivot': packed['pivot'],
                                   'intended_ground_point_in_tile': [16, 24],
                                   'variant': variant, 'facing': facing, 'animation_frame': pose})
    manifest = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'melee-pilot',
                'native_tile_size': 32, 'editor': 'LibreSprite 1.1-dev', 'palette_rgb': PALETTE,
                'status': 'Recovered first-pass eight-way equipped walking and melee; visual acceptance remains open',
                'facing_order': DIRECTIONS, 'poses': ['idle'] + [f'walk{i}' for i in range(1,8)] +
                    ['guard', 'coil', 'loaded', 'contact', 'overshoot', 'follow_through', 'return', 'settle'],
                'timing_source': '../player-animation.json',
                'images': images, 'assets': assets,
                'provenance': {'creator': 'Codex-assisted LibreSprite authoring for Adam Holland',
                               'source': '../cast-pilot/source/player.ase',
                               'source_sha256': checksum(REPO / 'assets/embervale/cast-pilot/source/player.ase'),
                               'notes': 'Original outfit retained; new arms, attack poses, swords, axes, pickaxes and shield authored through LibreSprite JavaScript.',
                               'licence': 'No separate asset licence grant declared; repository distribution policy applies.'},
                'previews': {name: {'path': f'previews/{name}', 'sha256': checksum(ART / f'previews/{name}')}
                             for name in ['melee-cycle.gif', 'melee-contact.png', 'equipment.png']}}
    (ART / 'manifest.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode('utf-8'))
    write_renderer_layout(json.loads((REPO / 'assets/embervale/forest-pilot/manifest.json').read_text(encoding='utf-8')))
    print(json.dumps(report))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor', required=True, type=Path)
    parser.add_argument('--regenerate', action='store_true', help='Recreate masters from the current eight-way outfit')
    parser.add_argument('--compare-with', type=Path, help='Directory containing baseline melee-player.png and melee-equipment.png')
    args = parser.parse_args()
    build(args.editor.resolve(strict=True), args.regenerate, args.compare_with)
