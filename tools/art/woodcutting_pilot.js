// Executed in LibreSprite after the cast pilot's common pixel helpers.
var directions = ['s', 'w', 'e', 'n', 'sw', 'se', 'nw', 'ne'];
var vectors = [[0,1],[-1,0],[1,0],[0,-1],[-0.707,0.707],[0.707,0.707],[-0.707,-0.707],[0.707,-0.707]];
function line(out, a, b, hex, width) {
  var dx = b[0] - a[0], dy = b[1] - a[1], steps = Math.max(Math.abs(dx), Math.abs(dy), 1);
  var radius = Math.floor(width / 2), c = colour(hex);
  for (var i = 0; i <= steps; i++) for (var y = -radius; y <= radius; y++) for (var x = -radius; x <= radius; x++)
    put(out, Math.round(a[0] + dx * i / steps) + x, Math.round(a[1] + dy * i / steps) + y, c);
}
function polygon(out, points, hex) {
  var c = colour(hex);
  for (var y = 0; y < out.height; y++) for (var x = 0; x < out.width; x++) {
    var inside = false;
    for (var i = 0, j = points.length - 1; i < points.length; j = i++) {
      var a = points[i], b = points[j];
      if ((a[1] > y) !== (b[1] > y) && x < (b[0] - a[0]) * (y - a[1]) / (b[1] - a[1]) + a[0]) inside = !inside;
    }
    if (inside) put(out, x, y, c);
  }
}
function geometry(direction, pose) {
  var v = vectors[direction], dx = v[0], dy = v[1];
  var grips = [[32+dx*5,38+dy*4],[32-dx*2,27],[32-dx*5,24],
    [32+dx*11,38+dy*10],[32+dx*13,39+dy*12],[32+dx*8,34+dy*8]];
  var heads = [[32+dx*17,26+dy*9],[32-dx*8,12],[32-dx*12,9],
    [32+dx*24,38+dy*24],[32+dx*26,39+dy*26],[32+dx*18,25+dy*18]];
  return {grip:grips[pose], head:heads[pose]};
}
function arm(out, shoulder, grip, near) {
  var elbow = [(shoulder[0]+grip[0])/2 + (near ? 2 : -2), (shoulder[1]+grip[1])/2 + 2];
  line(out, shoulder, elbow, '1f3345', 5);
  line(out, elbow, grip, '30211d', 3);
  line(out, shoulder, elbow, near ? '37617d' : '294b62', 3);
  line(out, elbow, grip, near ? 'd99d69' : '935b3d', 1);
}
function bodyPose(base, direction, pose) {
  var out = canvas(64,80), g = geometry(direction,pose), bob = pose === 3 || pose === 4 ? 1 : 0;
  arm(out,[27,35],g.grip,false);
  for (var y = 0; y < 48; y++) for (var x = 0; x < 32; x++) {
    var c = base.pixels[y*32+x], rgb = pc.rgbaR(c)*65536+pc.rgbaG(c)*256+pc.rgbaB(c);
    if (y >= 23 && y <= 31 && [0x935b3d,0xbe8054,0xd99d69,0xedbd89].indexOf(rgb) !== -1) continue;
    if (pc.rgbaA(c)) put(out,x+16,y+12+(y<26?bob:0),c);
  }
  // Retain the pelvis and feet while the shoulders and both arms work the tool.
  arm(out,[36,35+bob],g.grip,true);
  line(out,[g.grip[0]-1,g.grip[1]],[g.grip[0]+1,g.grip[1]],'edbd89',1);
  return out;
}
function axePose(direction,pose,tier) {
  var out = canvas(64,80), g = geometry(direction,pose), dx = g.head[0]-g.grip[0], dy = g.head[1]-g.grip[1];
  var length = Math.sqrt(dx*dx+dy*dy), u = [dx/length,dy/length], v = [-dy/length,dx/length];
  line(out,[g.grip[0]-u[0]*3,g.grip[1]-u[1]*3],g.head,'30211d',3);
  line(out,g.grip,g.head,'b27c48',1);
  function point(a,b) { return [g.head[0]+u[0]*a+v[0]*b,g.head[1]+u[1]*a+v[1]*b]; }
  polygon(out,[point(-4,-2),point(3,-2),point(5,4),point(-5,6)],'222b2b');
  var metal = [['84622c','b27c48','e3c073'],['404542','555d59','a69a7e'],['37617d','8da39c','efe0b7']][tier];
  polygon(out,[point(-3,-1),point(2,-1),point(4,3),point(-4,5)],metal[0]);
  polygon(out,[point(-2,0),point(2,0),point(3,3),point(-3,4)],metal[1]);
  line(out,point(-3,4),point(3,3),metal[2],1);
  return out;
}
var bodies, axes;
if (cfg.regenerate) {
  var player = read(cfg.player);
  if (player.width !== 128 || player.height !== 384) throw new Error('Woodcutting requires the eight-way player master');
  bodies = canvas(384,640); axes = canvas(384,1920);
  for (var d = 0; d < 8; d++) for (var p = 0; p < 6; p++) {
    blit(bodyPose(crop(player,0,d*48,32,48),d,p),bodies,p*64,d*80);
    for (var tier = 0; tier < 3; tier++) blit(axePose(d,p,tier),axes,p*64,(tier*8+d)*80);
  }
} else { bodies = read(cfg.sources.player); axes = read(cfg.sources.axes); }
if (bodies.width !== 384 || bodies.height !== 640 || axes.width !== 384 || axes.height !== 1920) throw new Error('Incorrect woodcutting master geometry');
[bodies,axes].forEach(function(image) { image.pixels.forEach(function(c) {
  if (pc.rgbaA(c) !== 0 && pc.rgbaA(c) !== 255) throw new Error('Woodcutting requires binary alpha');
}); });
var report = {bodyFrames:48,axeFrames:144,failedFrames:0};
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
  if (!lower || detached) throw new Error('Detached woodcutting legs: '+directions[d]+' pose '+p);
  for (var tier = 0; tier < 3; tier++) {
    var tool = crop(axes,p*64,(tier*8+d)*80,64,80), pixels = 0;
    tool.pixels.forEach(function(c) { if (pc.rgbaA(c)) pixels++; });
    if (!pixels) throw new Error('Missing axe cell');
    for (var y = 0; y < 80; y++) for (var x = 0; x < 64; x++)
      if ((x===0||x===63||y===0||y===79) && pc.rgbaA(tool.pixels[y*64+x])) throw new Error('Clipped axe cell');
  }
}
if (cfg.regenerate) {
  save(bodies,cfg.fixtures.player,cfg.sources.player); save(axes,cfg.fixtures.axes,cfg.sources.axes);
}
function combined(d,p,tier) {
  var out = canvas(64,80), body = crop(bodies,p*64,d*80,64,80), tool = crop(axes,p*64,(tier*8+d)*80,64,80);
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
console.log('WOODCUTTING_VALIDATION_JSON='+JSON.stringify(report));
console.log('WOODCUTTING_PILOT_OK: eight facings, six action poses, three axe tiers; connected legs and unclipped tools.');
