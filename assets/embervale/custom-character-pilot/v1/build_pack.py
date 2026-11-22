"""Package PixelLab source pixels into a checked, reusable character pilot.

Only lossless padding/cropping, layer separation, atlas packing and preview
composition happen here. Body poses and clothing artwork come from PixelLab.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import base64
import hashlib
import json
import numpy as np

ROOT = Path(__file__).resolve().parent
DIRECTIONS = ['south','west','east','north','south-west','south-east','north-west','north-east']
CELL=80
PIVOT=[40,70]
WALK_MS=[80,70,80,70,80,70,80,70]
IDLE_MS=[400,400,400,400]
SHIRT=['#10292a','#163c3c','#20534f','#2f6c61','#488574','#71a18a']
PANTS=['#241d23','#352a2a','#49382f','#624a38','#7c6247','#97805a']

def save_json(name,data):
    (ROOT/name).write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')

def data_url(im):
    import io
    out=io.BytesIO(); im.save(out,format='PNG')
    return 'data:image/png;base64,'+base64.b64encode(out.getvalue()).decode('ascii')

def atlas(frames):
    out=Image.new('RGBA',(len(frames)*CELL,CELL))
    for i,f in enumerate(frames):
        assert f.size==(CELL,CELL)
        out.paste(f,(i*CELL,0))
    return out

def material_only(im,colors):
    a=np.array(im).copy()
    allowed={tuple(bytes.fromhex(c[1:])) for c in colors}
    mask=np.array([tuple(p[:3]) in allowed and p[3]>0 for p in a.reshape(-1,4)]).reshape(CELL,CELL)
    a[~mask]=0
    return Image.fromarray(a)

proof=Image.open(ROOT/'source/layer-proof-raw.png').convert('RGBA')
assert proof.size==(1008,252)
raw=[[proof.crop((i*84,j*84,(i+1)*84,(j+1)*84)) for i in range(12)] for j in range(3)]
originals=[]
for name,n in [('idle-south-raw.png',4),('walk-south-raw.png',8)]:
    im=Image.open(ROOT/'source'/name).convert('RGBA')
    originals.extend(im.crop((i*84,0,(i+1)*84,84)) for i in range(n))
checks=[]
for i,(body,clothes,dressed) in enumerate(zip(*raw)):
    a,b,c=np.array(body),np.array(clothes),np.array(dressed)
    assert np.array_equal(a,np.array(originals[i])),f'Body changed in frame {i}'
    assert set(np.unique(a[:,:,3]))<={0,255}
    assert set(np.unique(b[:,:,3]))<={0,255}
    assert not np.any((b[:,:,3]>0)&(a[:,:,3]==0)),f'Clothing leaves silhouette in {i}'
    expected=a.copy(); mask=b[:,:,3]>0; expected[mask]=b[mask]
    assert np.array_equal(expected,c),f'Composition mismatch in {i}'
    # Remove two transparent pixels of service padding on every edge, never art.
    for image in [body,clothes,dressed]:
        bounds=image.getbbox()
        assert bounds and bounds[0]>=2 and bounds[1]>=2 and bounds[2]<=82 and bounds[3]<=82
    checks.append({'frame':i,'source_pixels_exact':True,'off_body_clothing_pixels':0,
                   'composition_exact':True,'transparent_crop_lossless':True})

body=[f.crop((2,2,82,82)) for f in raw[0]]
outfit=[f.crop((2,2,82,82)) for f in raw[1]]
dressed=[f.crop((2,2,82,82)) for f in raw[2]]
tunic=[material_only(f,SHIRT) for f in outfit]
trousers=[material_only(f,PANTS) for f in outfit]
for b,t,p,o in zip(body,tunic,trousers,outfit):
    a=np.array(t); c=np.array(p)
    assert not np.any((a[:,:,3]>0)&(c[:,:,3]>0))
    assert np.array_equal(np.array(Image.alpha_composite(p,t)),np.array(o))
    assert Image.alpha_composite(b,o).tobytes()==Image.alpha_composite(Image.alpha_composite(b,p),t).tobytes()

images={}
for name,fs in [('body-south',body),('outfit-south',outfit),('tunic-south',tunic),('trousers-south',trousers),('dressed-south',dressed)]:
    im=atlas(fs); im.save(ROOT/'layers'/f'{name}.png'); images[name]=im

rotations=Image.open(ROOT/'source/rotations.png').convert('RGBA')
standing=[]; ground_adjustments=[]
for i,direction in enumerate(DIRECTIONS):
    source=rotations.crop((0,i*64,64,(i+1)*64))
    bounds=source.getbbox(); dy=62-bounds[3]
    target=Image.new('RGBA',(CELL,CELL)); target.paste(source,(8,8+dy))
    assert target.getbbox()[3]==PIVOT[1]
    assert sum(target.getchannel('A').get_flattened_data())==sum(source.getchannel('A').get_flattened_data())
    standing.append(target); ground_adjustments.append({'direction':direction,'source_offset':[8,8+dy]})
images['standing']=atlas(standing)
images['standing'].save(ROOT/'layers/body-standing-eight-way.png')

font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',16)
small=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',13)
large=ImageFont.truetype('C:/Windows/Fonts/segoeuib.ttf',26)
def background():
    im=Image.new('RGB',(1120,530),'#111a20'); d=ImageDraw.Draw(im)
    d.text((34,22),'EMBERVALE  /  CUSTOM CHARACTER BASE',font=large,fill='#e9eddf')
    d.text((35,61),'Eight standing views • front-facing idle and walk • separate tunic and trousers',font=font,fill='#9eadae')
    for x,label in [(35,'BASE BODY'),(315,'OUTFIT LAYER'),(595,'COMBINED')]:
        d.rounded_rectangle((x,104,x+248,385),radius=12,fill='#1b292f',outline='#30464b')
        d.text((x+16,119),label,font=font,fill='#a6c4ba')
        d.line((x+12,366,x+236,366),fill='#304b49',width=1)
    d.text((875,116),'ONE BODY MOTION',font=font,fill='#dfd7b0')
    d.text((875,150),'Switch outfit on/off',font=small,fill='#b8c6c3')
    d.text((875,176),'Change cloth colours',font=small,fill='#b8c6c3')
    d.text((875,202),'Keep the same poses',font=small,fill='#b8c6c3')
    d.text((875,314),'South-facing pilot',font=small,fill='#90a6a5')
    d.text((875,340),'Not installed in game',font=small,fill='#90a6a5')
    for i,(name,imrot) in enumerate(zip(DIRECTIONS,standing)):
        x=35+i*132
        im.paste(imrot,(x+24,408),imrot)
        d.text((x+12,491),name.replace('-',' ').title(),font=small,fill='#a8bab6')
    return im

gif=[]
for i in range(4,12):
    frame=background()
    for x,im in [(35,body[i]),(315,outfit[i]),(595,dressed[i])]:
        zoom=im.resize((240,240),Image.Resampling.NEAREST)
        frame.paste(zoom,(x+4,139),zoom)
    gif.append(frame)
gif[0].save(ROOT/'previews/character-layer-preview.gif',save_all=True,append_images=gif[1:],duration=WALK_MS,loop=0,disposal=2,optimize=False)
gif[0].save(ROOT/'previews/character-layer-preview.png')

# Exact palette maps used only by the interactive review canvas. They never
# overwrite the original generated body or an item layer.
all_colors=set()
for f in body+standing: all_colors.update(p[:3] for p in f.get_flattened_data() if p[3])
skin=sorted([p for p in all_colors if p[0]>p[1]+8 and p[1]>p[2]+4],key=lambda p:sum(p))
def skin_map(stops):
    result={}
    for p in skin:
        lum=.2126*p[0]+.7152*p[1]+.0722*p[2]
        t=max(0,min(1,(lum-12)/205))*(len(stops)-1)
        lo=min(int(t),len(stops)-2); q=t-lo
        rgb=tuple(round(stops[lo][j]*(1-q)+stops[lo+1][j]*q) for j in range(3))
        result[','.join(map(str,p))]=rgb
    return result
skin_maps={'original':{},'warm':skin_map([(48,22,19),(111,57,31),(178,112,58),(226,170,103),(255,218,162)]),
           'deep':skin_map([(28,17,20),(58,31,29),(102,58,40),(147,94,61),(195,144,96)])}

manifest={
 'schema':'embervale.modular-character-pilot','version':1,'date':'2026-11-22',
 'status':'art and customisation proof; not installed into the game',
 'character_id':'506e9990-e9e3-4771-b17f-892c787939d1',
 'character_url':'https://www.pixellab.ai/create-character/506e9990-e9e3-4771-b17f-892c787939d1',
 'cell_size':[80,80],'ground_pivot':PIVOT,'native_tile_size':32,
 'source_geometry':{'standing_cells':[64,64],'animated_cells':[84,84],
    'animation_export_crop':[2,2,82,82],'standing_offsets':ground_adjustments,
    'rescaled_or_deformed_body_pixels':False},
 'direction_order':DIRECTIONS,
 'standing':{'image':'layers/body-standing-eight-way.png','frames':[
    {'direction':d,'rect':[i*CELL,0,CELL,CELL],'pivot':PIVOT} for i,d in enumerate(DIRECTIONS)]},
 'layers':{'body':'layers/body-south.png','trousers':'layers/trousers-south.png',
           'tunic':'layers/tunic-south.png','outfit_reference':'layers/outfit-south.png',
           'dressed_reference':'layers/dressed-south.png'},
 'draw_order':['body','trousers','tunic'],
 'clips':{'idle':{'direction':'south','columns':[0,1,2,3],'durations_ms':IDLE_MS,'loop':True,
                 'group_id':'249aa0d8-8620-4863-853b-f294efdaa4b0'},
          'walk':{'direction':'south','columns':list(range(4,12)),'durations_ms':WALK_MS,
                  'loop':True,'one_cycle_per_tile_at_ordinary_walk':True,
                  'group_id':'21093bb9-a630-4aaf-8252-60ec1414f034'}},
 'coverage':{'standing':DIRECTIONS,'idle':['south'],'walk':['south'],
             'outfit_idle':['south'],'outfit_walk':['south']},
 'outfit':{'name':'Starter sleeveless tunic and knee-length trousers',
    'type':'close-fitting palette-and-mask clothing proof',
    'tunic_ramp':SHIRT,'trousers_ramp':PANTS,
    'source':'PixelLab exact pixel workbench decisions on original generated poses',
    'new_body_generations_per_colour':0},
 'cost':{'base_eight_directions':2,'south_idle':1,'south_walk':1,
         'outfit_layers_and_packaging':0,'total_generations':4,
         'verified_account_generations_remaining':4.75},
 'known_limits':['Animated body and outfit cover south only; other facings are standing body views.',
    'This body is taller than the existing player. The exported 80x80 cells and (40,70) pivot are not a drop-in replacement for the old 64x64, (32,48) binding.',
    'Hair, beards, extra body shapes, equipment hand tracks and gameplay integration are not part of this pilot.',
    'Close-fitting cloth follows source shading. Loose sleeves, skirts and bulky armour require their own silhouette and occlusion work.',
    'Skin and cloth colour controls are review palette examples, not owner-accepted final character designs.']
}
save_json('manifest.json',manifest)
save_json('validation.json',{'status':'passed','frames_checked':12,'source_bodies_preserved':True,
   'transparent_padding_crop_lossless':True,'off_body_clothing_pixels':0,
   'composition_matches_pixellab_dressed_frames':True,'separate_material_layers_match_outfit':True,
   'standing_foot_pivots_aligned':8,'checks':checks,
   'game_renderer_or_gameplay_tests_run':False,'owner_visual_acceptance':'pending'})
save_json('palette-options.json',{'skin':skin_maps,'tunic':{
    'teal':SHIRT,'rust':['#331b24','#54262a','#793b35','#a45640','#c77853','#e5a575'],
    'blue':['#192b3b','#243d56','#315877','#477697','#6795b1','#96bfd0']},
    'trousers':PANTS})
payload={'images':{k:data_url(v) for k,v in images.items()},'skinMaps':skin_maps,
         'shirtRamp':SHIRT,'shirtPalettes':json.loads((ROOT/'palette-options.json').read_text())['tunic'],
         'walkMs':WALK_MS,'idleMs':IDLE_MS}
template=(ROOT/'review-template.html').read_text(encoding='utf-8')
(ROOT/'character-workshop.html').write_text(template.replace('/*PAYLOAD*/',json.dumps(payload,separators=(',',':'))),encoding='utf-8')
hashes={str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted(ROOT.rglob('*')) if p.is_file() and p.name not in ['sha256.json'] and '__pycache__' not in p.parts}
save_json('sha256.json',hashes)
print(json.dumps({'status':'passed','body_frames':12,'standing_views':8,'separate_clothing_layers':2,
                  'cell':[80,80],'pivot':PIVOT,'generations_used':4,'html_bytes':(ROOT/'character-workshop.html').stat().st_size}))
