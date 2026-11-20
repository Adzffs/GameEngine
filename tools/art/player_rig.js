// Small, integer-pixel posing rig. Outfit pixels stay authored; limbs have
// bounded upper/lower segments and every equipment grip uses the same joints.
var rigDirections=[[0,1],[-1,0],[1,0],[0,-1],[-0.70710678,0.70710678],[0.70710678,0.70710678],[-0.70710678,-0.70710678],[0.70710678,-0.70710678]];
function rigLine(out,a,b,hex,width) {
  var dx=b[0]-a[0],dy=b[1]-a[1],n=Math.max(Math.abs(dx),Math.abs(dy),1),r=Math.floor(width/2),c=colour(hex);
  for(var i=0;i<=n;i++) for(var y=-r;y<=r;y++) for(var x=-r;x<=r;x++)
    put(out,Math.round(a[0]+dx*i/n)+x,Math.round(a[1]+dy*i/n)+y,c);
}
function rigUnit(v) { var n=Math.max(0.001,Math.sqrt(v[0]*v[0]+v[1]*v[1])); return [v[0]/n,v[1]/n]; }
function rigReach(shoulder,desired) {
  var dx=desired[0]-shoulder[0],dy=desired[1]-shoulder[1],n=Math.sqrt(dx*dx+dy*dy),scale=n>9.5?9.5/n:1;
  return [shoulder[0]+dx*scale,shoulder[1]+dy*scale];
}
function rigArm(out,shoulder,hand,near,bend) {
  var dx=hand[0]-shoulder[0],dy=hand[1]-shoulder[1],n=Math.max(0.1,Math.sqrt(dx*dx+dy*dy));
  // Foreshorten folded arms in the top-down projection; a deep sideways
  // elbow triangle made the relaxed walk read as a permanent fighting stance.
  var height=Math.min(2.0,Math.sqrt(Math.max(0,25-n*n/4)));
  var elbow=[(shoulder[0]+hand[0])/2-dy/n*height*bend,(shoulder[1]+hand[1])/2+dx/n*height*bend];
  rigLine(out,shoulder,elbow,'1f3345',3); rigLine(out,elbow,hand,'30211d',3);
  rigLine(out,shoulder,elbow,near?'37617d':'294b62',1);
  rigLine(out,elbow,hand,near?'d99d69':'935b3d',1);
  put(out,Math.round(hand[0]),Math.round(hand[1]),colour('edbd89'));
}
function rigJoints(d,lean,bob,desired,offDesired) {
  var v=rigDirections[d],r=[-v[1],v[0]];
  var main=[48+r[0]*4+lean[0],47+r[1]*2+bob+lean[1]],other=[48-r[0]*4+lean[0],47-r[1]*2+bob+lean[1]];
  return {main:main,other:other,grip:rigReach(main,desired),off:rigReach(other,offDesired),right:r};
}
function rigBody(base,d,lean,bob,joints) {
  var out=canvas(96,96),behind=d===1||d===3||d===6;
  var v=rigDirections[d],bend=v[0]<0?-1:1;
  rigArm(out,behind?joints.main:joints.other,behind?joints.grip:joints.off,false,-bend);
  // Keep the belt/pelvis intact; taper the lunge through the torso instead of
  // moving a cut-out head and shirt as an unconnected rectangle.
  for(var y=0;y<48;y++) for(var x=0;x<32;x++) {
    var c=base.pixels[y*32+x],rgb=pc.rgbaR(c)*65536+pc.rgbaG(c)*256+pc.rgbaB(c);
    if(!pc.rgbaA(c)) continue;
    if(y>=22&&y<=29&&([0x935b3d,0xbe8054,0xd99d69,0xedbd89].indexOf(rgb)>=0||x<12||x>19)) continue;
    var weight=y<21?0.65:y<29?1:y<33?0.35:0;
    var tx=x+32+Math.round(lean[0]*weight),ty=y+24+Math.round((bob+lean[1])*weight);
    put(out,tx,ty,c);
  }
  rigArm(out,behind?joints.other:joints.main,behind?joints.off:joints.grip,true,bend);
  return out;
}
function rigWalk(base,d,phase) {
  if(phase===0) return crop(base,0,0,32,48);
  var v=rigDirections[d],r=[-v[1],v[0]],t=phase*Math.PI/4;
  var bob=[0,-1,0,1,0,-1,0,1][phase],swing=Math.sin(t)*2;
  var joints=rigJoints(d,[0,0],bob,[48+r[0]*5+v[0]*swing,53+r[1]*3+bob+v[1]*swing*0.5],
    [48-r[0]*5-v[0]*swing,53-r[1]*3+bob-v[1]*swing*0.5]);
  var out=rigBody(base,d,[0,0],bob,joints);
  for(var y=55;y<70;y++) for(var x=32;x<64;x++) out.pixels[y*96+x]=0;
  // One far leg and one near leg, rather than duplicated whole lower bodies.
  var nearSign=r[1]>=0?1:-1;
  for(var layer=0;layer<2;layer++) {
    var side=layer===0?-nearSign:nearSign,a=t+(side<0?Math.PI:0),stride=Math.sin(a)*3;
    var lift=Math.max(0,Math.cos(a))*1.8;
    var hip=[48+r[0]*side*2,55+r[1]*side+bob*0.3];
    var ankle=[48+r[0]*side*3+v[0]*stride,62+r[1]*side*1.7+v[1]*stride*0.55-lift];
    var knee=[(hip[0]+ankle[0])/2+v[0]*Math.max(0,Math.cos(a)),(hip[1]+ankle[1])/2];
    rigLine(out,hip,knee,'30211d',3); rigLine(out,knee,ankle,'30211d',3);
    rigLine(out,hip,knee,layer===1?'7b4e2e':'563727',1);
    rigLine(out,knee,ankle,layer===1?'935b3d':'7b4e2e',1);
    rigLine(out,[ankle[0]-1,ankle[1]+1],[ankle[0]+1+v[0],ankle[1]+1+Math.max(0,v[1])],'30211d',3);
    rigLine(out,ankle,[ankle[0]+v[0],ankle[1]+1],'7b4e2e',1);
  }
  // The retained waist overlaps both articulated thighs in every phase.
  for(var y=28;y<=31;y++) for(var x=0;x<32;x++) {
    var c=base.pixels[y*32+x]; if(pc.rgbaA(c)) put(out,x+32,y+24,c);
  }
  return crop(out,32,24,32,48);
}
