"""Assemble retained PixelLab garment pixels, exports and review evidence.
Does not redraw body poses or paint garment artwork.
"""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import numpy as np, json,base64,re,hashlib
BASE=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
raw=Image.open(OUT/'source/tunic-raw.png').convert('RGBA')
assert raw.size==(1008,84)
shirt=[];dressed=[];old=[];body=[];pants=[]
for i in range(12):
    f=raw.crop((84*i,0,84*(i+1),84));b=f.getbbox()
    assert b and min(b[:2])>=2 and max(b[2:])<=82
    shirt.append(f.crop((2,2,82,82)))
for filename,frames in [('body-south.png',body),('trousers-south.png',pants),('dressed-south.png',old)]:
    im=Image.open(BASE/'v1/layers'/filename).convert('RGBA')
    frames.extend(im.crop((i*80,0,(i+1)*80,80)) for i in range(12))
for b,p,t in zip(body,pants,shirt):dressed.append(Image.alpha_composite(Image.alpha_composite(b,p),t))
def atlas(fs):
    im=Image.new('RGBA',(len(fs)*80,80))
    for i,f in enumerate(fs):im.paste(f,(80*i,0))
    return im
atlas(shirt).save(OUT/'exports/tunic-south.png')
atlas(dressed).save(OUT/'exports/dressed-south.png')
atlas([Image.alpha_composite(p,t) for p,t in zip(pants,shirt)]).save(OUT/'exports/outfit-south.png')
def review_font(size,bold=False):
    options=['C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',
             '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']
    for path in options:
        if Path(path).is_file():return ImageFont.truetype(path,size)
    return ImageFont.load_default()
font=review_font(16)
large=review_font(25,True)
def overview(i):
    im=Image.new('RGB',(1040,420),'#111a20');d=ImageDraw.Draw(im)
    d.text((28,16),'TUNIC FIT REVISION',font=large,fill='#e8ede3')
    d.text((28,51),'Same body and trousers. New shoulder fit, sleeves, neckline and hem.',font=font,fill='#a4b7b0')
    for x,label,f in [(28,'PREVIOUS',old[i]),(364,'REVISED',dressed[i]),(700,'NEW TUNIC LAYER',shirt[i])]:
        d.rounded_rectangle((x,87,x+312,406),12,fill='#1d2b30',outline='#344c4d')
        d.text((x+16,100),label,font=font,fill='#b8cbbf')
        z=f.resize((280,280),Image.Resampling.NEAREST);im.paste(z,(x+16,123),z)
    return im
overview(4).save(OUT/'previews/before-after.png')
walk=[overview(i) for i in range(4,12)]
walk[0].save(OUT/'previews/before-after-walk.gif',save_all=True,append_images=walk[1:],duration=[80,70]*4,loop=0,disposal=2,optimize=False)
sheet=Image.new('RGB',(1000,850),'#111a20');d=ImageDraw.Draw(sheet)
d.text((24,16),'REVISED TUNIC - ALL TWELVE POSES',font=large,fill='#e8ede3')
for i,f in enumerate(dressed):
    x=20+(i%4)*245;y=62+(i//4)*260
    d.text((x+18,y),('IDLE '+str(i+1)) if i<4 else ('WALK '+str(i-3)),font=font,fill='#b8cbbf')
    z=f.resize((240,240),Image.Resampling.NEAREST);sheet.paste(z,(x,y+18),z)
sheet.save(OUT/'previews/all-poses.png')
html=(BASE/'v1/character-workshop.html').read_text(encoding='utf-8')
match=re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S)
assert match
payload=json.loads(match[1])
for key,name in [('tunic-south','tunic-south.png'),('outfit-south','outfit-south.png'),('dressed-south','dressed-south.png')]:
    payload['images'][key]='data:image/png;base64,'+base64.b64encode((OUT/'exports'/name).read_bytes()).decode('ascii')
html=html[:match.start(1)]+json.dumps(payload,separators=(',',':'))+html[match.end(1):]
html=html.replace('Character pilot 01','Tunic fit revision 02').replace('<title>Embervale Character Workshop</title>','<title>Embervale Tunic Fit v2</title>')
html=html.replace('A reusable base, shared movement, clothes you can switch.','Revised shoulder fit, sleeves, neckline and hem on the same body motion.')
(OUT/'character-workshop.html').write_bytes(html.encode())
print(json.dumps({'revision':str(OUT),'frames':len(shirt),'shirt_bounds':[f.getbbox() for f in shirt]}))

