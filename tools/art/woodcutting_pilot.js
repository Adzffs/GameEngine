// Eight authored chopping keys with shared bounded joints and fixed foot pivot.
var directions=['s','w','e','n','sw','se','nw','ne'],vectors=rigDirections;
function line(out,a,b,hex,width) { rigLine(out,a,b,hex,width); }
function polygon(out,points,hex) {
  var c=colour(hex);
  for(var y=0;y<out.height;y++) for(var x=0;x<out.width;x++) {
    var inside=false;
    for(var i=0,j=points.length-1;i<points.length;j=i++) {
      var a=points[i],b=points[j];
      if((a[1]>y)!==(b[1]>y)&&x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0]) inside=!inside;
    }
    if(inside) put(out,x,y,c);
  }
}
function geometry(d,p) {
  var v=vectors[d],r=[-v[1],v[0]],reach=[0,-1,-2,7,8,5,2,0][p];
  var leanAmount=[0,-1,-1,2,3,1,0,0][p],lean=[Math.round(v[0]*leanAmount),0];
  var desired=[48+r[0]+v[0]*reach,51+[0,-4,-7,0,1,-2,-1,0][p]+v[1]*reach*0.4];
  var aims=[[v[0]*0.4,-1],[-v[0]*0.5+r[0]*0.25,-1.1],[-v[0]*0.7+r[0]*0.3,-1.2],
    [0.6*v[0]+0.8*(v[0]<0?-1:1)*v[1],0.6*v[1]-0.8*(v[0]<0?-1:1)*v[0]],
    [0.6*v[0]+0.8*(v[0]<0?-1:1)*v[1],0.6*v[1]-0.8*(v[0]<0?-1:1)*v[0]],
    [v[0]*0.7,-0.8+v[1]*0.3],[v[0]*0.45,-1],[v[0]*0.4,-1]];
  var aim=rigUnit(aims[p]);
  // Rear diagonals need the head outside the face silhouette. Keep the shaft
  // across the cut and the blade leading, rather than hiding it behind hair.
  if(d>=4&&(p===3||p===4)) aim=rigUnit([v[0],-0.15]);
  var joints=rigJoints(d,lean,0,desired,[desired[0]-aim[0]*4,desired[1]-aim[1]*4]);
  return {grip:joints.grip,head:[joints.grip[0]+aim[0]*17,joints.grip[1]+aim[1]*17],aim:aim,joints:joints,lean:lean};
}
function bodyPose(base,d,p) {
  var g=geometry(d,p);
  return rigBody(base,d,g.lean,0,g.joints);
}
function axePose(d,p,tier) {
  var out=canvas(96,96),g=geometry(d,p),u=g.aim,v=[-u[1],u[0]],cut=vectors[d];
  var toward=v[0]*cut[0]+v[1]*cut[1];
  if(toward < -0.001 || (Math.abs(toward)<=0.001&&(cut[0]<0||(cut[0]===0&&cut[1]<0)))) v=[-v[0],-v[1]];
  function point(a,b) { return [g.head[0]+u[0]*a+v[0]*b,g.head[1]+u[1]*a+v[1]*b]; }
  line(out,[g.grip[0]-u[0]*4,g.grip[1]-u[1]*4],g.head,'30211d',3);
  line(out,g.grip,g.head,'b27c48',1);
  polygon(out,[point(-4,-2),point(3,-2),point(5,4),point(-5,6)],'222b2b');
  var metal=[['84622c','b27c48','e3c073'],['404542','555d59','a69a7e'],['37617d','8da39c','efe0b7']][tier];
  polygon(out,[point(-3,-1),point(2,-1),point(4,3),point(-4,5)],metal[0]);
  polygon(out,[point(-2,0),point(2,0),point(3,3),point(-3,4)],metal[1]);
  line(out,point(-3,4),point(3,3),metal[2],1);
  put(out,Math.round(g.grip[0]),Math.round(g.grip[1]),colour('edbd89'));
  return out;
}
var bodies,axes;
if(cfg.regenerate) {
  var player=read(cfg.player);
  if(player.width!==256||player.height!==384) throw new Error('Chopping requires the eight-pose walk master');
  bodies=canvas(768,768); axes=canvas(768,2304);
  for(var d=0;d<8;d++) for(var p=0;p<8;p++) {
    blit(bodyPose(crop(player,0,d*48,32,48),d,p),bodies,p*96,d*96);
    for(var tier=0;tier<3;tier++) blit(axePose(d,p,tier),axes,p*96,(tier*8+d)*96);
  }
} else { bodies=read(cfg.sources.player); axes=read(cfg.sources.axes); }
if(cfg.rebuildAxes) {
  axes=canvas(768,2304);
  for(var d=0;d<8;d++) for(var p=0;p<8;p++) for(var tier=0;tier<3;tier++)
    blit(axePose(d,p,tier),axes,p*96,(tier*8+d)*96);
}
if(bodies.width!==768||bodies.height!==768||axes.width!==768||axes.height!==2304) throw new Error('Incorrect chopping master geometry; regenerate to migrate six-pose art');
var report={bodyFrames:64,axeFrames:192,failedFrames:0,maxArmReach:9.5};
[bodies,axes].forEach(function(atlas) {
  for(var cy=0;cy<atlas.height;cy+=96) for(var cx=0;cx<atlas.width;cx+=96) {
    var ink=0;
    for(var y=0;y<96;y++) for(var x=0;x<96;x++) {
      var alpha=pc.rgbaA(atlas.pixels[(cy+y)*atlas.width+cx+x]);
      if(alpha!==0&&alpha!==255) throw new Error('Chopping requires binary alpha');
      if(alpha) { ink++; if(x===0||x===95||y===0||y===95) throw new Error('Clipped chopping cell'); }
    }
    if(!ink) throw new Error('Empty chopping cell');
  }
});
for(var d=0;d<8;d++) for(var p=0;p<8;p++) {
  var frame=crop(bodies,p*96,d*96,96,96),seen=new Array(9216).fill(false),sizes=[],labels=new Array(9216).fill(-1);
  for(var seed=0;seed<9216;seed++) {
    if(seen[seed]||!pc.rgbaA(frame.pixels[seed])) continue;
    var queue=[seed],label=sizes.length,count=0; seen[seed]=true;
    for(var i=0;i<queue.length;i++) {
      var index=queue[i],x=index%96,y=Math.floor(index/96); labels[index]=label;count++;
      for(var dy=-1;dy<=1;dy++) for(var dx=-1;dx<=1;dx++) {
        var nx=x+dx,ny=y+dy,n=ny*96+nx;
        if(nx>=0&&nx<96&&ny>=0&&ny<96&&!seen[n]&&pc.rgbaA(frame.pixels[n])) {seen[n]=true;queue.push(n);}
      }
    }
    sizes.push(count);
  }
  var main=sizes.indexOf(Math.max.apply(null,sizes)),lower=0,detached=0;
  for(var y=55;y<65;y++) for(var x=32;x<64;x++) if(pc.rgbaA(frame.pixels[y*96+x])) {
    lower++;if(labels[y*96+x]!==main) detached++;
  }
  if(!lower||detached) throw new Error('Detached chopping legs in direction '+d+' pose '+p);
}
if(cfg.regenerate) save(bodies,cfg.fixtures.player,cfg.sources.player);
if(cfg.regenerate||cfg.rebuildAxes) save(axes,cfg.fixtures.axes,cfg.sources.axes);
function combined(d,p,tier) {
  var out=canvas(96,96),body=crop(bodies,p*96,d*96,96,96),tool=crop(axes,p*96,(tier*8+d)*96,96,96);
  var behind=d===3||d===6||d===7;
  blit(behind?tool:body,out,0,0);blit(behind?body:tool,out,0,0);return out;
}
function background(w,h) {
  var out=canvas(w,h);
  for(var y=0;y<h;y++) for(var x=0;x<w;x++) out.pixels[y*w+x]=((x>>3)+(y>>3))%2?colour('748057'):colour('aaad8b');
  return out;
}
for(var p=0;p<8;p++) {
  var board=background(384,192);
  for(var d=0;d<8;d++) blit(combined(d,p,0),board,d%4*96,Math.floor(d/4)*96);
  save(scale(board,3),cfg.fixtures.board,cfg.frames[p]);
}
var contact=background(768,768);
for(var d=0;d<8;d++) for(var p=0;p<8;p++) blit(combined(d,p,0),contact,p*96,d*96);
save(scale(contact,2),cfg.fixtures.contact,cfg.contact);
var tiers=background(288,96);
for(var tier=0;tier<3;tier++) blit(combined(2,3,tier),tiers,tier*96,0);
save(scale(tiers,3),cfg.fixtures.tiers,cfg.tiers);
console.log('WOODCUTTING_VALIDATION_JSON='+JSON.stringify(report));
console.log('WOODCUTTING_PILOT_OK: eight facings, eight timed poses, bounded arms, connected bodies and unclipped tools.');
