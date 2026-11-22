"""Lossless packing and review assembly of retained PixelLab trousers.
Artwork is read from source/trousers-raw.png, never redrawn here.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json, base64, re
OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]
V2=OUT.parent/'tunic-fit-v2'
def frames(path,cell=80):
    im=Image.open(path).convert('RGBA')
    assert im.size==(cell*12,cell)
    return [im.crop((i*cell,0,(i+1)*cell,cell)) for i in range(12)]
source=frames(OUT/'source/trousers-raw.png',84)
for f in source:
    b=f.getbbox();assert b and b[0]>=2 and b[1]>=2 and b[2]<=82 and b[3]<=82
pants=[f.crop((2,2,82,82)) for f in source]
body=frames(BASE/'v1/layers/body-south.png')
tunic=frames(V2/'exports/tunic-south.png')
old=frames(V2/'exports/dressed-south.png')
outfit=[Image.alpha_composite(p,t) for p,t in zip(pants,tunic)]
dressed=[Image.alpha_composite(b,o) for b,o in zip(body,outfit)]
def atlas(fs):
    im=Image.new('RGBA',(960,80))
    for i,f in enumerate(fs):im.paste(f,(i*80,0))
    return im
for name,fs in [('trousers-south',pants),('outfit-south',outfit),('dressed-south',dressed)]:
    atlas(fs).save(OUT/f'exports/{name}.png')
def font(size,bold=False):
    choices=['C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',
             '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']
    for p in choices:
        if Path(p).is_file():return ImageFont.truetype(p,size)
    return ImageFont.load_default()
small=font(16);large=font(25,True)
def before_after(i):
    im=Image.new('RGB',(1040,420),'#111a20');d=ImageDraw.Draw(im)
    d.text((28,16),'TROUSER FIT REVISION',font=large,fill='#e8ede3')
    d.text((28,51),'Same body and tunic. New leg fit, crotch, fabric folds and cuffs.',font=small,fill='#a4b7b0')
    for x,label,f in [(28,'PREVIOUS TROUSERS',old[i]),(364,'REVISED TROUSERS',dressed[i]),(700,'NEW TROUSER LAYER',pants[i])]:
        d.rounded_rectangle((x,87,x+312,406),12,fill='#1d2b30',outline='#344c4d')
        d.text((x+16,100),label,font=small,fill='#b8cbbf')
        z=f.resize((280,280),Image.Resampling.NEAREST);im.paste(z,(x+16,123),z)
    return im
before_after(4).save(OUT/'previews/before-after.png')
walk=[before_after(i) for i in range(4,12)]
walk[0].save(OUT/'previews/before-after-walk.gif',save_all=True,append_images=walk[1:],duration=[80,70]*4,loop=0,disposal=2,optimize=False)
def sheet(fs,title,name):
    im=Image.new('RGB',(1000,850),'#111a20');d=ImageDraw.Draw(im)
    d.text((24,16),title,font=large,fill='#e8ede3')
    for i,f in enumerate(fs):
        x=20+(i%4)*245;y=62+(i//4)*260
        d.text((x+18,y),('IDLE '+str(i+1)) if i<4 else ('WALK '+str(i-3)),font=small,fill='#b8cbbf')
        z=f.resize((240,240),Image.Resampling.NEAREST);im.paste(z,(x,y+18),z)
    im.save(OUT/'previews'/name)
sheet(dressed,'REVISED TROUSERS - ALL TWELVE POSES','all-poses.png')
sheet([Image.alpha_composite(b,p) for b,p in zip(body,pants)],'HIP AND LEG FIT - TUNIC HIDDEN FOR REVIEW','fit-study.png')
html=(V2/'character-workshop.html').read_text(encoding='utf-8')
m=re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S);assert m
data=json.loads(m[1])
for key in ['trousers-south','outfit-south','dressed-south']:
    data['images'][key]='data:image/png;base64,'+base64.b64encode((OUT/f'exports/{key}.png').read_bytes()).decode()
html=html[:m.start(1)]+json.dumps(data,separators=(',',':'))+html[m.end(1):]
html=html.replace('Tunic fit revision 02','Trouser fit revision 03').replace('Embervale Tunic Fit v2','Embervale Trouser Fit v3')
html=html.replace('Revised shoulder fit, sleeves, neckline and hem on the same body motion.','Revised trouser fit, separate leg panels and cuffs; same body motion and v2 tunic.')
(OUT/'character-workshop.html').write_bytes(html.encode())
print(json.dumps({'frames':12,'revision':'trouser-fit-v3','body_and_tunic_unchanged':True}))
