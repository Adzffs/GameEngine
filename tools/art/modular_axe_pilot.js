// Lossless body reuse plus an independent shape, semantic mask and hand track.
// No procedural body generation or legacy wrist formulas run in this exporter.
var recipe=cfg.recipe, cellSize=96, materials=recipe.materials;
function fail(message){throw new Error(message);}
function equal(a,b){return pc.rgbaA(a)===pc.rgbaA(b)&&(!pc.rgbaA(a)||a===b);}
function bounded(image,minimum){
  var ink=0;
  for(var y=0;y<image.height;y++)for(var x=0;x<image.width;x++){
    var a=pc.rgbaA(image.pixels[y*image.width+x]);
    if(a!==0&&a!==255)fail('Nonbinary alpha');
    if(a){ink++;if(x===0||y===0||x===image.width-1||y===image.height-1)fail('Clipped proof artwork');}}
  if(ink<minimum)fail('Empty or incomplete proof artwork');
}
function line(image,a,b,c,width){
  var steps=Math.max(Math.abs(b[0]-a[0]),Math.abs(b[1]-a[1]),1),r=Math.floor(width/2);
  for(var i=0;i<=steps;i++)for(var y=-r;y<=r;y++)for(var x=-r;x<=r;x++)
    put(image,Math.round(a[0]+(b[0]-a[0])*i/steps)+x,Math.round(a[1]+(b[1]-a[1])*i/steps)+y,c);
}
function polygon(image,points,c){
  for(var y=0;y<image.height;y++)for(var x=0;x<image.width;x++){
    var inside=false;
    for(var i=0,j=points.length-1;i<points.length;j=i++){
      var a=points[i],b=points[j];
      if((a[1]>y)!==(b[1]>y)&&x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0])inside=!inside;}
    if(inside)put(image,x,y,c);}
}
var tags=[colour('ff0000'),colour('00ff00'),colour('0000ff')];
var body,shape,mask,legacy=null;
if(cfg.importLegacy){
  legacy=read(cfg.legacyBody);
  if(legacy.width!==768||legacy.height!==768)fail('Expected the saved eight-pose/eight-direction body master');
  body=crop(legacy,0,192,768,96);
  shape=canvas(32,40);mask=canvas(32,40);
  var grip=recipe.shape_grip,head=[grip[0],grip[1]-17];
  function local(a,b){return[head[0]+b,head[1]-a];}
  line(shape,[grip[0],grip[1]+4],head,colour('30211d'),3);
  line(shape,grip,head,colour('b27c48'),1);
  polygon(shape,[local(-4,-2),local(3,-2),local(5,4),local(-5,6)],colour('222b2b'));
  var regions=[[local(-3,-1),local(2,-1),local(4,3),local(-4,5)],
               [local(-2,0),local(2,0),local(3,3),local(-3,4)]];
  for(var region=0;region<2;region++){
    polygon(shape,regions[region],colour(materials[0].metal[region]));
    polygon(mask,regions[region],tags[region]);}
  line(shape,local(-3,4),local(3,3),colour(materials[0].metal[2]),1);
  line(mask,local(-3,4),local(3,3),tags[2],1);
}else{body=read(cfg.sources[0]);shape=read(cfg.sources[1]);mask=read(cfg.sources[2]);}
if(body.width!==768||body.height!==96||shape.width!==32||shape.height!==40||mask.width!==32||mask.height!==40)
  fail('Editable source dimensions differ from the recipe');
bounded(shape,30);bounded(mask,10);
var regionCounts=[0,0,0],regionAt=[];
for(var i=0;i<shape.pixels.length;i++){
  var tag=mask.pixels[i],region=-1;
  if(pc.rgbaA(tag)){
    region=tags.indexOf(tag);if(region<0)fail('Unknown semantic metal mask colour');
    if(!pc.rgbaA(shape.pixels[i]))fail('Metal mask paints outside the tool');regionCounts[region]++;}
  regionAt[i]=region;
}
if(regionCounts.some(function(n){return !n;}))fail('Every metal shading region must be present');
var tools=canvas(768,materials.length*96),combined=canvas(768,materials.length*96),
    bodyCells=[],toolCells=[],composites=[],regionsByPose=[];
var report={direction:'east',bodyPoses:8,shapeViews:1,materials:materials.length,
  exactBodyCells:0,metalRegionPixels:regionCounts,protectedSourcePixels:0,
  bodyAndHandleMismatchPixels:0,alphaMismatchPixels:0,materialDifferencePixels:[],
  composition:recipe.composition,foregroundPixelsPreserved:0,foregroundToolOverlapPixels:0,
  foregroundPixelsByPose:[],visibleToolOverBodyPixelsByPose:[],maxSecondaryHandShaftDistance:0,
  sourceStatus:'Existing unchanged body fixture; body quality remains unaccepted'};
for(var i=0;i<shape.pixels.length;i++)if(pc.rgbaA(shape.pixels[i])&&regionAt[i]<0)report.protectedSourcePixels++;
for(var p=0;p<8;p++){
  var frame=recipe.frames[p],b=crop(body,p*96,0,96,96);bounded(b,100);bodyCells[p]=b;
  if(legacy){for(var i=0;i<b.pixels.length;i++)if(!equal(b.pixels[i],legacy.pixels[(192+Math.floor(i/96))*768+p*96+i%96]))
    fail('Imported body pixels changed');}
  for(var i=0;i<b.pixels.length;i++)if(!equal(b.pixels[i],body.pixels[Math.floor(i/96)*768+p*96+i%96]))
    fail('Shared body cell changed during assembly');
  report.exactBodyCells++;
  // The axe keeps one depth plane. Only explicitly authored body parts cover
  // it; never flip the whole tool behind the body to hide a head intersection.
  var foreground=new Array(9216).fill(false),foregroundCount=0;
  Object.keys(frame.foreground).forEach(function(role){
    frame.foreground[role].forEach(function(span){
      for(var x=span[1];x<=span[2];x++){
        var at=span[0]*96+x;
        if(!pc.rgbaA(b.pixels[at]))fail('Foreground mask references empty body pixels');
        foreground[at]=true;foregroundCount++;
      }
    });
  });
  report.foregroundPixelsByPose.push(foregroundCount);
  var ux=frame.aim[0],uy=frame.aim[1],vx=-uy,vy=ux;
  var off=[frame.off_hand[0]-frame.main_hand[0],frame.off_hand[1]-frame.main_hand[1]];
  report.maxSecondaryHandShaftDistance=Math.max(report.maxSecondaryHandShaftDistance,Math.abs(off[0]*vx+off[1]*vy));
  var coverage=new Array(9216).fill(-1);
  for(var tier=0;tier<materials.length;tier++){
    var tool=canvas(96,96),result=canvas(96,96),colours=materials[tier].metal.map(colour);
    for(var y=0;y<96;y++)for(var x=0;x<96;x++){
      var dx=x-frame.main_hand[0],dy=y-frame.main_hand[1],
          sx=Math.round(recipe.shape_grip[0]+dx*vx+dy*vy),
          sy=Math.round(recipe.shape_grip[1]-dx*ux-dy*uy);
      if(sx<0||sy<0||sx>=32||sy>=40)continue;
      var source=sy*32+sx,c=shape.pixels[source],region=regionAt[source];
      if(pc.rgbaA(c)){
        tool.pixels[y*96+x]=region>=0?colours[region]:c;
        if(tier===0)coverage[y*96+x]=region;}
    }
    bounded(tool,30);
    blit(b,result,0,0);blit(tool,result,0,0);
    var visibleOverlap=0;
    for(var at=0;at<9216;at++){
      if(foreground[at]){
        result.pixels[at]=b.pixels[at];report.foregroundPixelsPreserved++;
        if(pc.rgbaA(tool.pixels[at]))report.foregroundToolOverlapPixels++;
      }else if(pc.rgbaA(b.pixels[at])&&pc.rgbaA(tool.pixels[at]))visibleOverlap++;
    }
    if(tier===0)report.visibleToolOverBodyPixelsByPose.push(visibleOverlap);
    bounded(result,100);
    if(!toolCells[tier]){toolCells[tier]=[];composites[tier]=[];}
    toolCells[tier][p]=tool;composites[tier][p]=result;
    blit(tool,tools,p*96,tier*96);blit(result,combined,p*96,tier*96);
  }
  regionsByPose[p]=coverage;
}
for(var tier=1;tier<materials.length;tier++){
  var differences=0;
  for(var p=0;p<8;p++)for(var i=0;i<9216;i++){
    var a=toolCells[0][p].pixels[i],b=toolCells[tier][p].pixels[i];
    if(pc.rgbaA(a)!==pc.rgbaA(b))report.alphaMismatchPixels++;
    if(!equal(a,b)&&regionsByPose[p][i]<0)report.bodyAndHandleMismatchPixels++;
    var ca=composites[0][p].pixels[i],cb=composites[tier][p].pixels[i];
    if(!equal(ca,cb)){
      differences++;if(regionsByPose[p][i]<0)report.bodyAndHandleMismatchPixels++;
    }
  }
  if(!differences)fail('Material appearance is not visibly distinct');
  report.materialDifferencePixels.push({material:materials[tier].id,differentPixels:differences});
}
if(report.bodyAndHandleMismatchPixels||report.alphaMismatchPixels)fail('A material change altered protected pixels or silhouette');
// Validate the entire candidate before creating/replacing any editable master.
if(cfg.importLegacy){
  save(body,cfg.fixtures.body,cfg.sources[0]);save(shape,cfg.fixtures.shape,cfg.sources[1]);save(mask,cfg.fixtures.mask,cfg.sources[2]);
}
save(body,cfg.fixtures.body,cfg.stage+'/textures/body.png');
save(shape,cfg.fixtures.shape,cfg.stage+'/textures/axe.png');
save(mask,cfg.fixtures.mask,cfg.stage+'/textures/metal-mask.png');
save(tools,cfg.fixtures.tools,cfg.stage+'/textures/tools.png');
save(combined,cfg.fixtures.composites,cfg.stage+'/textures/composites.png');
var font={A:['010','101','111','101','101'],B:['110','101','110','101','110'],E:['111','100','110','100','111'],
 I:['111','010','010','010','111'],L:['100','100','100','100','111'],N:['101','111','111','111','101'],
 O:['010','101','101','101','010'],R:['110','101','110','101','101'],S:['011','100','010','001','110'],
 T:['111','010','010','010','010'],Z:['111','001','010','100','111'],
 C:['011','100','100','100','011'],D:['110','101','101','101','110'],F:['111','100','110','100','100'],
 G:['011','100','101','101','011'],H:['101','101','111','101','101'],J:['001','001','001','101','010'],
 K:['101','101','110','101','101'],M:['101','111','111','101','101'],P:['110','101','110','100','100'],
 Q:['010','101','101','111','011'],U:['101','101','101','101','111'],V:['101','101','101','101','010'],
 W:['101','101','111','111','101'],X:['101','101','010','101','101'],Y:['101','101','010','010','010'],
 '0':['111','101','101','101','111'],'1':['010','110','010','010','111'],'2':['110','001','010','100','111'],
 '3':['110','001','010','001','110'],'4':['101','101','111','001','001'],'5':['111','100','110','001','110'],
 '6':['011','100','110','101','010'],'7':['111','001','010','010','010'],'8':['010','101','010','101','010'],
 '9':['010','101','011','001','110'],'-':['000','000','111','000','000']};
function label(out,name,x,y){name.toUpperCase().split('').forEach(function(c,index){
  var glyph=font[c];if(!glyph)return;
  for(var gy=0;gy<5;gy++)for(var gx=0;gx<3;gx++)if(glyph[gy][gx]==='1')put(out,x+index*4+gx,y+gy,colour('26333c'));
});}
function board(w,h){var out=canvas(w,h);out.pixels.fill(colour('caced0'));return out;}
for(var p=0;p<8;p++){
  var row=board(materials.length*96,108);
  for(var tier=0;tier<materials.length;tier++){
    label(row,materials[tier].id,tier*96+8,3);blit(composites[tier][p],row,tier*96,12);}
  save(scale(row,3),cfg.fixtures.comparison,cfg.loopFrames[p]);
}
var contact=board(768,materials.length*108);
for(var tier=0;tier<materials.length;tier++){
  label(contact,materials[tier].id,8,tier*108+3);
  for(var p=0;p<8;p++)blit(composites[tier][p],contact,p*96,tier*108+12);
}
save(scale(contact,2),cfg.fixtures.contact,cfg.stage+'/previews/contact.png');
var diagnostic=board(768,108);
function cross(out,point,c){
  var x=Math.round(point[0]),y=Math.round(point[1]);
  for(var k=-2;k<=2;k++){put(out,x+k,y,c);put(out,x,y+k,c);}
}
for(var p=0;p<8;p++){
  var frame=recipe.frames[p],tile=canvas(96,96);blit(composites[0][p],tile,0,0);
  cross(tile,recipe.ground_pivot,colour('e5b93f'));
  cross(tile,frame.off_hand,colour('367ee8'));
  cross(tile,frame.main_hand,colour('e83d55'));
  var a=recipe.shape_grip[1]-recipe.cutting_edge_local[1],b=recipe.cutting_edge_local[0]-recipe.shape_grip[0];
  cross(tile,[frame.main_hand[0]+frame.aim[0]*a-frame.aim[1]*b,
              frame.main_hand[1]+frame.aim[1]*a+frame.aim[0]*b],colour('24a36a'));
  blit(tile,diagnostic,p*96,12);
}
save(scale(diagnostic,2),cfg.fixtures.grips,cfg.stage+'/previews/grips.png');
var occlusion=board(768,108);
for(var p=0;p<8;p++){
  var tile=canvas(96,96);blit(bodyCells[p],tile,0,0);
  Object.keys(recipe.frames[p].foreground).forEach(function(role){
    recipe.frames[p].foreground[role].forEach(function(span){
      for(var x=span[1];x<=span[2];x++)put(tile,x,span[0],colour(role==='head'?'b550a1':role==='main_hand'?'e83d55':'367ee8'));
    });
  });
  label(occlusion,String(p),p*96+8,3);blit(tile,occlusion,p*96,12);
}
save(scale(occlusion,2),cfg.fixtures.grips,cfg.stage+'/previews/occlusion.png');
console.log('MODULAR_AXE_JSON='+JSON.stringify(report));
