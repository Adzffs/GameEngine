// Lossless PixelLab frame assembly and inspection; no pose generation.
var cfg=__CONFIG__,pc=app.pixelColor;
function canvas(w,h){return {width:w,height:h,pixels:new Array(w*h).fill(0)};}
function read(path){var s=app.open(path).sprite,c=s.layer(0).cel(0),img=c.image,out=canvas(s.width,s.height);
  if(s.layerCount!==1||s.layer(0).celCount!==1)throw new Error('One layer/frame required');
  for(var y=0;y<img.height;y++)for(var x=0;x<img.width;x++)out.pixels[(y+c.y)*out.width+x+c.x]=img.getPixel(x,y);return out;}
function save(out,fixture,path){var s=app.open(fixture).sprite,img=s.layer(0).cel(0).image;
  if(s.width!==out.width||s.height!==out.height)throw new Error('Wrong fixture size');
  for(var y=0;y<out.height;y++)for(var x=0;x<out.width;x++)img.putPixel(x,y,out.pixels[y*out.width+x]);s.commit();s.saveAs(path,false);}
function blit(src,out,left,top,scale){for(var y=0;y<src.height*scale;y++)for(var x=0;x<src.width*scale;x++){
  var c=src.pixels[Math.floor(y/scale)*src.width+Math.floor(x/scale)];if(pc.rgbaA(c))out.pixels[(top+y)*out.width+left+x]=c;}}
function cell(sheet,facing,pose){var out=canvas(64,64);for(var y=0;y<64;y++)for(var x=0;x<64;x++)out.pixels[y*64+x]=sheet.pixels[(facing*64+y)*sheet.width+pose*64+x];return out;}
function backdrop(w,h){var out=canvas(w,h),c=pc.rgba(202,205,190,255);out.pixels.fill(c);return out;}
var atlas;
if(cfg.importing){atlas=canvas(576,512);for(var d=0;d<8;d++)for(var p=0;p<9;p++)blit(read(cfg.inputs[cfg.directions[d]][p]),atlas,p*64,d*64,1);
}else atlas=read(cfg.source);
if(atlas.width!==576||atlas.height!==512)throw new Error('Expected nine 64x64 cells in eight directional rows');
var report={cells:72,walkFrames:64,idleFrames:8,binaryAlpha:true,clippedCells:0,emptyCells:0,bounds:[]};
var contacts=backdrop(1152,1024);
for(var d=0;d<8;d++)for(var p=0;p<9;p++){
  var image=cell(atlas,d,p),n=0,b=[64,64,-1,-1];
  for(var y=0;y<64;y++)for(var x=0;x<64;x++){var a=pc.rgbaA(image.pixels[y*64+x]);if(a!==0&&a!==255)throw new Error('Nonbinary frame alpha');
    if(a){n++;b[0]=Math.min(b[0],x);b[1]=Math.min(b[1],y);b[2]=Math.max(b[2],x);b[3]=Math.max(b[3],y);}}
  if(n<100)report.emptyCells++;if(b[0]===0||b[1]===0||b[2]===63||b[3]===63)report.clippedCells++;
  report.bounds.push({direction:cfg.directions[d],pose:p,pixels:n,bounds:b});blit(image,contacts,p*128,d*128,2);
}
if(report.emptyCells||report.clippedCells)throw new Error('Empty or edge-clipped character cell');
// Validate every cell before replacing an existing editable master.
if(cfg.importing){save(atlas,cfg.fixtures.master,cfg.source);var saved=read(cfg.source);report.exactImportedCells=0;
  for(var d=0;d<8;d++)for(var p=0;p<9;p++){var original=read(cfg.inputs[cfg.directions[d]][p]),restored=cell(saved,d,p);
    for(var i=0;i<original.pixels.length;i++)if(pc.rgbaA(original.pixels[i])!==pc.rgbaA(restored.pixels[i])||
      (pc.rgbaA(original.pixels[i])&&original.pixels[i]!==restored.pixels[i]))throw new Error('Imported visible pixels changed');
    report.exactImportedCells++;}}
save(contacts,cfg.fixtures.contacts,cfg.contacts);
for(var p=0;p<8;p++){var loop=backdrop(768,384);for(var d=0;d<8;d++)blit(cell(atlas,d,p+1),loop,(d%4)*192,Math.floor(d/4)*192,3);
  save(loop,cfg.fixtures.loop,cfg.loopFrames[p]);}
console.log('PIXELLAB_WALK_JSON='+JSON.stringify(report));console.log('PIXELLAB_WALK_OK: eight supplied idles and 64 generated walk frames, exact source pixels');
