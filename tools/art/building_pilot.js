// Exact overhead tiles; LibreSprite authors the pixels and editable masters.
var cfg=__CONFIG__, pc=app.pixelColor;
function colour(h) { return pc.rgba(parseInt(h.slice(0,2),16),parseInt(h.slice(2,4),16),parseInt(h.slice(4,6),16),255); }
function canvas(w,h) { return {width:w,height:h,pixels:new Array(w*h).fill(0)}; }
function put(o,x,y,c) { if(x>=0&&y>=0&&x<o.width&&y<o.height)o.pixels[y*o.width+x]=c; }
function box(o,x,y,w,h,c) { for(var yy=y;yy<y+h;yy++)for(var xx=x;xx<x+w;xx++)put(o,xx,yy,colour(c)); }
function read(path) {
 var s=app.open(path).sprite,l=s.layer(0),c=l.cel(0),im=c.image,o=canvas(s.width,s.height);
 if(s.layerCount!==1||l.celCount!==1)throw new Error('Building masters require one layer and one frame');
 for(var y=0;y<im.height;y++)for(var x=0;x<im.width;x++)put(o,c.x+x,c.y+y,im.getPixel(x,y));return o;
}
function save(o,fixture,path) {
 var s=app.open(fixture).sprite,im=s.layer(0).cel(0).image;
 if(s.width!==o.width||s.height!==o.height)throw new Error('Incorrect building canvas');
 for(var y=0;y<o.height;y++)for(var x=0;x<o.width;x++)im.putPixel(x,y,o.pixels[y*o.width+x]);
 s.commit();s.saveAs(path,false);
}
function blit(src,dst,dx,dy,sx,sy,w,h,alpha) {
 sx=sx||0;sy=sy||0;w=w||src.width;h=h||src.height;alpha=alpha===undefined?255:alpha;
 for(var y=0;y<h;y++)for(var x=0;x<w;x++) {
  var c=src.pixels[(sy+y)*src.width+sx+x];if(!pc.rgbaA(c))continue;
  if(alpha<255) {var prev=dst.pixels[(dy+y)*dst.width+dx+x],a=alpha/255;
   c=pc.rgba(Math.round(pc.rgbaR(c)*a+pc.rgbaR(prev)*(1-a)),Math.round(pc.rgbaG(c)*a+pc.rgbaG(prev)*(1-a)),Math.round(pc.rgbaB(c)*a+pc.rgbaB(prev)*(1-a)),255);}
  put(dst,dx+x,dy+y,c);
 }
}
function scale(src,f) {var o=canvas(src.width*f,src.height*f);for(var y=0;y<o.height;y++)for(var x=0;x<o.width;x++)o.pixels[y*o.width+x]=src.pixels[Math.floor(y/f)*src.width+Math.floor(x/f)];return o;}
function wall(mode) {
 var o=canvas(32,48),low=mode>=12;
 if(mode===7||mode===13) {
  var top=low?24:12;
  // Open south entrance: short jambs in cutaway, overhead lintel outside.
  for(var j=0;j<2;j++) {
   var x=j?26:0;box(o,x,top,6,36-top,'603e2b');box(o,x+1,top,1,36-top,'b97c43');
   box(o,x,top,6,2,'d59b5a');box(o,x,34,6,2,'677165');
  }
  if(!low){box(o,0,12,32,8,'7b4e2e');box(o,0,12,32,1,'d59b5a');box(o,0,19,32,3,'463026');}
  box(o,6,36,20,2,'858d7a');box(o,6,37,20,1,'4b554d');return o;
 }
 // Ground-plan arms: N1/E2/S4/W8. Caps follow the actual wall axis.
 var masks=[10,10,5,5,10,5,5,10,6,12,3,9,10,10,3,9],mask=masks[mode];
 function footprint(x,y) {
  if(x<0)return !!(mask&8)&&y>=12&&y<20;
  if(x>=32)return !!(mask&2)&&y>=12&&y<20;
  if(y<0)return !!(mask&1)&&x>=12&&x<20;
  if(y>=32)return !!(mask&4)&&x>=12&&x<20;
  return (x>=12&&x<20&&y>=12&&y<20)||
   ((mask&1)&&x>=12&&x<20&&y<12)||((mask&4)&&x>=12&&x<20&&y>=20)||
   ((mask&8)&&y>=12&&y<20&&x<12)||((mask&2)&&y>=12&&y<20&&x>=20);
 }
 for(var x=0;x<32;x++) {
  var last=-1;
  for(var y=0;y<32;y++)if(footprint(x,y))last=y;
  if(last<0)continue;
  // Retain the tall side return where the low front joins a corner post.
  var height=low&&!(mode>=14&&x>=12&&x<20)?4:16,shift=16-height;
  var panel=mode===0||mode===1||mode===4||((mode>=8&&mode<=11)&&(x<12||x>=20));
  for(var y=last+1+shift;y<=last+16;y++) {
   var fromTop=y-last-shift;
   var c=panel?(mode===1?'bdb094':'c5b795'):'603e2b';
   if(fromTop===1)c='463026';else if(fromTop===2)c='d5c7a4';
   if(y>=last+14)c=x%8===0?'40483f':y===last+14?'858d7a':'677165';
   else if(x%32<3||x%32>28||!panel)c=x%2?'7b4e2e':'603e2b';
   put(o,x,y,colour(c));
  }
  for(var y=0;y<32;y++)if(footprint(x,y)) {
   var c='7b4e2e';
   if(!footprint(x,y-1))c='d59b5a';else if(!footprint(x-1,y))c='b97c43';
   else if(!footprint(x+1,y)||!footprint(x,y+1))c='463026';
   else if((mask===5&&x===15)||(mask===10&&y===15))c='985f35';
   put(o,x,y+shift,colour(c));
  }
 }
 if(mode===0)for(var i=0;i<12;i++)box(o,6+i,31-Math.floor(i/2),2,1,'7b4e2e');
 if(mode===4) {
  box(o,8,22,16,10,'603e2b');box(o,10,23,12,7,'334d56');box(o,11,24,10,5,'55767b');
  box(o,11,24,2,2,'8da39c');box(o,15,23,1,7,'463026');box(o,10,26,12,1,'463026');box(o,8,31,16,1,'b97c43');
 }
 if(mode===5||mode===6) {
  // Side windows are narrow vertical recesses along the wall, not front panels.
  box(o,12,9,8,16,'463026');box(o,13,10,6,14,'334d56');box(o,14,11,4,12,'55767b');
  box(o,14,11,1,11,'8da39c');box(o,13,16,6,1,'463026');box(o,12,9,8,1,'b97c43');
 }
 return o;
}
function roofTile(i) {
 var o=canvas(32,32),rx=i%3,ry=Math.floor(i/3);
 if(i>=9) {
  box(o,0,8,32,9,'483026');box(o,0,9,32,5,'a75f3c');box(o,0,9,32,1,'d09258');
  for(var x=0;x<32;x+=8) {box(o,x,8,2,8,'70412d');box(o,x+2,9,1,6,'ba7748');}
  if(i===9)box(o,0,7,3,11,'603e2b');if(i===11)box(o,29,7,3,11,'603e2b');return o;
 }
 for(var y=0;y<32;y++)for(var x=0;x<32;x++) {
  var py=y%8,px=(x+Math.floor(y/8)%2*4)%8;
  var c=py===7?'70412d':px===0?'8c5036':px===7?'7a452e':py===0?'ba7748':py===1?'b16d43':py>5?'8c5036':'a75f3c';
  if(py===6&&(px<2||px>5))c='603c2b';
  if(py===7&&px>=2&&px<=5)c='985638';
  if(py===1&&px>=2&&px<=4)c='c28652';
  if((rx===0&&x<2)||(rx===2&&x>29)||(ry===0&&y<2)||(ry===2&&y>27))c='483026';
  if((rx===0&&x===2)||(ry===0&&y===2))c='ba7748';
  var clipped=(rx===0&&ry===0&&x+y<9)||(rx===2&&ry===0&&31-x+y<9)||
      (rx===0&&ry===2&&x+31-y<7)||(rx===2&&ry===2&&62-x-y<7);
  if(!clipped)put(o,x,y,colour(c));
 }
 if(ry===2) {box(o,3,29,26,2,'603e2b');box(o,6,29,3,3,'7b4e2e');box(o,23,29,3,3,'7b4e2e');}
 return o;
}
function floorTile(v) {
 var o=canvas(32,32);
 for(var y=0;y<32;y++)for(var x=0;x<32;x++) {
  var row=Math.floor(y/8),py=y%8,px=(x+(row%2)*16)%32;
  var c=py===0?'463b2c':py===1?'8c6a43':py===7?'624b33':px===0?'55412f':(row%2?'76563a':'705137');
  if(x>2&&x<29&&y>2&&y<29&&((x*13+y*7+v*17)%193===0))c='82603e';
  put(o,x,y,colour(c));
 }return o;
}
var walls,roof,floors;
if(cfg.regenerate||cfg.regenerateWalls) {
 walls=canvas(256,96);
 for(var i=0;i<16;i++)blit(wall(i),walls,i%8*32,Math.floor(i/8)*48);
 save(walls,cfg.fixtures.walls,cfg.sources.walls);
}else walls=read(cfg.sources.walls);
if(cfg.regenerate) {
 roof=canvas(96,128);floors=canvas(96,32);
 for(var i=0;i<12;i++)blit(roofTile(i),roof,i%3*32,Math.floor(i/3)*32);
 for(var i=0;i<3;i++)blit(floorTile(i),floors,i*32,0);
 save(roof,cfg.fixtures.roof,cfg.sources.roof);save(floors,cfg.fixtures.floors,cfg.sources.floors);
} else {roof=read(cfg.sources.roof);floors=read(cfg.sources.floors);}
[[walls,256,96],[roof,96,128],[floors,96,32]].forEach(function(t) {
 if(t[0].width!==t[1]||t[0].height!==t[2])throw new Error('Building master geometry changed');
 t[0].pixels.forEach(function(c) {if(pc.rgbaA(c)!==0&&pc.rgbaA(c)!==255)throw new Error('Building tiles require binary alpha');});
});
var grass=read(cfg.forest),stone=read(cfg.stone),board=canvas(768,256);
for(var y=0;y<256;y++)for(var x=0;x<768;x++)put(board,x,y,grass.pixels[(y%32)*grass.width+x%32]);
function roofPatch(out,x,y,w,h,a) {
 var xs=[0,32,w-32],ys=[0,32,h-32],ws=[32,w-64,32],hs=[32,h-64,32];
 for(var r=0;r<3;r++)for(var c=0;c<3;c++)for(var yy=0;yy<hs[r];yy+=32)for(var xx=0;xx<ws[c];xx+=32)
  blit(roof,out,x+xs[c]+xx,y+ys[r]+yy,c*32,r*32,Math.min(32,ws[c]-xx),Math.min(32,hs[r]-yy),a);
 for(var section=0;section<3;section++)for(var xx=0;xx<ws[section];xx+=32) {
  var i=9+section;
  blit(roof,out,x+xs[section]+xx,y+16,i%3*32,96,Math.min(32,ws[section]-xx),32,a);
 }
}
for(var b=0;b<2;b++) {
 var bx=48+b*384,by=64;
 for(var ty=0;ty<5;ty++)for(var tx=0;tx<9;tx++) {
  var edge=ty===0||ty===4||tx===0||tx===8;
  blit(edge?stone:floors,board,bx+tx*32,by+ty*32,edge?96:((tx+ty)%3)*32,0,32,32);
  if(edge) {
   var left=Math.max(0,20-tx*32),right=Math.min(32,268-tx*32);
   var top=Math.max(0,20-ty*32),bottom=Math.min(32,140-ty*32);
   if(tx===4&&ty===4){left=0;right=32;top=0;bottom=20;}
   if(right>left&&bottom>top)blit(floors,board,bx+tx*32+left,by+ty*32+top,((tx+ty)%3)*32+left,top,right-left,bottom-top);
  }
  if(!edge)continue;
  var index=ty===0?(tx===0?8:tx===8?9:1):ty===4?(tx===0?10:tx===8?11:tx===4?7:tx===2||tx===6?4:0):tx===0?(ty===2?5:2):(ty===2?6:3);
  if(b===1&&ty===4)index=tx===0?14:tx===8?15:tx===4?13:12;
  blit(walls,board,bx+tx*32,by+ty*32-16,index%8*32,Math.floor(index/8)*48,32,48);
 }
 roofPatch(board,bx-8,by-48,304,184,b===0?255:25);
}
save(scale(board,2),cfg.fixtures['building-pilot-2x.png'],cfg.previews['building-pilot-2x.png']);
var inspect=canvas(256,96);
for(var y=0;y<96;y++)for(var x=0;x<256;x++)put(inspect,x,y,colour(((x>>3)+(y>>3))%2?'b4c5cf':'e5ded0'));
blit(walls,inspect,0,0);save(scale(inspect,3),cfg.fixtures['walls-inspection-3x.png'],cfg.previews['walls-inspection-3x.png']);
console.log('BUILDING_PILOT_OK: 16 directional wall/window/door/corner/cutaway cells, 12 roof/ridge cells, 3 matching floors; binary alpha.');
