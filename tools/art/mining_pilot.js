// Shared drawing helpers and body authoring come from the corrected axe pilot.
// Lower the contact/return grip to meet a rock rather than a tree trunk.
var inheritedGeometry = geometry;
geometry = function(direction,pose) {
  var g = inheritedGeometry(direction,pose);
  var lower = pose === 3 || pose === 4 ? 8 : pose === 5 ? 4 : pose === 0 ? 2 : 0;
  g.grip[1] += lower; g.head[1] += lower;
  return g;
};

function pickaxePose(direction,pose,tier) {
  var out = canvas(64,80), g = geometry(direction,pose);
  var dx = g.head[0]-g.grip[0], dy = g.head[1]-g.grip[1], length = Math.sqrt(dx*dx+dy*dy);
  var u = [dx/length,dy/length], v = [-dy/length,dx/length], cut = vectors[direction];
  var toward = v[0]*cut[0]+v[1]*cut[1];
  if (toward < -0.001 || (Math.abs(toward) <= 0.001 && (cut[0] < 0 || (cut[0] === 0 && cut[1] < 0))))
    v = [-v[0],-v[1]];
  function point(a,b) { return [g.head[0]+u[0]*a+v[0]*b,g.head[1]+u[1]*a+v[1]*b]; }
  line(out,[g.grip[0]-u[0]*3,g.grip[1]-u[1]*3],g.head,'30211d',3);
  line(out,g.grip,g.head,'b27c48',1);
  // Curved, tapered cross-head: the leading point faces the selected rock.
  polygon(out,[point(-1,-10),point(2,-8),point(5,-2),point(5,2),point(2,8),
    point(-1,11),point(-2,7),point(0,2),point(0,-2),point(-2,-7)],'222b2b');
  var metal = [['84622c','b27c48','e3c073'],['404542','555d59','a69a7e'],['37617d','8da39c','efe0b7']][tier];
  polygon(out,[point(-1,-8),point(1,-7),point(4,-2),point(4,2),point(1,7),
    point(-1,9),point(-1,6),point(1,1),point(1,-1),point(-1,-6)],metal[0]);
  line(out,point(2,-5),point(3,0),metal[1],1);
  line(out,point(3,0),point(1,6),metal[1],1);
  line(out,point(1,6),point(-1,10),metal[2],1);
  return out;
}

var bodies, pickaxes;
if (cfg.regenerate) {
  var player = read(cfg.player);
  if (player.width !== 128 || player.height !== 384) throw new Error('Mining requires the eight-way player master');
  bodies = canvas(384,640); pickaxes = canvas(384,1920);
  for (var d = 0; d < 8; d++) for (var p = 0; p < 6; p++) {
    blit(bodyPose(crop(player,0,d*48,32,48),d,p),bodies,p*64,d*80);
    for (var tier = 0; tier < 3; tier++) blit(pickaxePose(d,p,tier),pickaxes,p*64,(tier*8+d)*80);
  }
} else { bodies = read(cfg.sources.player); pickaxes = read(cfg.sources.pickaxes); }
if (cfg.rebuildPickaxes) {
  pickaxes = canvas(384,1920);
  for (var d = 0; d < 8; d++) for (var p = 0; p < 6; p++)
    for (var tier = 0; tier < 3; tier++) blit(pickaxePose(d,p,tier),pickaxes,p*64,(tier*8+d)*80);
}
if (bodies.width !== 384 || bodies.height !== 640 || pickaxes.width !== 384 || pickaxes.height !== 1920) throw new Error('Incorrect mining master geometry');
[bodies,pickaxes].forEach(function(image) { image.pixels.forEach(function(c) {
  if (pc.rgbaA(c) !== 0 && pc.rgbaA(c) !== 255) throw new Error('Mining requires binary alpha');
}); });
var report = {bodyFrames:48,pickaxeFrames:144,failedFrames:0};
for (var d = 0; d < 8; d++) for (var p = 0; p < 6; p++) {
  var frame = crop(bodies,p*64,d*80,64,80), seen = new Array(5120).fill(false), sizes = [], labels = new Array(5120).fill(-1);
  for (var seed = 0; seed < 5120; seed++) {
    if (seen[seed] || !pc.rgbaA(frame.pixels[seed])) continue;
    var queue = [seed], count = 0, label = sizes.length; seen[seed] = true;
    for (var i = 0; i < queue.length; i++) {
      var index = queue[i], x = index%64, y = Math.floor(index/64); labels[index] = label; count++;
      for (var dy = -1; dy <= 1; dy++) for (var dx = -1; dx <= 1; dx++) {
        var nx = x+dx, ny = y+dy, n = ny*64+nx;
        if (nx>=0 && nx<64 && ny>=0 && ny<80 && !seen[n] && pc.rgbaA(frame.pixels[n])) { seen[n]=true; queue.push(n); }
      }
    }
    sizes.push(count);
  }
  var main = sizes.indexOf(Math.max.apply(null,sizes)), lower = 0, detached = 0;
  for (var y = 41; y < 52; y++) for (var x = 16; x < 48; x++) {
    var index = y*64+x;
    if (pc.rgbaA(frame.pixels[index])) { lower++; if (labels[index] !== main) detached++; }
  }
  if (!lower || detached) throw new Error('Detached mining legs: '+directions[d]+' pose '+p);
  for (var tier = 0; tier < 3; tier++) {
    var tool = crop(pickaxes,p*64,(tier*8+d)*80,64,80), pixels = 0;
    tool.pixels.forEach(function(c) { if (pc.rgbaA(c)) pixels++; });
    if (!pixels) throw new Error('Missing axe cell');
    for (var y = 0; y < 80; y++) for (var x = 0; x < 64; x++)
      if ((x===0||x===63||y===0||y===79) && pc.rgbaA(tool.pixels[y*64+x])) throw new Error('Clipped axe cell');
  }
}
if (cfg.regenerate) {
  save(bodies,cfg.fixtures.player,cfg.sources.player);
}
if (cfg.regenerate || cfg.rebuildPickaxes) save(pickaxes,cfg.fixtures.pickaxes,cfg.sources.pickaxes);
function combined(d,p,tier) {
  var out = canvas(64,80), body = crop(bodies,p*64,d*80,64,80), tool = crop(pickaxes,p*64,(tier*8+d)*80,64,80);
  var behind = d===3||d===6||d===7;
  blit(behind?tool:body,out,0,0); blit(behind?body:tool,out,0,0);
  return out;
}
function background(w,h) {
  var out = canvas(w,h);
  for (var y = 0; y < h; y++) for (var x = 0; x < w; x++) out.pixels[y*w+x] = ((x>>3)+(y>>3))%2 ? colour('748057') : colour('aaad8b');
  return out;
}
for (var p = 0; p < 6; p++) {
  var board = background(256,160);
  for (var d = 0; d < 8; d++) blit(combined(d,p,0),board,d%4*64,Math.floor(d/4)*80);
  save(scale(board,3),cfg.fixtures.board,cfg.frames[p]);
}
var contact = background(384,640);
for (var d = 0; d < 8; d++) for (var p = 0; p < 6; p++) blit(combined(d,p,0),contact,p*64,d*80);
save(scale(contact,2),cfg.fixtures.contact,cfg.contact);
var tiers = background(192,80);
for (var tier = 0; tier < 3; tier++) blit(combined(2,3,tier),tiers,tier*64,0);
save(scale(tiers,4),cfg.fixtures.tiers,cfg.tiers);
console.log('MINING_VALIDATION_JSON='+JSON.stringify(report));
console.log('MINING_PILOT_OK: eight facings, six action poses, three pickaxe tiers; connected legs and unclipped tools.');
