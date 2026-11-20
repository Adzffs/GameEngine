// Editable player and equipment atlases authored with LibreSprite pixel APIs.
var directions = ['s','w','e','n','sw','se','nw','ne'];
var vectors = [[0,1],[-1,0],[1,0],[0,-1],[-0.707,0.707],[0.707,0.707],[-0.707,-0.707],[0.707,-0.707]];
function line(out,a,b,hex,width) {
  var dx=b[0]-a[0],dy=b[1]-a[1],steps=Math.max(Math.abs(dx),Math.abs(dy),1),r=Math.floor(width/2),c=colour(hex);
  for(var i=0;i<=steps;i++) for(var y=-r;y<=r;y++) for(var x=-r;x<=r;x++)
    put(out,Math.round(a[0]+dx*i/steps)+x,Math.round(a[1]+dy*i/steps)+y,c);
}
function polygon(out,points,hex) {
  var c=colour(hex);
  for(var y=0;y<96;y++) for(var x=0;x<96;x++) {
    var inside=false;
    for(var i=0,j=points.length-1;i<points.length;j=i++) {
      var a=points[i],b=points[j];
      if((a[1]>y)!==(b[1]>y)&&x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0]) inside=!inside;
    }
    if(inside) put(out,x,y,c);
  }
}
function normalize(v) { var n=Math.sqrt(v[0]*v[0]+v[1]*v[1]); return [v[0]/n,v[1]/n]; }
function geometry(d,p,style) {
  var v=rigDirections[d],right=[-v[1],v[0]],attack=p>=8,a=p-8;
  var bob=attack?0:[0,-1,0,1,0,-1,0,1][p],sway=attack?0:Math.sin(p*Math.PI/4);
  var carry=[48+right[0]*5+v[0]*sway,53+right[1]*3+bob+v[1]*sway*0.5];
  var grip=carry,off=[48-right[0]*5,52-right[1]*3+bob];
  var carryAim=normalize([right[0]*0.2,-1+right[1]*0.1]),aim=carryAim,lean=[0,0];
  if(attack) {
    var reach=[0,-2,-3,9,10,5,2,0][a],across=[5,6,6,3,-2,-3,2,5][a];
    var height=[0,-3,-5,-4,-2,1,1,0][a];
    grip=[48+right[0]*across+v[0]*reach,53+right[1]*across*0.45+v[1]*reach*0.55+height];
    var load=normalize([-v[0]*0.7+right[0]*0.8,-v[1]*0.3+right[1]*0.3-1.1]);
    var strike=normalize([v[0],v[1]*0.65]);
    var through=normalize([v[0]*0.55-right[0]*0.85,v[1]*0.35-right[1]*0.55]);
    var low=normalize([v[0]*0.15-right[0]*0.8,v[1]*0.1-right[1]*0.5+0.5]);
    aim=[carryAim,normalize([load[0]+carryAim[0],load[1]+carryAim[1]]),load,strike,through,low,
      normalize([low[0]*0.3+carryAim[0]*0.7,low[1]*0.3+carryAim[1]*0.7]),carryAim][a];
    var weight=[0,-1,-1,2,3,2,1,0][a];
    lean=[Math.round(v[0]*weight),Math.round(v[1]*weight*0.35)];
    off=[48-right[0]*5+v[0],51-right[1]*3];
    if(a===7) { grip=carry; off=[48-right[0]*5,52-right[1]*3]; }
    if(style===0&&a!==7) {
      grip=[48+right[0]*4+v[0]*[0,-1,-2,10,11,6,2,0][a],52+right[1]*2+v[1]*[0,-1,-2,6,7,3,1,0][a]];
      if(a>=5) lean=[0,0];
    }
    if((style===3||style===4)&&a!==7) {
      var side=v[0]<0?-1:1;
      var bite=normalize([v[0]*0.6+side*v[1]*0.8,v[1]*0.6-side*v[0]*0.8]);
      if(d>=4) bite=normalize([v[0],-0.15]);
      var raised=normalize([-v[0]*0.35+right[0]*0.25,-1.2-v[1]*0.1]);
      aim=[carryAim,normalize([raised[0]+carryAim[0],raised[1]+carryAim[1]]),raised,bite,bite,
        normalize([bite[0]*0.6,bite[1]*0.6-0.7]),normalize([carryAim[0]+bite[0]*0.2,carryAim[1]]),carryAim][a];
      var toolReach=[0,-1,-2,8,9,5,2,0][a];
      grip=[48+right[0]*(style===3?4:2)+v[0]*toolReach,
        52+right[1]*2+v[1]*toolReach*0.5+[0,-4,-6,-1,0,-2,-1,0][a]];
    }
  }
  if(style===2) {
    grip=[48+v[0]*(attack?[0,-2,-3,5,6,3,1,0][a]:0)+right[0],
      51+v[1]*(attack?[0,-1,-2,3,4,2,1,0][a]:0)+(attack?[0,-3,-4,-1,0,1,1,0][a]:bob)];
    off=[grip[0]-aim[0]*4,grip[1]-aim[1]*4];
  }
  var joints=rigJoints(d,lean,bob,grip,off);
  return {grip:joints.grip,off:joints.off,aim:aim,lean:lean,right:right,joints:joints,bob:bob};
}
function bodyPose(base,d,p,style) {
  var g=geometry(d,p,style);
  return rigBody(base,d,g.lean,0,g.joints);
}
function equipmentPose(d,p,index) {
  var style=index===1?2:index>=2&&index<=4?3:index>=5&&index<=7?4:1;
  var out=canvas(96,96),g=geometry(d,p,style),u=g.aim,v=[-u[1],u[0]],grip=g.grip;
  function pt(a,b) { return [grip[0]+u[0]*a+v[0]*b,grip[1]+u[1]*a+v[1]*b]; }
  if(index<2&&(p===11||p===12)) {
    // Short, angular motion accents belong to the fast strike, never recovery.
    var radius=index===1?27:19,angle=Math.atan2(u[1],u[0]),previous=null;
    for(var sample=0;sample<6;sample++) {
      var theta=angle-0.48+sample*0.09;
      var point=[grip[0]+Math.cos(theta)*radius,grip[1]+Math.sin(theta)*radius];
      if(previous) line(out,previous,point,p===11?'dbc79a':'a69a7e',1);
      previous=point;
    }
  }
  if(index===8) {
    var c=[g.off[0]+vectors[d][0]*4,g.off[1]+vectors[d][1]*2],w=(d===1||d===2)?4:7;
    polygon(out,[[c[0]-w,c[1]-8],[c[0]+w,c[1]-8],[c[0]+w-1,c[1]+3],[c[0],c[1]+9],[c[0]-w+1,c[1]+3]],'222b2b');
    polygon(out,[[c[0]-w+1,c[1]-7],[c[0]+w-1,c[1]-7],[c[0]+w-2,c[1]+2],[c[0],c[1]+7],[c[0]-w+2,c[1]+2]],'b27c48');
    for(var x=-w+2;x<w-1;x+=3) line(out,[c[0]+x,c[1]-6],[c[0]+x,c[1]+2],'7b4e2e',1);
    line(out,[c[0]-w+1,c[1]-6],[c[0]+w-1,c[1]-6],'e3c073',1);
    line(out,[c[0]-2,c[1]],[c[0]+2,c[1]],'a69a7e',3);
    return out;
  }
  var tier=index<2?0:index<5?index-2:index-5;
  var metal=[['84622c','c8a653','e3c073'],['404542','8da39c','a69a7e'],['37617d','8da39c','efe0b7']][tier];
  if(index<2) {
    var length=index===1?27:19,w=index===1?4:3;
    polygon(out,[pt(3,-w),pt(length-5,-w),pt(length,0),pt(length-5,w),pt(3,w)],'222b2b');
    polygon(out,[pt(4,-w+1),pt(length-5,-w+1),pt(length-1,0),pt(length-5,w-1),pt(4,w-1)],index===1?'8da39c':metal[0]);
    line(out,pt(5,0),pt(length-3,0),index===1?'efe0b7':metal[2],1);
    line(out,pt(3,-w-3),pt(3,w+3),'30211d',3);
    line(out,pt(3,-w-2),pt(3,w+2),'e3c073',1);
    line(out,pt(-5,0),pt(2,0),'30211d',3); line(out,pt(-4,0),pt(1,0),'7b4e2e',1);
    if(index===1) line(out,pt(-6,0),pt(-5,0),'c8a653',3);
  } else {
    line(out,pt(-5,0),pt(16,0),'30211d',3); line(out,pt(-4,0),pt(16,0),'b27c48',1);
    if(index<5) {
      // Put the cutting edge on the target-facing side of the shaft.
      var cut=vectors[d]; if(v[0]*cut[0]+v[1]*cut[1]<0) v=[-v[0],-v[1]];
      polygon(out,[pt(12,-2),pt(19,-2),pt(21,5),pt(11,8)],'222b2b');
      polygon(out,[pt(13,-1),pt(18,-1),pt(20,4),pt(12,7)],metal[0]);
      polygon(out,[pt(14,0),pt(18,0),pt(19,4),pt(13,6)],metal[1]);
      line(out,pt(13,6),pt(19,4),metal[2],1);
    } else {
      polygon(out,[pt(15,-12),pt(19,-6),pt(19,6),pt(15,12),pt(16,4),pt(16,-4)],'222b2b');
      line(out,pt(16,-9),pt(18,0),metal[1],3); line(out,pt(18,0),pt(16,9),metal[1],3);
      line(out,pt(16,-9),pt(17,0),metal[2],1); line(out,pt(17,0),pt(16,9),metal[2],1);
    }
  }
  // The fingers wrap over the handle, keeping the object attached to the hand.
  line(out,[grip[0]-1,grip[1]],grip,'edbd89',1);
  return out;
}
var bodies,equipment;
if(cfg.regenerate) {
  var player=read(cfg.player);
  if(player.width!==256||player.height!==384) throw new Error('Melee requires the eight-pose player master');
  bodies=canvas(1536,3840); equipment=canvas(1536,6912);
  for(var style=0;style<5;style++) for(var d=0;d<8;d++) for(var p=0;p<16;p++)
    blit(bodyPose(crop(player,(p<8?p:0)*32,d*48,32,48),d,p,style),bodies,p*96,(style*8+d)*96);
  for(var index=0;index<9;index++) for(var d=0;d<8;d++) for(var p=0;p<16;p++)
    blit(equipmentPose(d,p,index),equipment,p*96,(index*8+d)*96);
} else { bodies=read(cfg.sources.player); equipment=read(cfg.sources.equipment); }
if(bodies.width!==1536||bodies.height!==3840||equipment.width!==1536||equipment.height!==6912) throw new Error('Incorrect melee atlas size');
var report={bodyFrames:640,equipmentFrames:1152,clippedFrames:0,emptyFrames:0,maxArmReach:9.5};
[bodies,equipment].forEach(function(atlas) {
  for(var cy=0;cy<atlas.height;cy+=96) for(var cx=0;cx<1536;cx+=96) {
    var count=0;
    for(var y=0;y<96;y++) for(var x=0;x<96;x++) {
      var alpha=pc.rgbaA(atlas.pixels[(cy+y)*1536+cx+x]);
      if(alpha!==0&&alpha!==255) throw new Error('Non-binary melee alpha');
      if(alpha) { count++; if(x===0||y===0||x===95||y===95) report.clippedFrames++; }
    }
    if(!count) report.emptyFrames++;
  }
});
if(report.clippedFrames||report.emptyFrames) throw new Error(JSON.stringify(report));
// Keep the editable 96px cells, but ship only each frame's occupied rectangle.
// Include the ground pivot so the runtime never needs an out-of-frame anchor.
function packedLayout(atlas) {
  var entries=[],width=1024,x=1,y=1,rowHeight=0;
  for(var cy=0;cy<atlas.height;cy+=96) for(var cx=0;cx<atlas.width;cx+=96) {
    var left=48,top=64,right=49,bottom=65;
    for(var py=0;py<96;py++) for(var px=0;px<96;px++)
      if(pc.rgbaA(atlas.pixels[(cy+py)*atlas.width+cx+px])) {
        left=Math.min(left,px);top=Math.min(top,py);
        right=Math.max(right,px+1);bottom=Math.max(bottom,py+1);
      }
    var w=right-left,h=bottom-top;
    if(x+w+1>width) { x=1;y+=rowHeight+2;rowHeight=0; }
    entries.push({source:[cx+left,cy+top,w,h],rect:[x,y,w,h],pivot:[48-left,64-top]});
    x+=w+2;rowHeight=Math.max(rowHeight,h);
  }
  var height=y+rowHeight+1;
  if(height>4096||width*height>4194304) throw new Error('Packed melee exceeds the existing image budget');
  return {size:[width,height],frames:entries};
}
console.log('MELEE_PACKING_JSON='+JSON.stringify({player:packedLayout(bodies),equipment:packedLayout(equipment)}));
// Diagonal motion must stay on integer pixels and preserve the whole body.
for(var row=0;row<40;row++) for(var p=0;p<16;p++) {
  var upper=0,lower=0;
  for(var y=25;y<43;y++) for(var x=30;x<66;x++) if(pc.rgbaA(bodies.pixels[(row*96+y)*1536+p*96+x])) upper++;
  for(var y=56;y<67;y++) for(var x=30;x<66;x++) if(pc.rgbaA(bodies.pixels[(row*96+y)*1536+p*96+x])) lower++;
  if(upper<20||lower<20) throw new Error('Incomplete melee body in row '+row+' pose '+p);
}
if(cfg.regenerate) { save(bodies,cfg.fixtures.player,cfg.sources.player); save(equipment,cfg.fixtures.equipment,cfg.sources.equipment); }
function combined(d,p,index,shield) {
  var out=canvas(96,96),style=index===1?2:index<0?0:index>=2&&index<=4?3:index>=5&&index<=7?4:1;
  var body=crop(bodies,p*96,(style*8+d)*96,96,96),held=index<0?null:crop(equipment,p*96,(index*8+d)*96,96,96);
  var off=shield?crop(equipment,p*96,(8*8+d)*96,96,96):null;
  var behind=d===1||d===3||d===6,offBehind=d===2||d===3||d===7;
  if(off&&offBehind) blit(off,out,0,0); if(held&&behind) blit(held,out,0,0);
  blit(body,out,0,0); if(held&&!behind) blit(held,out,0,0); if(off&&!offBehind) blit(off,out,0,0);
  return out;
}
function background(w,h) {
  var out=canvas(w,h);
  for(var y=0;y<h;y++) for(var x=0;x<w;x++) out.pixels[y*w+x]=((x>>3)+(y>>3))%2?colour('748057'):colour('aaad8b');
  return out;
}
for(var p=0;p<8;p++) {
  var board=background(384,192);
  for(var d=0;d<8;d++) blit(combined(d,p+8,0,true),board,d%4*96,Math.floor(d/4)*96);
  save(scale(board,3),cfg.fixtures.board,cfg.frames[p]);
}
var contact=background(768,768);
for(var d=0;d<8;d++) for(var p=0;p<8;p++) blit(combined(d,p+8,0,true),contact,p*96,d*96);
save(scale(contact,2),cfg.fixtures.contact,cfg.contact);
var gear=background(864,96);
for(var index=0;index<9;index++) blit(combined(2,11,index===8?-1:index,index===8),gear,index*96,0);
save(scale(gear,2),cfg.fixtures.gear,cfg.gear);
console.log('MELEE_VALIDATION_JSON='+JSON.stringify(report));
console.log('MELEE_PILOT_OK');
if(cfg.comparison) {
  var oldBody=read(cfg.comparison.player),oldGear=read(cfg.comparison.equipment);
  if(oldBody.width!==960||oldGear.width!==960) throw new Error('Comparison requires the six-pose baseline');
  function oldCombined(d,pose) {
    var out=canvas(96,96),p=pose+4;
    var body=crop(oldBody,p*96,(8+d)*96,96,96),held=crop(oldGear,p*96,d*96,96,96),off=crop(oldGear,p*96,(64+d)*96,96,96);
    var behind=d===1||d===3||d===6,offBehind=d===2||d===3||d===7;
    if(offBehind) blit(off,out,0,0);if(behind) blit(held,out,0,0);
    blit(body,out,0,0);if(!behind) blit(held,out,0,0);if(!offBehind) blit(off,out,0,0);
    return out;
  }
  var letters={B:['110','101','110','101','110'],E:['111','100','110','100','111'],F:['111','100','110','100','100'],
    O:['111','101','101','101','111'],R:['110','101','110','101','101'],A:['010','101','111','101','101'],T:['111','010','010','010','010']};
  function label(out,text,x,y) {
    for(var i=0;i<text.length;i++) for(var row=0;row<5;row++) for(var col=0;col<3;col++)
      if(letters[text[i]][row][col]==='1') put(out,x+i*4+col,y+row,colour('efe0b7'));
  }
  for(var frame=0;frame<75;frame++) {
    var time=frame*0.02,oldPose=time<1.2?Math.min(2,Math.floor((time+0.00001)/0.4)):Math.min(5,3+Math.floor((time-1.2+0.00001)/0.1));
    var remaining=1.2-time,newPose=remaining>0.22?0:remaining>0.088?1:2;
    if(time>=1.2) {
      var age=time-1.2,boundary=0,times=[0.07,0.04,0.07,0.05,0.07];newPose=7;
      for(var p=0;p<times.length;p++) {boundary+=times[p];if(age+0.00001<boundary){newPose=3+p;break;}}
    }
    var board=background(384,224),facings=[0,2,3,7];
    for(var group=0;group<4;group++) {
      var x=(group%2)*192,y=Math.floor(group/2)*112;
      label(board,'BEFORE',x+36,y+4);label(board,'AFTER',x+134,y+4);
      blit(oldCombined(facings[group],oldPose),board,x,y+16);
      blit(combined(facings[group],newPose+8,0,true),board,x+96,y+16);
    }
    save(scale(board,3),cfg.comparison.fixture,cfg.comparison.frames[frame]);
  }
}
