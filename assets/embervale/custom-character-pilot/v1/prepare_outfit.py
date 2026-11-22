"""Inspect original PixelLab poses and emit exact outfit-mask decisions.

This script does not redraw bodies or rasterize clothing. Apply the JSON with
PixelLab pixelart_workbench edit, then retain that returned transparent artwork.
"""
from pathlib import Path
from PIL import Image
import json

ROOT = Path(__file__).resolve().parent
def get_frames():
    result = []
    for name, count in [('idle-south-raw.png', 4), ('walk-south-raw.png', 8)]:
        im = Image.open(ROOT / 'source' / name).convert('RGBA')
        assert im.size == (count*84, 84), (name, im.size)
        result.extend(im.crop((i*84, 0, (i+1)*84, 84)) for i in range(count))
    return result

def rgba(p):
    return '#' + ''.join(f'{x:02x}' for x in p)

def ramp_color(p, ramp):
    lum = .2126*p[0] + .7152*p[1] + .0722*p[2]
    # Deliberate cloth ramps: fewer shades than the detailed skin shading.
    index = sum(lum >= threshold for threshold in [35, 70, 110, 150, 190])
    return ramp[min(index, len(ramp)-1)]

SHIRT = ['#10292aff','#163c3cff','#20534fff','#2f6c61ff','#488574ff','#71a18aff']
PANTS = ['#241d23ff','#352a2aff','#49382fff','#624a38ff','#7c6247ff','#97805aff']
frames = get_frames()
decisions, tracks = [], []
for i, im in enumerate(frames, 1):
    grey = [(x,y) for y in range(38,55) for x in range(29,55)
            if (p:=im.getpixel((x,y)))[3] and max(p[:3])-min(p[:3]) <= 18
            and 25 <= max(p[:3]) <= 140]
    assert grey, f'No waistband found in frame {i}'
    x0, x1 = min(x for x,y in grey), max(x for x,y in grey)
    waist = min(y for x,y in grey)
    cx = (x0+x1)//2
    shirt = [[cx-7,waist-13],[cx-3,waist-15],[cx,waist-11],
             [cx+3,waist-15],[cx+7,waist-13],[cx+6,waist-4],
             [cx+7,waist+2],[cx-7,waist+2],[cx-6,waist-4]]
    pants = [[x0,waist],[x1,waist],[x1+2,waist+15],[x0-2,waist+15]]
    tracks.append({'frame':i,'waist':waist,'center_x':cx,
                   'shirt_polygon':shirt,'pants_polygon':pants,
                   'body_bbox':list(im.getbbox())})
    common={'layer':'image','frames':[i]}
    for name, poly in [('shirt',shirt),('pants',pants)]:
        decisions.append(dict(common,operation='select_mask',name=name,
            selector={'kind':'polygon','points':poly},reason='Follow authored body pose'))
    decisions.append(dict(common,operation='select_mask',name='outside',
        selector={'kind':'rectangle','region':[0,0,83,83]},reason='Isolate clothing only'))
    for poly in [shirt,pants]:
        decisions.append(dict(common,operation='select_mask',name='outside',combine='subtract',
            selector={'kind':'polygon','points':poly},reason='Keep clothing region'))
    decisions.append(dict(common,operation='brush',mask='outside',mode='erase',
        points=[[42,32],[42,52]],size=64,shape='square',reason='Remove body from clothing layer'))
    # Both maps read the source's existing shades. The masks preserve transparency.
    colors=sorted(set(im.get_flattened_data()))
    for name, ramp in [('pants',PANTS),('shirt',SHIRT)]:
        mapping={rgba(p):ramp_color(p,ramp) for p in colors if p[3]}
        decisions.append(dict(common,operation='map_colors',mask=name,
            region=[0,0,83,83],colors=mapping,reason='Shade cloth without changing pose'))

(ROOT/'outfit-decisions.json').write_text(json.dumps(decisions,indent=2)+'\n',encoding='utf-8')
(ROOT/'outfit-tracks.json').write_text(json.dumps(tracks,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'frames':len(frames),'operations':len(decisions),'tracks':tracks}))
