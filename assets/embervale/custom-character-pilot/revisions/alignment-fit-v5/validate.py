"""Read-only export/master parity and complete pose-registration checks."""
from pathlib import Path
from PIL import Image
import numpy as np,json,hashlib,base64,re
OUT=Path(__file__).resolve().parent
BASE=OUT.parents[1]
def rgba(path):return np.array(Image.open(path).convert('RGBA'))
def components(mask):
    seen=set();sizes=[]
    for y,x in zip(*np.where(mask)):
        start=(int(x),int(y))
        if start in seen:continue
        todo=[start];seen.add(start);size=0
        while todo:
            x,y=todo.pop();size+=1
            for nx,ny in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
                if 0<=nx<84 and 0<=ny<84 and mask[ny,nx] and (nx,ny) not in seen:seen.add((nx,ny));todo.append((nx,ny))
        sizes.append(size)
    return sorted(sizes,reverse=True)
def master_frame(recipe,index):
    view=recipe['scene']['views']['south'];pose=view['frames'][index]
    cmds=view['poses'][pose]['garment']['draw'];assert len(cmds)==1
    grid=cmds[0];assert grid['op']=='grid' and grid['at']==[0,0] and len(grid['rows'])==84
    image=np.zeros((84,84,4),np.uint8)
    for y,row in enumerate(grid['rows']):
        assert len(row)==84
        for x,c in enumerate(row):
            if c=='.':continue
            value=grid['legend'][c].lstrip('#');assert len(value)==6
            image[y,x]=[int(value[j:j+2],16) for j in [0,2,4]]+[255]
    return image
def validate():
    profiles=json.loads((OUT/'recipes/pose-registration.json').read_bytes())
    body=rgba(BASE/'v1/source/layer-proof-raw.png')[:84]
    old4=OUT.parent/'coverage-fit-v4'
    recipes={k:json.loads((OUT/f'recipes/{k}.json').read_bytes()) for k in ['tunic','knee','full']}
    raw={k:rgba(OUT/f'source/{k}-raw.png') for k in recipes}
    previous={k:rgba(old4/('source/tunic-raw.png' if k=='tunic' else f'source/trousers-{k}-raw.png')) for k in recipes}
    for image in raw.values():assert image.shape==(84,1008,4) and set(np.unique(image[:,:,3]))<={0,255}
    results=[];pixel_comparisons=0;old_unsupported=0;old_forearm_overlaps=0
    max_cuff_width=0;max_cuff_centre=0;old_bad_cuffs=0;coverage=0
    for i,p in enumerate(profiles):
        assert p['frame']==i+1
        b=body[:,i*84:(i+1)*84];ba=b[:,:,3]>0
        allowed=ba.copy();allowed[:,1:]|=ba[:,:-1];allowed[:,:-1]|=ba[:,1:]
        row={'frame':i+1,'garments':{},'cuffs':[]}
        for k in recipes:
            image=raw[k][:,i*84:(i+1)*84];mask=image[:,:,3]>0
            expected=master_frame(recipes[k],i)
            assert np.array_equal(image,expected),('source/master pixel mismatch',i+1,k)
            pixel_comparisons+=84*84
            required=p['required_shoulders'] if k=='tunic' else p['pants'][k]['required']
            assert all(ba[y,x] and mask[y,x] for x,y in required),('uncovered source body',i+1,k)
            coverage+=len(required)
            protected=p['source_hands']+p['source_feet']+p['face']+(p['pants'][k]['protected_lower'] if k!='tunic' else [])
            assert all(not mask[y,x] for x,y in protected),('cloth on protected body',i+1,k)
            assert not np.any(mask&~allowed),('garment detached from pose contour',i+1,k)
            clusters=components(mask);assert len(clusters)==1,('detached garment scraps',i+1,k,clusters)
            oldmask=previous[k][:,i*84:(i+1)*84,3]>0
            old_unsupported+=int(np.count_nonzero(oldmask&~allowed))
            old_forearm_overlaps+=sum(bool(oldmask[y,x]) for x,y in p['reclassified_forearm_pixels'])
            row['garments'][k]={'pixels':int(mask.sum()),'coverage_pixels':len(required),'unsupported_pixels':0,'protected_overlaps':0,'connected_parts':1}
            if k=='tunic':continue
            sides={}
            for r in p['pants'][k]['source_rows']:sides.setdefault(r['y'],{})[r['side']]=r['source_span']
            for y,s in sides.items():
                if 0 in s and 1 in s:
                    assert not np.any(mask[y,s[0][1]+1:s[1][0]]),('filled source leg separation',i+1,k,y)
            for cuff in p['pants'][k]['cuffs']:
                side,y,span=cuff['side'],cuff['y'],cuff['source_span'];lo,hi=span
                source_x=[x for x in range(29 if side==0 else 42,42 if side==0 else 55) if ba[y,x] and [x,y] in p['pants'][k]['required']]
                assert source_x and [min(source_x),max(source_x)]==span,('invalid original cuff reference',i+1,k,side)
                xs=[x for x in range(28 if side==0 else 42,42 if side==0 else 56) if mask[y,x]]
                assert xs and min(xs)<=lo<=hi<=max(xs)
                width=(max(xs)-min(xs))-(hi-lo);centre=abs((min(xs)+max(xs)-lo-hi)/2)
                assert width<=1 and centre<=0.5,('cuff floats away from ankle/knee',i+1,k,side,width,centre)
                max_cuff_width=max(max_cuff_width,width);max_cuff_centre=max(max_cuff_centre,centre)
                ox=[x for x in range(28 if side==0 else 42,42 if side==0 else 56) if oldmask[y,x]]
                if ox and ((max(ox)-min(ox))-(hi-lo)>1 or abs((min(ox)+max(ox)-lo-hi)/2)>0.5):old_bad_cuffs+=1
                row['cuffs'].append({'variant':k,'side':side,'y':y,'source_span':span,'new_span':[min(xs),max(xs)],'centre_error_px':centre})
        results.append(row)
    b=Image.open(BASE/'v1/layers/body-south.png').convert('RGBA');t=Image.open(OUT/'exports/tunic-south.png').convert('RGBA')
    for k in recipes:
        name='tunic-south' if k=='tunic' else f'trousers-{k}-south'
        packed=rgba(OUT/f'exports/{name}.png');assert packed.shape==(80,960,4)
        for i in range(12):
            source=raw[k][:,i*84:(i+1)*84];bounds=Image.fromarray(source).getbbox()
            assert bounds and min(bounds[:2])>=2 and max(bounds[2:])<=82
            assert np.array_equal(source[2:82,2:82],packed[:,i*80:(i+1)*80])
    for v in ['knee','full']:
        p=Image.open(OUT/f'exports/trousers-{v}-south.png').convert('RGBA');o=Image.open(OUT/f'exports/outfit-{v}-south.png').convert('RGBA');d=Image.open(OUT/f'exports/dressed-{v}-south.png').convert('RGBA')
        assert Image.alpha_composite(p,t).tobytes()==o.tobytes() and Image.alpha_composite(b,o).tobytes()==d.tobytes()
    preserved={}
    for name,path in [('v1',BASE/'v1'),('tunic-fit-v2',OUT.parent/'tunic-fit-v2'),('trouser-fit-v3',OUT.parent/'trouser-fit-v3'),('coverage-fit-v4',old4)]:
        inventory=json.loads((path/'sha256.json').read_bytes())
        for rel,digest in inventory.items():assert hashlib.sha256((path/rel).read_bytes()).hexdigest()==digest,(name,rel)
        preserved[name]=len(inventory)
    html=(OUT/'character-workshop.html').read_text(encoding='utf-8');data=json.loads(re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',html,re.S)[1])
    for name in ['tunic-south','trousers-knee-south','trousers-full-south']:
        assert base64.b64decode(data['images'][name].split(',',1)[1])==(OUT/f'exports/{name}.png').read_bytes()
    previous_html=(old4/'character-workshop.html').read_text(encoding='utf-8');old_data=json.loads(re.search(r'const DATA = (.*?);\s*const DIRECTIONS=',previous_html,re.S)[1])
    assert data['images']['body-south']==old_data['images']['body-south']
    assert data['walkMs']==old_data['walkMs'] and data['idleMs']==old_data['idleMs']
    gif=Image.open(OUT/'previews/before-after-walk.gif');assert gif.n_frames==8
    duration=0
    for i in range(8):gif.seek(i);duration+=gif.info['duration']
    assert duration==600
    assert old_unsupported>0 and old_forearm_overlaps>0 and old_bad_cuffs>0
    return {'status':'passed','body_poses':12,'garment_cells':36,'source_pixels_compared_to_editable_master':pixel_comparisons,
      'covered_source_body_checks':coverage,'unsupported_garment_pixels':0,'protected_body_overlaps':0,
      'disconnected_garment_components':0,'maximum_cuff_extra_width_px':max_cuff_width,'maximum_cuff_centre_error_px':max_cuff_centre,
      'v4_unsupported_pixels':old_unsupported,'v4_forearm_overlap_checks':old_forearm_overlaps,'v4_bad_cuffs':old_bad_cuffs,
      'forearm_pixels_reclassified':sum(len(p['reclassified_forearm_pixels']) for p in profiles),
      'source_crop_lossless':True,'composition_exact':True,'original_body_unchanged':True,
      'preserved_hash_entries':preserved,'walk_preview_duration_ms':duration,'per_frame':results,'owner_visual_acceptance':'pending'}
if __name__=='__main__':
    report=validate();print(json.dumps({k:v for k,v in report.items() if k!='per_frame'}))
