// Retained six-pose geometry for the existing mining masters.
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
  if (pose === 3 || pose === 4) {
    // Tilt the shaft across the cut so its blade, rather than its handle tip,
    // reaches the bark. Mirrored side views keep the head above the hands.
    var side = dx < 0 ? -1 : 1;
    heads[pose] = [grips[pose][0]+14*(0.6*dx+0.8*side*dy),
      grips[pose][1]+14*(0.6*dy-0.8*side*dx)];
  }
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
