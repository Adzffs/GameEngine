// LibreSprite draws the final pixels, exports editable masters and builds the review board.
var cfg = __CONFIG__, pc = app.pixelColor;
function colour(hex) { return pc.rgba(parseInt(hex.slice(0, 2), 16), parseInt(hex.slice(2, 4), 16), parseInt(hex.slice(4, 6), 16), 255); }
var palette = cfg.palette.map(colour);
function canvas(w, h) { return {width:w, height:h, pixels:new Array(w * h).fill(0)}; }
function put(out, x, y, c) { if (x >= 0 && y >= 0 && x < out.width && y < out.height) out.pixels[y * out.width + x] = c; }
function read(path) {
  var s = app.open(path).sprite, layer = s.layer(0), cel = layer.cel(0), img = cel.image;
  if (s.layerCount !== 1 || layer.celCount !== 1) throw new Error('Environment masters require one image layer and one frame');
  var out = canvas(s.width, s.height);
  for (var y = 0; y < img.height; y++) for (var x = 0; x < img.width; x++) put(out, cel.x + x, cel.y + y, img.getPixel(x, y));
  return out;
}
function save(image, fixture, path) {
  var s = app.open(fixture).sprite, img = s.layer(0).cel(0).image;
  if (image.width !== s.width || image.height !== s.height) throw new Error('Incorrect environment canvas');
  for (var y = 0; y < image.height; y++) for (var x = 0; x < image.width; x++) img.putPixel(x, y, image.pixels[y * image.width + x]);
  s.commit(); s.saveAs(path, false);
}
function blit(src, dst, dx, dy, sx, sy, w, h) {
  sx = sx || 0; sy = sy || 0; w = w || src.width; h = h || src.height;
  for (var y = 0; y < h; y++) for (var x = 0; x < w; x++) {
    var c = src.pixels[(sy + y) * src.width + sx + x];
    if (pc.rgbaA(c)) put(dst, dx + x, dy + y, c);
  }
}
function nearest(c) {
  var best = palette[0], distance = Infinity;
  palette.forEach(function(p) {
    var dr = pc.rgbaR(c) - pc.rgbaR(p), dg = pc.rgbaG(c) - pc.rgbaG(p), db = pc.rgbaB(c) - pc.rgbaB(p);
    var d = dr * dr + dg * dg + db * db;
    if (d < distance) { distance = d; best = p; }
  });
  return best;
}
function hash(x, y, seed) { return ((x * 73856093 ^ y * 19349663 ^ seed * 83492791) >>> 0); }
function water(variant) {
  var out = canvas(32, 32), base = colour('34585a'), shade = colour('315354'), light = colour('3e6665'), glint = colour('507773');
  for (var y = 0; y < 32; y++) for (var x = 0; x < 32; x++) {
    var c = (Math.floor((x + y * 2) / 8) % 4 === 0) ? shade : base;
    // The outer two pixels are invariant, so every variant meets every other variant.
    if (x > 2 && x < 28 && y > 2 && y < 28) {
      if ((y === 8 + variant * 3 && x >= 5 && x < 11) || (y === 23 - variant * 2 && x >= 19 && x < 25)) c = light;
      if (y === 9 + variant * 3 && x >= 7 && x < 9) c = glint;
    }
    put(out, x, y, c);
  }
  return out;
}
function stone(variant) {
  var out = canvas(32, 32), colours = ['5c6459','60675b','646b5e'].map(colour);
  for (var y = 0; y < 32; y++) for (var x = 0; x < 32; x++) {
    var row = Math.floor(y / 8), py = y % 8, px = (x + (row % 2) * 8) % 16;
    var block = Math.floor((x + (row % 2) * 8) / 16), c = colours[(row + block) % 3];
    if (py === 0 || px === 0) c = colour('414c43');
    else if (py === 1 || px === 1) c = colour('7a806c');
    else if (py === 7 || px === 15) c = colour('505b4d');
    else if (x > 2 && x < 29 && y > 2 && y < 29 && hash(x, y, variant) % 127 === 0) c = colour('727967');
    put(out, x, y, c);
  }
  return out;
}
var edges = ['n','e','s','w','outer-nw','outer-ne','outer-sw','outer-se','inner-nw','inner-ne','inner-sw','inner-se'];
function bank(family, edge) {
  var out = canvas(32, 32);
  for (var y = 0; y < 32; y++) for (var x = 0; x < 32; x++) {
    var n = y, s = 31 - y, w = x, e = 31 - x, d;
    if (edge === 'n') d = n; else if (edge === 's') d = s;
    else if (edge === 'w') d = w; else if (edge === 'e') d = e;
    else {
      var corner = edge.slice(-2);
      var a = corner.indexOf('n') >= 0 ? n : s, b = corner.indexOf('w') >= 0 ? w : e;
      d = edge.indexOf('inner') === 0 ? (a + b) * 0.8 : Math.min(a, b);
    }
    // Slight step in the cut edge, repeated every tile; no transparent gaps at joins.
    var along = (edge === 'n' || edge === 's') ? x : y;
    var lip = (along % 16 >= 6 && along % 16 <= 9) ? 1 : 0;
    if (d > 6 + lip) continue;
    var c;
    if (family === 'stone') {
      c = d < 1 ? colour('7a806c') : d < 3 ? colour('5c6459') : d < 5 ? colour('3b4943') : colour('3d6460');
    } else if (family === 'dirt') {
      c = d < 2 + lip ? colour('ad8b52') : d < 4 + lip ? colour('79603e') : d < 6 + lip ? colour('354538') : colour('3d6460');
    } else {
      c = d < 1 ? colour('78804a') : d < 2 + lip ? colour('596b36') : d < 4 + lip ? colour('625039') : d < 6 + lip ? colour('354538') : colour('3d6460');
    }
    put(out, x, y, c);
  }
  return out;
}
var terrain, copper, tin, rubble;
if (cfg.regenerate) {
  terrain = canvas(256, 192);
  for (var i = 0; i < 42; i++) {
    var tile = i < 3 ? water(i) : i < 6 ? stone(i - 3) : bank(i < 18 ? 'earth' : i < 30 ? 'stone' : 'dirt', edges[(i - 6) % 12]);
    blit(tile, terrain, (i % 8) * 32, Math.floor(i / 8) * 32);
  }
  var ref = read(cfg.reference), boxes = [];
  for (var index = 0; index < 3; index++) {
    var x0 = Math.floor(index * ref.width / 3), x1 = Math.floor((index + 1) * ref.width / 3);
    var b = {x0:x1,y0:ref.height,x1:x0,y1:0};
    for (var y = 0; y < ref.height; y++) for (var x = x0; x < x1; x++) {
      if (pc.rgbaA(ref.pixels[y * ref.width + x]) < 192) continue;
      b.x0 = Math.min(b.x0, x); b.y0 = Math.min(b.y0, y); b.x1 = Math.max(b.x1, x + 1); b.y1 = Math.max(b.y1, y + 1);
    }
    if (b.x1 <= b.x0) throw new Error('Empty mining reference panel');
    boxes.push(b);
  }
  // One scale across the three panels preserves the intended low depleted silhouette.
  var factor = Math.min(30 / Math.max.apply(null, boxes.map(function(b) { return b.x1 - b.x0; })),
                        26 / Math.max.apply(null, boxes.map(function(b) { return b.y1 - b.y0; })));
  function rock(index) {
    var b = boxes[index], width = Math.round((b.x1 - b.x0) * factor), height = Math.round((b.y1 - b.y0) * factor);
    var out = canvas(32, 40), left = Math.floor((32 - width) / 2), top = 32 - height;
    for (var y = 0; y < height; y++) for (var x = 0; x < width; x++) {
      var sx = Math.min(b.x1 - 1, b.x0 + Math.floor((x + 0.5) / factor));
      var sy = Math.min(b.y1 - 1, b.y0 + Math.floor((y + 0.5) / factor));
      var c = ref.pixels[sy * ref.width + sx];
      if (pc.rgbaA(c) >= 192) put(out, left + x, top + y, nearest(c));
    }
    return out;
  }
  copper = rock(0); tin = rock(1); rubble = rock(2);
  save(terrain, cfg.fixtures.terrain, cfg.sources.terrain);
  save(copper, cfg.fixtures.copper, cfg.sources.copper);
  save(tin, cfg.fixtures.tin, cfg.sources.tin);
  save(rubble, cfg.fixtures.rubble, cfg.sources.rubble);
} else {
  terrain = read(cfg.sources.terrain); copper = read(cfg.sources.copper); tin = read(cfg.sources.tin); rubble = read(cfg.sources.rubble);
}
if (terrain.width !== 256 || terrain.height !== 192) throw new Error('Terrain master geometry changed');
[copper,tin,rubble].forEach(function(image) { if (image.width !== 32 || image.height !== 40) throw new Error('Rock master geometry changed'); });
[terrain,copper,tin,rubble].forEach(function(image) {
  image.pixels.forEach(function(c) { if (pc.rgbaA(c) !== 0 && pc.rgbaA(c) !== 255) throw new Error('Environment sprites require binary alpha'); });
});
var grass = read(cfg.forest), board = canvas(384,192);
for (var y = 0; y < board.height; y++) for (var x = 0; x < board.width; x++)
  board.pixels[y * board.width + x] = grass.pixels[(y % 32) * grass.width + x % 32];
function frame(index, dx, dy, qx, qy) {
  blit(terrain, board, dx, dy, index % 8 * 32 + (qx || 0), Math.floor(index / 8) * 32 + (qy || 0), qx === undefined ? 32 : 16, qy === undefined ? 32 : 16);
}
// Same quarter selection as runtime: water, straight banks, both kinds of corners and stone crossing ends.
for (var ty = 0; ty < 6; ty++) for (var tx = 3; tx < 7; tx++) {
  frame((tx + ty) % 3, tx * 32, ty * 32);
  if (tx === 3) frame(9, tx * 32, ty * 32);
  if (tx === 6) frame(7, tx * 32, ty * 32);
}
for (var tx = 2; tx < 8; tx++) frame(3 + tx % 3, tx * 32, 64);
for (var tx = 3; tx < 7; tx++) { frame(20, tx * 32, 32); frame(18, tx * 32, 96); }
for (var ty = 0; ty < 6; ty++) for (var tx = 8; tx < 12; tx++) frame(3 + (tx + ty) % 3, tx * 32, ty * 32);
blit(copper, board, 268, 12); blit(tin, board, 324, 12); blit(rubble, board, 300, 100);
// Corner contact sheet at left: four outside and four inside corners.
for (var i = 0; i < 8; i++) {
  var dx = (i % 2) * 32, dy = Math.floor(i / 2) * 32;
  frame(i % 3, dx, dy); frame(10 + i, dx, dy);
}
var large = canvas(768,384);
for (var y = 0; y < large.height; y++) for (var x = 0; x < large.width; x++)
  large.pixels[y * large.width + x] = board.pixels[Math.floor(y / 2) * board.width + Math.floor(x / 2)];
save(large, cfg.fixtures.board, cfg.preview);
console.log('ENVIRONMENT_PILOT_OK: 42 terrain cells; copper, tin and shared depleted rubble; binary alpha.');
