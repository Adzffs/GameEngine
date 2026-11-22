"""Pack retained PixelLab layers and assemble review images. No artwork painting."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json,re,base64
OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]
def frames(path,cell=80):
    im=Image.open(path).convert('RGBA');assert im.size==(12*cell,cell)
    return [im.crop((i*cell,0,(i+1)*cell,cell)) for i in range(12)]
def source(name):
    result=[]
    for f in frames(OUT/'source'/name,84):
        b=f.getbbox();assert b and min(b[:2])>=2 and max(b[2:])<=82
        result.append(f.crop((2,2,82,82)))
    return result
def atlas(fs,path):
    im=Image.new('RGBA',(960,80))
    for i,f in enumerate(fs):im.paste(f,(80*i,0))
    im.save(path)
body=frames(BASE/'v1/layers/body-south.png')
old=frames(BASE/'revisions/trouser-fit-v3/exports/dressed-south.png')
top=source('tunic-raw.png');atlas(top,OUT/'exports/tunic-south.png')
pants={};dressed={}
for variant in ['knee','full']:
    pants[variant]=source(f'trousers-{variant}-raw.png')
    outfit=[Image.alpha_composite(p,t) for p,t in zip(pants[variant],top)]
    dressed[variant]=[Image.alpha_composite(b,o) for b,o in zip(body,outfit)]
    for name,fs in [('trousers',pants[variant]),('outfit',outfit),('dressed',dressed[variant])]:
        atlas(fs,OUT/f'exports/{name}-{variant}-south.png')
def font(size,bold=False):
    for path in ['C:/Windows/Fonts/segoeuib.ttf' if bold else 'C:/Windows/Fonts/segoeui.ttf',
                 '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf' if bold else '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']:
        if Path(path).is_file():return ImageFont.truetype(path,size)
    return ImageFont.load_default()
small=font(16);large=font(25,True)
def comparison(i):
    im=Image.new('RGB',(1140,420),'#111a20');d=ImageDraw.Draw(im)
    d.text((26,16),'SHOULDER AND LEG COVERAGE',font=large,fill='#e8ede3')
    d.text((26,51),'Previous outfit, corrected knee-length pants, and full-length alternative.',font=small,fill='#a4b7b0')
    for x,label,f in [(26,'PREVIOUS',old[i]),(402,'REVISED KNEE LENGTH',dressed['knee'][i]),(778,'REVISED FULL LENGTH',dressed['full'][i])]:
        d.rounded_rectangle((x,87,x+336,406),12,fill='#1d2b30',outline='#344c4d')
        d.text((x+16,100),label,font=small,fill='#b8cbbf')
        z=f.resize((280,280),Image.Resampling.NEAREST);im.paste(z,(x+28,123),z)
    return im
comparison(4).save(OUT/'previews/before-after.png')
walk=[comparison(i) for i in range(4,12)]
walk[0].save(OUT/'previews/before-after-walk.gif',save_all=True,append_images=walk[1:],duration=[80,70]*4,loop=0,disposal=2,optimize=False)
for variant in ['knee','full']:
    sheet=Image.new('RGB',(1000,850),'#111a20');d=ImageDraw.Draw(sheet)
    d.text((24,16),f'{variant.upper()} LENGTH - ALL TWELVE POSES',font=large,fill='#e8ede3')
    for i,f in enumerate(dressed[variant]):
        x=20+(i%4)*245;y=62+(i//4)*260
        d.text((x+18,y),('IDLE '+str(i+1)) if i<4 else ('WALK '+str(i-3)),font=small,fill='#b8cbbf')
        z=f.resize((240,240),Image.Resampling.NEAREST);sheet.paste(z,(x,y+18),z)
    sheet.save(OUT/f'previews/all-poses-{variant}.png')
# Enlarged evidence exposes the exact edges missed by the earlier full-body review.
for name,box,scale in [('shoulders',(22,21,60,38),6),('legs',(26,42,54,72),5)]:
    bw=(box[2]-box[0])*scale;bh=(box[3]-box[1])*scale
    variants=['previous','knee'] if name=='shoulders' else ['previous','knee','full']
    cw=(bw+12)*len(variants);ch=bh+56
    im=Image.new('RGB',(cw*3,ch*4),'#111a20');d=ImageDraw.Draw(im)
    for i in range(12):
        x=(i%3)*cw;y=(i//3)*ch
        d.text((x+8,y+3),f'POSE {i+1}',font=small,fill='#e8ede3')
        for j,v in enumerate(variants):
            f=old[i] if v=='previous' else dressed[v][i]
            z=f.crop(box).resize((bw,bh),Image.Resampling.NEAREST)
            d.text((x+j*(bw+12)+8,y+25),v.upper(),font=small,fill='#a4b7b0')
            im.paste(z,(x+j*(bw+12),y+48),z)
    im.save(OUT/f'previews/{name}-closeups.png')
html=(BASE/'revisions/trouser-fit-v3/character-workshop.html').read_text(encoding='utf-8')
m=re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S);assert m
data=json.loads(m[1])
for key,path in [('tunic-south','tunic-south'),('trousers-south','trousers-full-south'),
                 ('outfit-south','outfit-full-south'),('dressed-south','dressed-full-south'),
                 ('trousers-full-south','trousers-full-south'),('trousers-knee-south','trousers-knee-south')]:
    data['images'][key]='data:image/png;base64,'+base64.b64encode((OUT/f'exports/{path}.png').read_bytes()).decode()
html=html[:m.start(1)]+json.dumps(data,separators=(',',':'))+html[m.end(1):]
html=html.replace('Embervale Trouser Fit v3','Embervale Garment Coverage v4').replace('Trouser fit revision 03','Garment coverage revision 04')
html=html.replace('Revised trouser fit, separate leg panels and cuffs; same body motion and v2 tunic.','Covered shoulders and thighs, with knee-length or full-length pants on the same body motion.')
control='<div class="field"><label for="bottom">Pants length</label><select id="bottom"><option value="full" selected>Full length</option><option value="knee">Knee length</option></select></div>\n'
html=html.replace('  <div class="field"><span class="label">Facing</span>',control+'  <div class="field"><span class="label">Facing</span>')
html=html.replace("piece(imgs['trousers-south'],col,null)","piece(imgs['trousers-'+$('bottom').value+'-south'],col,null)")
html=html.replace("for(const name of ['display','skin','anchor'])","for(const name of ['display','skin','anchor','bottom'])")
html=html.replace("$('skin').disabled=ok&&$('display').value==='outfit';","$('skin').disabled=ok&&$('display').value==='outfit'; $('bottom').disabled=!ok||$('display').value==='body';")
assert 'id="bottom"' in html and "$('bottom').value" in html
(OUT/'character-workshop.html').write_bytes(html.encode())
print(json.dumps({'revision':'coverage-fit-v4','frames_per_variant':12,'variants':['knee','full'],'unchanged_body':True}))
