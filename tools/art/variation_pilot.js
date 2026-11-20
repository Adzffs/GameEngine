// Native pixel authoring. All painting and master-file writes happen in LibreSprite.
var cfg=__CONFIG__,pc=app.pixelColor;
function colour(h){return pc.rgba(parseInt(h.slice(0,2),16),parseInt(h.slice(2,4),16),parseInt(h.slice(4,6),16),255);}
function canvas(w,h){return {width:w,height:h,pixels:new Array(w*h).fill(0)};}
function put(o,x,y,c){if(x>=0&&y>=0&&x<o.width&&y<o.height)o.pixels[y*o.width+x]=c;}
function box(o,x,y,w,h,c){for(var yy=y;yy<y+h;yy++)for(var xx=x;xx<x+w;xx++)put(o,xx,yy,colour(c));}
function noise(x,y,s){var h=(x*73856093^y*19349663^s*83492791)>>>0;h^=h>>>16;h=Math.imul(h,0x7feb352d);h^=h>>>15;return h>>>0;}
function mix(a,b,t){return pc.rgba(Math.round(pc.rgbaR(a)*(1-t)+pc.rgbaR(b)*t),Math.round(pc.rgbaG(a)*(1-t)+pc.rgbaG(b)*t),Math.round(pc.rgbaB(a)*(1-t)+pc.rgbaB(b)*t),255);}
function read(path){var s=app.open(path).sprite,l=s.layer(0),c=l.cel(0),im=c.image,o=canvas(s.width,s.height);
 if(s.layerCount!==1||l.celCount!==1)throw new Error('Variation masters require one layer and one frame');
 for(var y=0;y<im.height;y++)for(var x=0;x<im.width;x++)put(o,c.x+x,c.y+y,im.getPixel(x,y));return o;}
function save(o,fixture,path){var s=app.open(fixture).sprite,im=s.layer(0).cel(0).image;
 if(s.width!==o.width||s.height!==o.height)throw new Error('Incorrect variation canvas');
 for(var y=0;y<o.height;y++)for(var x=0;x<o.width;x++)im.putPixel(x,y,o.pixels[y*o.width+x]);s.commit();s.saveAs(path,false);}
function blit(src,dst,dx,dy,sx,sy,w,h){sx=sx||0;sy=sy||0;w=w||src.width;h=h||src.height;
 for(var y=0;y<h;y++)for(var x=0;x<w;x++){var c=src.pixels[(sy+y)*src.width+sx+x];if(pc.rgbaA(c))put(dst,dx+x,dy+y,c);}}
function crop(src,x,y,w,h){var o=canvas(w,h);blit(src,o,0,0,x,y,w,h);return o;}
function scale(src,f){var o=canvas(src.width*f,src.height*f);for(var y=0;y<o.height;y++)for(var x=0;x<o.width;x++)o.pixels[y*o.width+x]=src.pixels[Math.floor(y/f)*src.width+Math.floor(x/f)];return o;}
function blade(o,x,y,c,light,height){box(o,x,y-height,1,height,c);put(o,x-1,y-2,colour(c));put(o,x+1,y-3,colour(light));put(o,x+2,y-4,colour(light));}
var greens=[['4b603a','4f643d','53683f','5b7045','718252'],['3d5636','405b39','46613b','516c40','657f49'],['5b653b','606a3f','687143','75804c','899257']];
function grass(pattern,v){var o=canvas(32,32),cs=[],n=pattern;
 for(var i=0;i<4;i++){cs.push(n%3);n=Math.floor(n/3);}
 for(var y=0;y<32;y++)for(var x=0;x<32;x++){
  var fx=x/31,fy=y/31,border=x<2||y<2||x>29||y>29;
  var tone=noise(Math.floor((x%31)/4),Math.floor((y%31)/4),border?0:v+1)%11;
  tone=tone<2?1:tone>9?3:2;
  var top=mix(colour(greens[cs[0]][tone]),colour(greens[cs[1]][tone]),fx);
  var bottom=mix(colour(greens[cs[2]][tone]),colour(greens[cs[3]][tone]),fx);
  put(o,x,y,mix(top,bottom,fy));
 }
 for(var i=0;i<5;i++){var x=4+noise(i,pattern,v+3)%23,y=8+noise(i,pattern,v+6)%20;
  var style=cs[(x>15?1:0)+(y>15?2:0)];blade(o,x,y,greens[style][1],greens[style][3],2+i%2);}
 return o;}
function distance(x,y,mask){var dx=x-15.5,dy=y-15.5,d=Math.sqrt(dx*dx+dy*dy);
 if((mask&1)&&dy<=0)d=Math.min(d,Math.abs(dx));if((mask&2)&&dx>=0)d=Math.min(d,Math.abs(dy));
 if((mask&4)&&dy>=0)d=Math.min(d,Math.abs(dx));if((mask&8)&&dx<=0)d=Math.min(d,Math.abs(dy));return d;}
function path(mask,condition,v){var o=canvas(32,32),p=['594b35','7b6040','98764c','a58656',condition===0?'b59664':condition===1?'af9569':'a28d5d'];
 for(var y=0;y<32;y++)for(var x=0;x<32;x++){
  var d=mask===16?0:distance(x,y,mask),edge=x<2||y<2||x>29||y>29;
  if(d>11.8-(edge?0:noise(x,y,6)%3*0.18))continue;
  var c=d>10.6?p[0]:d>9?p[1]:d>7.5?p[2]:p[3];
  if(d<8&&noise(Math.floor(x/3),Math.floor(y/2),condition*4+v+9)%29===0)c=p[4];
  if(condition===0&&d<8&&((mask===5&&(x===11||x===20))||(mask===10&&(y===11||y===20))))c=p[2];
  put(o,x,y,colour(c));
  if(!edge&&d<8&&noise(x,y,condition*4+v+18)%317===0){put(o,x,y,colour('c1af80'));put(o,x+1,y,colour('74634a'));}
  if(condition===2&&!edge&&d>9.8&&noise(x,y,8)%17===0)blade(o,x,y,'4b603a','75804c',2);
 }
 // Soft scuffs and one small infill patch, kept inside the dirt footprint.
 for(var k=0;k<(v===2?2:1);k++){var sx=7+noise(k,v,14)%15,sy=8+noise(k,v,15)%15;
  for(var xx=sx;xx<sx+5;xx++)if(mask===16||distance(xx,sy,mask)<7)put(o,xx,sy,colour(p[2]));}
 if(v===3)for(var y=12;y<19;y++)for(var x=11;x<21;x++)if(mask===16||distance(x,y,mask)<7){
  if((x===11||x===20||y===12||y===18)&&noise(x,y,2)%3===0)continue;
  put(o,x,y,colour(noise(x,y,3)%5===0?'8e7953':'9b855c'));}
 return o;}
function water(mask,v){var o=canvas(32,32),shallow=colour('426f73'),deep=colour('284b62');
 for(var y=0;y<32;y++)for(var x=0;x<32;x++){
  var fx=x/31,fy=y/31,t=(mask&1?1:0)*(1-fx)*(1-fy)+(mask&2?1:0)*fx*(1-fy)+(mask&4?1:0)*(1-fx)*fy+(mask&8?1:0)*fx*fy;
  put(o,x,y,mix(shallow,deep,t));
 }
 for(var i=0;i<3;i++){var x=4+noise(i,v,4)%17,y=5+noise(i,v,11)%21,w=3+noise(i,v,9)%6;
  box(o,x,y,w,1,mask===15?'365c70':'54827e');if(i===1)box(o,x+2,y+2,Math.max(2,w-2),1,mask===15?'31556b':'4b7779');}
 return o;}
function stone(condition,v){var o=canvas(32,32);
 for(var y=0;y<32;y++)for(var x=0;x<32;x++){
  var row=Math.floor(y/8),px=(x+row%2*8)%16,py=y%8;
  var c=py===0||px===0?'50594f':py===1||px===1?'929782':py===7||px===15?'646e60':'7b8471';
  if(condition===3)c=x<2||y<2||x>29||y>29?'747c68':noise(Math.floor(x/7),Math.floor(y/6),v)%11===0?'7b826d':'747c68';
  put(o,x,y,colour(c));
 }
 if(condition===1){for(var i=0;i<7;i++){var x=4+i*3,y=8+v*4+Math.floor(i/2);put(o,x,y,colour('4b5548'));if(i%2)put(o,x,y+1,colour('626f50'));}
  box(o,2+v*5,24,5,2,'607049');box(o,3+v*5,24,3,1,'728052');}
 if(condition===2){box(o,4+v*6,9,8,5,'a1a18b');box(o,4+v*6,14,8,1,'5e6658');box(o,5+v*6,9,6,1,'b4b39a');}
 if(condition===3)for(var i=0;i<5;i++){var x=3+noise(i,v,5)%24,y=4+noise(i,v,6)%23;box(o,x,y,3,2,'555f53');box(o,x,y,2,1,'969982');}
 return o;}
function leaf(o,x,y,tone,turn){var a=tone?'9b874c':'9b713e',b=tone?'b4a064':'bd8b4e',c=tone?'6f6740':'715537';
 if(turn){box(o,x,y,2,3,a);put(o,x,y,colour(b));put(o,x+1,y+3,colour(c));}
 else{box(o,x,y,3,2,a);put(o,x+1,y-1,colour(b));put(o,x+3,y+1,colour(c));}}
function flora(i){var o=canvas(32,32),v=i%4;
 if(i<8){var kind=Math.floor(i/2),flowers=[['dfd8ad','afab73','e4ba66'],['c9adcf','9379ad','e2c9d3'],['d7b760','a88a42','e7d28b'],['a9b9d3','718da8','d4d9d4']][kind];
  for(var k=0;k<4+i%2;k++){var x=5+noise(k,i,1)%22,y=13+noise(k,i,2)%16,h=4+noise(k,i,3)%4;
   blade(o,x,y,'3f5d35','78944e',h);box(o,x-1,y-h-1,3,2,flowers[0]);put(o,x,y-h-2,colour(flowers[2]));put(o,x,y-h,colour(flowers[1]));}
 }else if(i<16){var dry=i>=12;for(var k=0;k<4+v*2;k++){var x=4+noise(k,i,4)%24,y=14+noise(k,i,5)%15;
   blade(o,x,y,dry?'626d3d':'3e5932',dry?'8c9258':'7c9351',3+noise(k,i,6)%5);}
 }else if(i<20){for(var k=0;k<5;k++){var x=6+noise(k,i,2)%20,y=25+noise(k,i,5)%5,h=7+noise(k,i,8)%9;
   blade(o,x,y,'485e37','87935a',h);if(k%2)box(o,x,y-h-3,2,4,'927247');}
 }else if(i<28||i>=36){var sparse=i>=36,count=sparse?4:13,tone=i>=24&&i<28;
  for(var k=0;k<count;k++){var x=3+noise(k,i,13)%25,y=8+noise(k,i,14)%19;leaf(o,x,y,tone,k%3===0);}
 }else if(i<32){for(var k=0;k<4;k++){var x=4+noise(k,i,2)%23,y=14+noise(k,i,3)%14;box(o,x,y,3+k%2,2,'535e4d');box(o,x,y,2,1,'999f85');}
 }else{for(var k=0;k<5;k++){var x=4+noise(k,i,3)%22,y=14+noise(k,i,7)%14;box(o,x,y,4,2,'405c37');box(o,x+1,y,2,1,'71844c');}}
 return o;}
function tint(src,condition,material){var o=canvas(src.width,src.height);
 for(var y=0;y<o.height;y++)for(var x=0;x<o.width;x++){
  var c=src.pixels[y*src.width+x];if(!pc.rgbaA(c))continue;
  var r=pc.rgbaR(c),g=pc.rgbaG(c),b=pc.rgbaB(c);
  if(material){var l=(r*0.3+g*0.5+b*0.2);c=pc.rgba(Math.round(l*0.65),Math.round(l*0.79),Math.round(l*0.86),255);}
  if(condition===0)c=mix(c,colour(material?'8497a0':'d7b57c'),0.10);
  else c=mix(c,colour(material?'687b75':'8b8467'),0.16);
  put(o,x,y,c);
 }return o;}
function wallTile(part,condition,v,old){var o=tint(crop(old,part%8*32,Math.floor(part/8)*48,32,48),condition,0);
 // Age marks stay within the saved directional silhouette, including cutaways.
 function mark(x,y,c){if(x>=0&&y>=0&&x<32&&y<48&&pc.rgbaA(o.pixels[y*32+x]))put(o,x,y,colour(c));}
 function patch(x,y,w,h,c){for(var yy=y;yy<y+h;yy++)for(var xx=x;xx<x+w;xx++)mark(xx,yy,c);}
 var side=part===2||part===3||part===5||part===6;
 if(condition>0){
  var x=side?15:8+v*9,start=side?4:23;
  for(var k=0;k<5;k++){var xx=x+(k>2?1:0),yy=start+k,c=o.pixels[yy*32+xx];
   if(pc.rgbaA(c)&&pc.rgbaR(c)>pc.rgbaB(c)+20)mark(xx,yy,'8d896c');}
  var base=side?46:34;
  for(var x=1;x<31;x++)if(noise(x,part,v)%7===0)mark(x,base,'536347');
 }
 if(condition===2){
  if(part===0||part===1){patch(5+v*10,23,7,6,'d5c6a2');patch(6+v*10,23,5,1,'e4d5af');}
  if(side){patch(14,3+v*3,4,4,'a57740');patch(14,3+v*3,1,4,'d59b5a');}
  else if(part>=12){patch(v?22:2,25,7,5,'a57740');patch(v?22:2,25,7,1,'d59b5a');}
  else {patch(v?29:1,21,2,12,'855432');patch(v?29:1,21,1,12,'c49152');}
 }return o;}
function roofTile(part,condition,material,old){var o=tint(crop(old,part%3*32,Math.floor(part/3)*32,32,32),condition,material);
 if(condition===1&&part<9){
  // Moss follows damp eaves and north edges; never blankets every roof tile.
  if(part===0||part===1||part===3||part>=6)for(var k=0;k<7;k++){var x=4+noise(k,part,2)%23,y=part>=6?23+noise(k,part,3)%5:3+noise(k,part,4)%5;
   if(pc.rgbaA(o.pixels[y*32+x]))box(o,x,y,2,1,material?'5b7157':'697347');}
  if(part===4){box(o,12,15,3,1,material?'34434a':'674830');box(o,15,16,2,1,material?'52636a':'9a6a44');}
 }
 if(condition===2&&part<9){
  // Fresh tiles form one bounded patch; the renderer only selects it locally.
  for(var y=8;y<24;y++)for(var x=4;x<28;x++)if(pc.rgbaA(o.pixels[y*32+x])){
   var py=y%8,px=(x+Math.floor(y/8)%2*4)%8;
   put(o,x,y,colour(material?(py===0?'94a4ab':px===0?'435761':'6a818d'):(py===0?'d5995b':px===0?'81482e':'be7946')));}
 }return o;}
function treeTile(old,v,stump){var o=canvas(64,96);
 for(var y=0;y<96;y++)for(var x=0;x<64;x++){
  // Keep the root pivot fixed; only crown/upper trunk proportions change.
  var crown=stump?0:Math.max(0,Math.min(1,(77-y)/32));
  var width=1+(v===1?-0.10:v===2?0.08:0)*crown;
  var sx=Math.round(32+(x-32)/width),sy=stump?y:Math.round(88+(y-88)/(v===1?1.04:v===2?0.97:1));
  if(sx<0||sx>=64||sy<0||sy>=96)continue;
  var c=old.pixels[sy*64+sx];if(!pc.rgbaA(c))continue;
  if(v===1)c=mix(c,colour('57663c'),0.07);if(v===2)c=mix(c,colour('8b7b46'),0.10);put(o,x,y,c);
 }
 if(v===2&&!stump){box(o,29,75,2,6,'463026');put(o,30,75,colour('a07843'));}
 return o;}
function rockTile(old,v){var o=canvas(32,40);
 for(var y=0;y<40;y++)for(var x=0;x<32;x++){
  var sx=v===1?31-x:x,sy=y;if(v===2&&y<29)sy=Math.min(39,y+1+Math.floor((29-y)/12));
  var c=old.pixels[sy*32+sx];if(!pc.rgbaA(c))continue;
  if(v===2&&x>26&&y<19)continue;put(o,x,y,c);
 }return o;}
var sheets={};
if(cfg.regenerate||cfg.regenerateWalls){
 var old={};Object.keys(cfg.old).forEach(function(k){old[k]=read(cfg.old[k]);});
 Object.keys(cfg.sheets).forEach(function(name){var d=cfg.sheets[name],out=canvas(d[0],d[1]);
  if(!cfg.regenerate&&name!=='walls'){sheets[name]=read(cfg.sources[name]);return;}
  for(var i=0;i<d[4];i++){var tile;
   if(name==='grass')tile=grass(Math.floor(i/2),i%2);
   if(name==='paths')tile=path(i%17,Math.floor(i/68),Math.floor(i/17)%4);
   if(name==='water')tile=water(Math.floor(i/3),i%3);
   if(name==='stone')tile=stone(Math.floor(i/3),i%3);
   if(name==='flora')tile=flora(i);
   if(name==='walls')tile=wallTile(i%16,Math.floor(i/32),Math.floor(i/16)%2,old.walls);
   if(name==='roof')tile=roofTile(i%12,Math.floor(i/12)%3,Math.floor(i/36),old.roof);
   if(name==='floors'){tile=tint(crop(old.floors,i%3*32,0,32,32),Math.floor(i/3),0);
    if(i>=3){box(tile,5+i%3*5,12,9,1,'493d2e');if(i>=6){box(tile,4+i%3*5,17,13,4,'967247');box(tile,5+i%3*5,17,11,1,'b58e56');}}}
   if(name==='trees')tile=treeTile(old.tree,i,false);
   if(name==='stumps')tile=treeTile(old.stump,i,true);
   if(name==='rocks')tile=rockTile(i<3?old.copper:i<6?old.tin:old.rubble,i%3);
   blit(tile,out,i%d[5]*d[2],Math.floor(i/d[5])*d[3]);
  }save(out,cfg.fixtures[name],cfg.sources[name]);sheets[name]=out;
 });
}else Object.keys(cfg.sheets).forEach(function(name){sheets[name]=read(cfg.sources[name]);});
Object.keys(sheets).forEach(function(name){var o=sheets[name],d=cfg.sheets[name];
 if(o.width!==d[0]||o.height!==d[1])throw new Error('Variation master geometry changed: '+name);
 o.pixels.forEach(function(c){if(pc.rgbaA(c)!==0&&pc.rgbaA(c)!==255)throw new Error('Binary alpha required: '+name);});});
var board=canvas(640,384);
for(var y=0;y<384;y++)for(var x=0;x<640;x++)put(board,x,y,colour('272f2b'));
function show(name,i,x,y){var d=cfg.sheets[name];blit(sheets[name],board,x,y,i%d[5]*d[2],Math.floor(i/d[5])*d[3],d[2],d[3]);}
for(var i=0;i<12;i++){show('grass',[0,80,160,40][i%4],i*32,0);show('stone',i,i*32,40);}
for(var i=0;i<12;i++)show('water',i%3+Math.floor(i/4)*15,i*32,80);
for(var i=0;i<40;i++)show('flora',i,i%20*32,120+Math.floor(i/20)*40);
for(var i=0;i<3;i++){show('trees',i,8+i*72,220);show('stumps',i,240+i*64,220);}
for(var condition=0;condition<3;condition++){
 for(var part=0;part<3;part++)show('walls',condition*32+[4,7,2][part],400+part*32,208+condition*52);
 for(var part=0;part<3;part++)show('roof',condition*12+[6,7,8][part],512+part*32,208+condition*52);
}
for(var i=0;i<9;i++)show('rocks',i,8+i*40,336);
save(scale(board,2),cfg.fixtures.contact,cfg.preview);
console.log('VARIATION_PILOT_OK: 11 editable atlases; contextual grass, paths, water, stone, flora/leaves, building ages, trees and rocks.');
