"""Read-only whole-area coverage checks for the two independent garment choices."""
from pathlib import Path
from PIL import Image
import json,hashlib,base64,re
import numpy as np
OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]
def load(path):return Image.open(path).convert('RGBA')
def validate():
    body=load(BASE/'v1/source/layer-proof-raw.png').crop((0,0,1008,84))
    top=load(OUT/'source/tunic-raw.png')
    pants={v:load(OUT/f'source/trousers-{v}-raw.png') for v in ['knee','full']}
    domains=json.loads((OUT/'recipes/body-coverage-domains.json').read_bytes())
    previous_top=load(BASE/'revisions/tunic-fit-v2/source/tunic-raw.png')
    previous_pants=load(BASE/'revisions/trouser-fit-v3/source/trousers-raw.png')
    counts={'covered_shoulder_pixels':0,'covered_knee_trouser_pixels':0,'covered_full_trouser_pixels':0,
            'protected_hand_pixels':0,'protected_foot_pixels':0,'previous_uncovered_shoulder_pixels':0,
            'previous_uncovered_knee_trouser_pixels':0}
    results=[]
    for i,domain in enumerate(domains):
        assert domain['frame']==i+1
        for key in ['required_shoulders','required_trousers','required_knee_trousers','protected_hands','protected_feet','protected_face']:
            assert len(set(map(tuple,domain[key])))==len(domain[key]),('duplicate body label',i+1,key)
            assert all(body.getpixel((i*84+x,y))[3] for x,y in domain[key]),('invalid body label',i+1,key)
        row={'frame':i+1,'missing_shoulders':[],'missing_knee':[],'missing_full':[]}
        for p in domain['required_shoulders']:
            x,y=p
            if not top.getpixel((i*84+x,y))[3]:row['missing_shoulders'].append(p)
            counts['covered_shoulder_pixels']+=1
            if not previous_top.getpixel((i*84+x,y))[3]:counts['previous_uncovered_shoulder_pixels']+=1
        for v,key in [('knee','required_knee_trousers'),('full','required_trousers')]:
            for p in domain[key]:
                x,y=p
                if not pants[v].getpixel((i*84+x,y))[3]:row['missing_'+v].append(p)
                counts['covered_'+v+'_trouser_pixels']+=1
                if v=='knee' and not previous_pants.getpixel((i*84+x,y))[3]:counts['previous_uncovered_knee_trouser_pixels']+=1
            for key in ['protected_hands','protected_feet','protected_face']:
                for x,y in domain[key]:
                    assert not top.getpixel((i*84+x,y))[3] and not pants[v].getpixel((i*84+x,y))[3],('painted protected body part',i+1,v,key,x,y)
        for x,y in domain['protected_knee_lower_legs']:
            assert not pants['knee'].getpixel((i*84+x,y))[3],('knee pants over bare lower leg',i+1,x,y)
        assert not row['missing_shoulders'] and not row['missing_knee'] and not row['missing_full'],row
        counts['protected_hand_pixels']+=len(domain['protected_hands'])
        counts['protected_foot_pixels']+=len(domain['protected_feet'])
        tx,ny=domain['neck'];assert not top.getpixel((i*84+tx,ny+2))[3],('closed neck opening',i+1)
        results.append(row)
    for name,im in [('tunic',top),*[(f'trousers-{v}',im) for v,im in pants.items()]]:
        assert im.size==(1008,84) and set(np.unique(np.array(im)[:,:,3]))<={0,255}
        packed=load(OUT/f'exports/{name}-south.png')
        assert packed.size==(960,80)
        for i in range(12):
            source=im.crop((i*84,0,(i+1)*84,84));box=source.getbbox()
            assert box and min(box[:2])>=2 and max(box[2:])<=82
            assert source.crop((2,2,82,82)).tobytes()==packed.crop((i*80,0,(i+1)*80,80)).tobytes()
    b=load(BASE/'v1/layers/body-south.png');t=load(OUT/'exports/tunic-south.png')
    for v in ['knee','full']:
        p=load(OUT/f'exports/trousers-{v}-south.png');o=load(OUT/f'exports/outfit-{v}-south.png');d=load(OUT/f'exports/dressed-{v}-south.png')
        assert Image.alpha_composite(p,t).tobytes()==o.tobytes()
        assert Image.alpha_composite(b,o).tobytes()==d.tobytes()
    preserved={}
    for label,path in [('v1',BASE/'v1'),('tunic-fit-v2',OUT.parent/'tunic-fit-v2'),('trouser-fit-v3',OUT.parent/'trouser-fit-v3')]:
        inventory=json.loads((path/'sha256.json').read_bytes())
        for rel,digest in inventory.items():assert hashlib.sha256((path/rel).read_bytes()).hexdigest()==digest,(label,rel)
        preserved[label]=len(inventory)
    html=(OUT/'character-workshop.html').read_text(encoding='utf-8')
    data=json.loads(re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S)[1])
    assert 'id="bottom"' in html and 'value="knee"' in html and 'value="full"' in html
    assert "imgs['trousers-'+$('bottom').value+'-south']" in html
    for name in ['tunic-south','trousers-knee-south','trousers-full-south']:
        assert base64.b64decode(data['images'][name].split(',',1)[1])==(OUT/f'exports/{name}.png').read_bytes()
    prior_html=(OUT.parent/'trouser-fit-v3/character-workshop.html').read_text(encoding='utf-8')
    prior_data=json.loads(re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',prior_html,re.S)[1])
    assert data['images']['body-south']==prior_data['images']['body-south']
    gif=Image.open(OUT/'previews/before-after-walk.gif');duration=0
    assert gif.n_frames==8
    for i in range(8):gif.seek(i);duration+=gif.info['duration']
    assert duration==600
    assert counts['previous_uncovered_shoulder_pixels']>0 and counts['previous_uncovered_knee_trouser_pixels']>0
    return {'status':'passed','frames':12,'pants_variants':['knee','full'],**counts,
      'missing_covered_body_pixels':0,'protected_body_overlaps':0,'source_crop_lossless':True,
      'composition_exact':True,'unchanged_body':True,'preserved_hash_entries':preserved,
      'walk_preview_duration_ms':duration,'owner_visual_acceptance':'pending'}
if __name__=='__main__':print(json.dumps(validate()))
