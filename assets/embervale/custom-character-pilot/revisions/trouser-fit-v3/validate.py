"""Read-only checks against retained body samples, not the drawing recipe."""
from pathlib import Path
from PIL import Image
import hashlib, json, re, base64
import numpy as np
OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]
V2=OUT.parent/'tunic-fit-v2'
def validate():
    raw=Image.open(OUT/'source/trousers-raw.png').convert('RGBA')
    assert raw.size==(1008,84)
    a=np.array(raw);assert set(np.unique(a[:,:,3]))<={0,255}
    body=Image.open(BASE/'v1/source/layer-proof-raw.png').convert('RGBA').crop((0,0,1008,84))
    samples=json.loads((OUT/'recipes/acceptance-samples.json').read_bytes())
    hand_rows=json.loads((V2/'recipes/acceptance-samples.json').read_bytes())
    counts={'body_coverage_samples':0,'clear_gap_samples':0,'protected_hand_pixels':0,'protected_lower_leg_pixels':0}
    for i,entry in enumerate(samples):
        g=np.array(raw.crop((i*84,0,(i+1)*84,84)))
        b=np.array(body.crop((i*84,0,(i+1)*84,84)))
        for x,y in entry['required_body_coverage']:
            assert b[y,x,3]>0,('invalid source sample',i+1,x,y)
            assert g[y,x,3]>0,('exposed waist/upper thigh',i+1,x,y)
            counts['body_coverage_samples']+=1
        for x,y in entry['clear_between_legs']:
            assert b[y,x,3]==0,('not an actual source leg gap',i+1,x,y)
            assert g[y,x,3]==0,('joined leg silhouettes',i+1,x,y)
            counts['clear_gap_samples']+=1
        for lo,hi in hand_rows[i][-2:]:
            for y in range(44,56):
                for x in range(lo,hi+1):
                    if b[y,x,3]:
                        counts['protected_hand_pixels']+=1
                        assert not g[y,x,3],('covered hand/forearm',i+1,x,y)
        # Below both knees, all original lower-leg/foot pixels must remain visible.
        assert not np.any(g[:38,:,3]),('trousers over upper body',i+1)
        assert not np.any(g[63:,:,3]),('trousers over protected calves/feet',i+1)
        counts['protected_lower_leg_pixels']+=int(np.count_nonzero(b[63:,:,3]))
        bounds=Image.fromarray(g).getbbox();assert bounds and min(bounds[:2])>=2 and max(bounds[2:])<=82
    p=Image.open(OUT/'exports/trousers-south.png').convert('RGBA')
    t=Image.open(V2/'exports/tunic-south.png').convert('RGBA')
    b=Image.open(BASE/'v1/layers/body-south.png').convert('RGBA')
    o=Image.open(OUT/'exports/outfit-south.png').convert('RGBA')
    d=Image.open(OUT/'exports/dressed-south.png').convert('RGBA')
    for im in [p,t,b,o,d]:assert im.size==(960,80)
    assert Image.alpha_composite(p,t).tobytes()==o.tobytes(),'outfit composition'
    assert Image.alpha_composite(b,o).tobytes()==d.tobytes(),'dressed composition'
    for i in range(12):
        assert raw.crop((i*84+2,2,i*84+82,82)).tobytes()==p.crop((i*80,0,(i+1)*80,80)).tobytes()
    preserved={}
    for name,path in [('v1',BASE/'v1'),('tunic-fit-v2',V2)]:
        inv=json.loads((path/'sha256.json').read_bytes())
        for rel,digest in inv.items():assert hashlib.sha256((path/rel).read_bytes()).hexdigest()==digest,(name,rel)
        preserved[name]=len(inv)
    # The self-contained workshop must use the real new layer and unchanged body/tunic.
    html=(OUT/'character-workshop.html').read_text(encoding='utf-8')
    m=re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S);assert m
    data=json.loads(m[1])
    original=(V2/'character-workshop.html').read_text(encoding='utf-8')
    original_data=json.loads(re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',original,re.S)[1])
    for key in ['body-south','tunic-south','body-standing-eight-way']:
        if key in original_data['images']:assert data['images'][key]==original_data['images'][key],key
    assert base64.b64decode(data['images']['trousers-south'].split(',',1)[1])==(OUT/'exports/trousers-south.png').read_bytes()
    return {'status':'passed','frames':12,**counts,'hand_overlaps':0,'lower_leg_overlaps':0,
            'source_crop_lossless':True,'composition_exact':True,'body_and_v2_tunic_unchanged':True,
            'preserved_hash_entries':preserved,'preview_layer_matches_export':True}
if __name__=='__main__':print(json.dumps(validate()))
