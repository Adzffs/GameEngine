"""Read-only fit regressions against independent body landmarks and v1 inputs."""
from pathlib import Path
from PIL import Image
import hashlib,json
import numpy as np

OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]

def validate():
    raw=np.array(Image.open(OUT/'source/tunic-raw.png').convert('RGBA'))
    old=np.array(Image.open(BASE/'v1/layers/tunic-south.png').convert('RGBA'))
    body=np.array(Image.open(BASE/'v1/source/layer-proof-raw.png').convert('RGBA'))[:84]
    assert raw.shape==(84,1008,4)
    assert set(np.unique(raw[:,:,3]))<={0,255}
    samples=json.loads((OUT/'recipes/acceptance-samples.json').read_text(encoding='utf-8'))
    results=[]
    for row in samples:
        i,cx,ny,left,right,wy,hand_l,hand_r=row
        garment=raw[:,(i-1)*84:i*84]
        b=body[:,(i-1)*84:i*84]
        previous=old[:,(i-1)*80:i*80]
        required=[[left,ny],[right,ny],[cx,ny+7],[cx,wy+4]]
        assert all(b[y,x,3]>0 for x,y in required),f'Invalid body sample {i}'
        missing=[p for p in required if garment[p[1],p[0],3]==0]
        rejected_old=[p for p in required if previous[p[1]-2,p[0]-2,3]==0]
        overlaps=[]
        for interval in [hand_l,hand_r]:
            for y in range(44,56):
                for x in range(interval[0],interval[1]+1):
                    if b[y,x,3]>0 and garment[y,x,3]>0:overlaps.append([x,y])
        assert not missing,(i,'uncovered shoulder/chest/hip',missing)
        assert not overlaps,(i,'garment over hand/forearm',overlaps)
        # Raised shoulders may extend above the neck line, outside the actual face.
        assert not np.any(garment[:ny,cx-5:cx+6,3]),(i,'protected face overlap')
        assert not np.any(garment[:ny-1,:,3]),(i,'garment above declared shoulder height')
        assert garment[ny+2,cx,3]==0,(i,'closed neck opening')
        results.append({'frame':i,'old_missing':rejected_old,'new_missing':missing,'hand_overlaps':0})
    assert sum(bool(r['old_missing']) for r in results)==12
    shoulder_samples=json.loads((OUT/'recipes/shoulder-crest-samples.json').read_text())
    for entry in shoulder_samples:
        index=entry['frame']-1
        for x,y in entry['points']:
            assert body[y,index*84+x,3]>0,('invalid source shoulder sample',entry['frame'],x,y)
            assert raw[y,index*84+x,3]>0,('exposed shoulder strip',entry['frame'],x,y)
    b=Image.open(BASE/'v1/layers/body-south.png').convert('RGBA')
    p=Image.open(BASE/'v1/layers/trousers-south.png').convert('RGBA')
    t=Image.open(OUT/'exports/tunic-south.png').convert('RGBA')
    o=Image.open(OUT/'exports/outfit-south.png').convert('RGBA')
    d=Image.open(OUT/'exports/dressed-south.png').convert('RGBA')
    assert Image.alpha_composite(p,t).tobytes()==o.tobytes()
    assert Image.alpha_composite(b,o).tobytes()==d.tobytes()
    for i in range(12):
        source=Image.fromarray(raw[:,i*84:(i+1)*84])
        bounds=source.getbbox()
        assert bounds and bounds[0]>=2 and bounds[1]>=2 and bounds[2]<=82 and bounds[3]<=82
        assert source.crop((2,2,82,82)).tobytes()==t.crop((i*80,0,(i+1)*80,80)).tobytes()
    inventory=json.loads((BASE/'v1/sha256.json').read_text(encoding='utf-8'))
    for name,digest in inventory.items():
        assert hashlib.sha256((BASE/'v1'/name).read_bytes()).hexdigest()==digest,name
    return {'status':'passed','frames':12,'v1_rejected_frames':12,'missing_coverage':0,
            'hand_overlaps':0,'face_overlap_pixels':0,'composition_exact':True,
            'source_crop_lossless':True,'v1_inventory_preserved':True,
            'additional_raised_shoulder_samples':sum(len(x['points']) for x in shoulder_samples)}

if __name__=='__main__':print(json.dumps(validate()))
