"""Lossless source packing and integer-scale evidence; never paints source art."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json,re,base64
OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]
V4=OUT.parent/'coverage-fit-v4'
def frames(path,cell=80):
    im=Image.open(path).convert('RGBA');assert im.size==(12*cell,cell)
    return [im.crop((i*cell,0,(i+1)*cell,cell)) for i in range(12)]
def packed(name):
    result=[]
    for f in frames(OUT/f'source/{name}-raw.png',84):
        b=f.getbbox();assert b and min(b[:2])>=2 and max(b[2:])<=82
        result.append(f.crop((2,2,82,82)))
    return result
def atlas(fs,path):
    im=Image.new('RGBA',(960,80))
    for i,f in enumerate(fs):im.paste(f,(i*80,0))
    im.save(path)
def font(size,bold=False):
    for p in ['C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',
              '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']:
        if Path(p).is_file():return ImageFont.truetype(p,size)
    return ImageFont.load_default()
small=font(17);large=font(25,True)
body=frames(BASE/'v1/layers/body-south.png');old=frames(V4/'exports/dressed-full-south.png')
top=packed('tunic');pants={v:packed(v) for v in ['knee','full']};outfit={};dressed={}
atlas(top,OUT/'exports/tunic-south.png')
for v in ['knee','full']:
    outfit[v]=[Image.alpha_composite(p,t) for p,t in zip(pants[v],top)]
    dressed[v]=[Image.alpha_composite(b,o) for b,o in zip(body,outfit[v])]
    for kind,fs in [('trousers',pants[v]),('outfit',outfit[v]),('dressed',dressed[v])]:atlas(fs,OUT/f'exports/{kind}-{v}-south.png')
def comparison(i):
    im=Image.new('RGB',(1440,460),'#152027');d=ImageDraw.Draw(im)
    d.text((24,14),'POSE-REGISTERED CLOTHING',font=large,fill='#e8ede3')
    d.text((24,49),'Original body, previous full-length outfit, revised knee length and revised full length. Exact 4x zoom.',font=small,fill='#a4b7b0')
    for x,label,f in [(24,'ORIGINAL BODY',body[i]),(376,'PREVIOUS V4',old[i]),(728,'REVISED KNEE LENGTH',dressed['knee'][i]),(1080,'REVISED FULL LENGTH',dressed['full'][i])]:
        d.text((x+12,90),label,font=small,fill='#b8cbbf')
        z=f.resize((320,320),Image.Resampling.NEAREST);im.paste(z,(x+12,124),z)
    return im
comparison(4).save(OUT/'previews/before-after.png')
walk=[comparison(i) for i in range(4,12)]
walk[0].save(OUT/'previews/before-after-walk.gif',save_all=True,append_images=walk[1:],duration=[80,70]*4,loop=0,disposal=2,optimize=False)
for v in ['knee','full']:
    im=Image.new('RGB',(1440,1120),'#152027');d=ImageDraw.Draw(im)
    d.text((24,14),v.upper()+' LENGTH - ALL TWELVE POSES - 4X PIXELS',font=large,fill='#e8ede3')
    for i,f in enumerate(dressed[v]):
        x=16+(i%4)*356;y=58+(i//4)*350
        d.text((x+22,y),('IDLE '+str(i+1)) if i<4 else ('WALK '+str(i-3)),font=small,fill='#b8cbbf')
        z=f.resize((320,320),Image.Resampling.NEAREST);im.paste(z,(x+12,y+22),z)
    im.save(OUT/f'previews/all-poses-{v}.png')
# One detailed review board per pose, with consistent 4x overview and 8x crops.
(OUT/'previews/frames').mkdir(exist_ok=True)
for i in range(12):
    im=Image.new('RGB',(1480,860),'#152027');d=ImageDraw.Draw(im)
    d.text((24,14),f'POSE {i+1:02} - BODY REGISTRATION AND PIXEL REVIEW',font=large,fill='#e8ede3')
    for x,label,f in [(10,'ORIGINAL BODY',body[i]),(380,'V4 FULL LENGTH',old[i]),(750,'V5 KNEE LENGTH',dressed['knee'][i]),(1120,'V5 FULL LENGTH',dressed['full'][i])]:
        d.text((x+22,56),label,font=small,fill='#b8cbbf')
        z=f.resize((320,320),Image.Resampling.NEAREST);im.paste(z,(x+20,85),z)
    for x,label,f,box in [(20,'SHOULDERS / SLEEVES',dressed['full'][i],(22,21,58,41)),
                          (400,'HIPS / THIGHS',dressed['knee'][i],(26,40,54,60)),
                          (730,'ANKLES / CUFFS',dressed['full'][i],(26,52,54,72)),
                          (1060,'ISOLATED TOP',top[i],(22,21,58,51))]:
        d.text((x,442),label,font=small,fill='#b8cbbf')
        z=f.crop(box).resize(((box[2]-box[0])*8,(box[3]-box[1])*8),Image.Resampling.NEAREST);im.paste(z,(x,480),z)
    im.save(OUT/f'previews/frames/frame-{i+1:02}.png')
html=(V4/'character-workshop.html').read_text(encoding='utf-8')
m=re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S);assert m
data=json.loads(m[1])
for key,path in [('tunic-south','tunic-south'),('trousers-south','trousers-full-south'),
                 ('outfit-south','outfit-full-south'),('dressed-south','dressed-full-south'),
                 ('trousers-full-south','trousers-full-south'),('trousers-knee-south','trousers-knee-south')]:
    data['images'][key]='data:image/png;base64,'+base64.b64encode((OUT/f'exports/{path}.png').read_bytes()).decode()
html=html[:m.start(1)]+json.dumps(data,separators=(',',':'))+html[m.end(1):]
html=html.replace('Embervale Garment Coverage v4','Embervale Garment Alignment v5').replace('Garment coverage revision 04','Garment alignment revision 05')
html=html.replace('Covered shoulders and thighs, with knee-length or full-length pants on the same body motion.','Clothing registered to every original pose, with knee-length and full-length options.')
(OUT/'character-workshop.html').write_bytes(html.encode())
print(json.dumps({'frames':12,'variants':['knee','full'],'overview_zoom':4,'detail_zoom':8,'body_unchanged':True}))
