// Read-only geometry audit. Run from the repository root with Node.js.
const fs=require('fs'),vm=require('vm'),cp=require('child_process');
const baseline=process.argv[2]||'16f9697';
const old=cp.execFileSync('git',['show',baseline+':tools/art/melee_pilot.js'],{encoding:'utf8'});
const before={};vm.runInNewContext(old.split('function arm(')[0],before);
let oldReach=0;
for(let d=0;d<8;d++)for(let p=0;p<10;p++)for(let style=0;style<3;style++){
  const g=before.geometry(d,p,style),r=g.right;
  const a=[48+r[0]*5+g.lean[0],46+r[1]*3+g.lean[1]],b=[48-r[0]*5+g.lean[0],46-r[1]*3+g.lean[1]];
  oldReach=Math.max(oldReach,Math.hypot(g.grip[0]-a[0],g.grip[1]-a[1]),Math.hypot(g.off[0]-b[0],g.off[1]-b[1]));
}
const after={};vm.runInNewContext(fs.readFileSync('tools/art/player_rig.js','utf8')+'\n'+
  fs.readFileSync('tools/art/melee_pilot.js','utf8').split('function bodyPose(')[0],after);
let newReach=0,transitions=0;
for(let d=0;d<8;d++)for(let p=0;p<16;p++)for(let style=0;style<5;style++){
  const g=after.geometry(d,p,style),j=g.joints;
  newReach=Math.max(newReach,Math.hypot(j.grip[0]-j.main[0],j.grip[1]-j.main[1]),Math.hypot(j.off[0]-j.other[0],j.off[1]-j.other[1]));
  if(p===15){
    const idle=after.geometry(d,0,style);
    for(const key of ['grip','off','aim','lean'])for(let axis=0;axis<2;axis++)
      if(Math.abs(idle[key][axis]-g[key][axis])>0.0001) throw new Error('Discontinuous recovery: '+[style,d,key,axis]);
    transitions++;
  }
}
if(newReach>9.50001) throw new Error('Arm reach exceeded the joint contract');
console.log(JSON.stringify({baseline,oldMaxShoulderToWrist:oldReach,newMaxShoulderToWrist:newReach,
  matchingIdleTransitions:transitions,units:'native pixels before rasterisation'},null,2));
