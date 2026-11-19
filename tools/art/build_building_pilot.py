"""Author/export a modular, overhead timber-and-stone building kit in LibreSprite."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile
from build_forest_pilot import REPO, blank_png, checksum, png_size, write_renderer_layout

ART = REPO / 'assets/embervale/building-pilot'
SIZES = {'walls': (256, 96), 'roof': (96, 128), 'floors': (96, 32)}
PREVIEWS = {'building-pilot-2x.png': (1536, 512), 'walls-inspection-3x.png': (768, 288)}
WALLS = ['front', 'back', 'west', 'east', 'window-front', 'window-west', 'window-east',
         'doorway', 'corner-nw', 'corner-ne', 'corner-sw', 'corner-se']
ROOF = ['nw', 'n', 'ne', 'w', 'center', 'e', 'sw', 's', 'se', 'ridge-w', 'ridge', 'ridge-e']

def build(editor, regenerate=False):
    for name in ['source', 'textures', 'previews']:
        (ART/name).mkdir(parents=True, exist_ok=True)
    if not regenerate and any(not (ART/f'source/{name}.ase').is_file() for name in SIZES):
        raise RuntimeError('Building masters missing; use --regenerate for initial authoring')
    def run(*args):
        r = subprocess.run([str(editor), '-b', *map(str,args)], cwd=editor.parent, capture_output=True, timeout=180)
        out = (r.stdout+r.stderr).decode('utf-8',errors='replace')
        if r.returncode or re.search(r'(^|[\r\n])Error:',out): raise RuntimeError(out[-4000:])
        return out
    with tempfile.TemporaryDirectory(prefix='embervale-building-') as scratch:
        temp = Path(scratch); fixtures = {}
        for name,size in {**SIZES,**PREVIEWS}.items():
            p = temp/(name if name.endswith('.png') else name+'.png')
            blank_png(p,*size); fixtures[name] = p.as_posix()
        cfg = {'regenerate':regenerate,'fixtures':fixtures,
               'sources':{name:(ART/f'source/{name}.ase').as_posix() for name in SIZES},
               'forest':(REPO/'assets/embervale/forest-pilot/textures/terrain.png').as_posix(),
               'stone':(REPO/'assets/embervale/environment-pilot/textures/terrain.png').as_posix(),
               'previews':{name:(ART/'previews'/name).as_posix() for name in PREVIEWS}}
        script = temp/'building.js'
        script.write_text(Path(__file__).with_name('building_pilot.js').read_text().replace('__CONFIG__',json.dumps(cfg)),encoding='utf-8')
        out = run('--script',script)
        if 'BUILDING_PILOT_OK:' not in out: raise RuntimeError('Building authoring did not complete: '+out[-2000:])
        print(out[out.index('BUILDING_PILOT_OK:'):].strip())
    images = {}
    for name,size in SIZES.items():
        source,image,metadata = (ART/f'source/{name}.ase',ART/f'textures/{name}.png',ART/f'textures/{name}.export.json')
        run(source,'--sheet',image,'--data',metadata)
        data = json.loads(metadata.read_text()); data['meta']['image'] = image.name
        metadata.write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
        if png_size(image)!=size or data['meta']['size']!=dict(zip(['w','h'],size)): raise RuntimeError('Export size mismatch: '+name)
        images[name] = {'source':f'source/{name}.ase','image':f'textures/{name}.png','size':list(size),
            'editor_metadata':f'textures/{name}.export.json','source_sha256':checksum(source),'image_sha256':checksum(image)}
    assets = []
    for i,name in enumerate(WALLS):
        assets.append({'id':'building.wall.'+name,'image':'walls','rect':[i%8*32,i//8*48,32,48],
            'pivot':[16,40],'intended_ground_point_in_tile':[16,24]})
    for i,name in enumerate(ROOF):
        assets.append({'id':'building.roof.'+name,'image':'roof','rect':[i%3*32,i//3*32,32,32],'pivot':[0,0]})
    for i in range(3):
        assets.append({'id':f'building.floor.{i+1:02}','image':'floors','rect':[i*32,0,32,32],'pivot':[0,0]})
    manifest = {'schema':'embervale.authoring-batch','version':1,'id':'building-pilot','native_tile_size':32,
        'editor':'LibreSprite (authored with 1.3; existing JavaScript batch API)',
        'status':'Modular timber/stone starter building kit; first authored workshop shell',
        'projection':'Overhead tile grid with shallow visible wall fronts; no isometric rotation',
        'palette_rgb':sorted(set(re.findall(r"'([0-9a-f]{6})'", Path(__file__).with_name('building_pilot.js').read_text())) - {'b4c5cf','e5ded0'}),
        'lighting':'upper left','images':images,'assets':assets,
        'provenance':{'creator':'OpenAI image generation and Codex-assisted LibreSprite authoring for Adam Holland',
            'reference':'reference/workshop-reference.png','reference_sha256':checksum(ART/'reference/workshop-reference.png'),
            'notes':'Original architectural reference with no third-party input. Final overhead tiles are drawn at exact native pixel size in LibreSprite JavaScript; the frontal reference is not pasted into the game.',
            'licence':'No separate asset licence grant declared; repository distribution policy applies.'},
        'previews':{name:{'path':'previews/'+name,'size':list(size),'sha256':checksum(ART/'previews'/name)} for name,size in PREVIEWS.items()}}
    (ART/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    write_renderer_layout(json.loads((REPO/'assets/embervale/forest-pilot/manifest.json').read_text()))
    print(json.dumps({'frames':len(assets),'masters':len(images)}))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--editor',required=True,type=Path)
    parser.add_argument('--regenerate',action='store_true',help='Replace the three editable masters with the native-pixel originals')
    args=parser.parse_args(); build(args.editor.resolve(strict=True),args.regenerate)
