// LibreSprite 1.1-dev JavaScript API. The Python launcher supplies paths only.
// All artwork, resampling, palette conversion and preview drawing happen here.
var cfg = __CONFIG__;
var rgba = app.pixelColor;
function colour(hex) {
  return rgba.rgba(parseInt(hex.slice(0, 2), 16), parseInt(hex.slice(2, 4), 16),
                   parseInt(hex.slice(4, 6), 16), 255);
}
function colours(values) { return values.map(colour); }
var grass = colours(['485d39', '4b603a', '4f643d', '53683f', '586d43', '62764a', '718252']);
var earth = colours(['59452f', '715438', '87633f', '99764b', 'aa8957', 'ba9a65', 'c9ab77']);
var treePalette = colours(cfg.treePalette);
function hash(x, y, salt) {
  var n = Math.imul(x + 411, 374761393) ^ Math.imul(y + 97, 668265263) ^ Math.imul(salt + 13, 2246822519);
  n = Math.imul(n ^ (n >>> 13), 1274126177);
  return (n ^ (n >>> 16)) >>> 0;
}
function canvas(w, h, fill) { return {width:w, height:h, pixels:new Array(w * h).fill(fill || 0)}; }
function put(image, x, y, c) {
  if (x >= 0 && y >= 0 && x < image.width && y < image.height)
    image.pixels[y * image.width + x] = c;
}
function blit(src, dst, dx, dy) {
  for (var y = 0; y < src.height; y++) {
    for (var x = 0; x < src.width; x++) {
      var c = src.pixels[y * src.width + x];
      if (rgba.rgbaA(c)) put(dst, dx + x, dy + y, c);
    }
  }
}
function grassTile(variant) {
  var tile = canvas(32, 32);
  for (var y = 0; y < 32; y++) {
    for (var x = 0; x < 32; x++) {
      // Shared perimeter makes every grass variant compatible with every other.
      var edge = x < 2 || x > 29 || y < 2 || y > 29;
      var px = x === 31 ? 0 : x, py = y === 31 ? 0 : y;
      var seed = edge ? 0 : variant;
      var patch = hash(px >> 2, py >> 2, seed) % 9;
      var level = patch < 2 ? 1 : patch > 6 ? 3 : 2;
      if (hash(px, py, seed + 32) % 29 === 0) level++;
      tile.pixels[y * 32 + x] = grass[level];
    }
  }
  // Short leaf clusters, with shadows one pixel lower. No bright scatter noise.
  for (var i = 0; i < 4; i++) {
    var cx = 4 + hash(i, variant, 7) % 23;
    var cy = 5 + hash(i, variant, 11) % 22;
    put(tile, cx, cy, grass[1]); put(tile, cx + 1, cy, grass[1]);
    put(tile, cx - 1, cy - 2, grass[4]); put(tile, cx, cy - 1, grass[5]);
    put(tile, cx + 1, cy - 3, grass[4]); put(tile, cx + 2, cy - 2, grass[4]);
  }
  return tile;
}
function pathDistance(x, y, mask) {
  var dx = x - 15.5, dy = y - 15.5;
  var dist = Math.sqrt(dx * dx + dy * dy);
  if ((mask & 1) && dy <= 0) dist = Math.min(dist, Math.abs(dx));
  if ((mask & 2) && dx >= 0) dist = Math.min(dist, Math.abs(dy));
  if ((mask & 4) && dy >= 0) dist = Math.min(dist, Math.abs(dx));
  if ((mask & 8) && dx <= 0) dist = Math.min(dist, Math.abs(dy));
  return dist;
}
function pathTile(mask) {
  var tile = grassTile(0);
  for (var y = 0; y < 32; y++) {
    for (var x = 0; x < 32; x++) {
      var dist = pathDistance(x, y, mask);
      // Straight, equal-width edge ports; rougher contours only inside a tile.
      var rim = Math.min(x, y, 31 - x, 31 - y);
      var px = x === 31 ? 0 : x, py = y === 31 ? 0 : y;
      var roughness = rim < 3 ? 0 : ((hash(x >> 1, y >> 1, 43) % 3) - 1) * 0.32;
      var d = dist + roughness;
      if (d > 11.5) continue;
      var shade = 4;
      if (d > 10.8) shade = 0;
      else if (d > 9.8) shade = 1;
      else if (d > 8.8) shade = 2;
      else if (hash(px >> 1, py >> 1, 51) % 11 < 3) shade = 3;
      else if (hash(px, py, 61) % 41 === 0) shade = 5;
      tile.pixels[y * 32 + x] = earth[shade];
    }
  }
  // A few low contrast pebbles live away from all connection ports.
  [[11, 12], [20, 21], [22, 9]].forEach(function(p, i) {
    if (pathDistance(p[0], p[1], mask) < 7) {
      put(tile, p[0], p[1], earth[2]); put(tile, p[0] + 1, p[1], earth[3]);
      if (i === 0) put(tile, p[0], p[1] - 1, earth[5]);
    }
  });
  return tile;
}
function nearestColour(c) {
  var best = treePalette[0], bestDistance = Infinity;
  for (var i = 0; i < treePalette.length; i++) {
    var p = treePalette[i];
    var dr = rgba.rgbaR(c) - rgba.rgbaR(p);
    var dg = rgba.rgbaG(c) - rgba.rgbaG(p);
    var db = rgba.rgbaB(c) - rgba.rgbaB(p);
    var distance = dr * dr + dg * dg + db * db;
    if (distance < bestDistance) { bestDistance = distance; best = p; }
  }
  return best;
}
function read(filename) {
  var doc = app.open(filename), sprite = doc.sprite, layer = sprite.layer(0);
  if (sprite.layerCount !== 1 || layer.celCount !== 1)
    throw new Error('Pilot masters require one image layer and one frame');
  var cel = layer.cel(0), image = cel.image;
  var out = canvas(sprite.width, sprite.height);
  for (var y = 0; y < image.height; y++) for (var x = 0; x < image.width; x++)
    put(out, cel.x + x, cel.y + y, image.getPixel(x, y));
  return out;
}
function tileFromAtlas(atlas, i) {
  if (atlas.width !== 128 || atlas.height !== 160) throw new Error('Terrain master must be 128x160');
  var out = canvas(32, 32), left = (i % 4) * 32, top = Math.floor(i / 4) * 32;
  for (var y = 0; y < 32; y++) for (var x = 0; x < 32; x++)
    out.pixels[y * 32 + x] = atlas.pixels[(top + y) * atlas.width + left + x];
  return out;
}
if (cfg.regenerate) {
var refDoc = app.open(cfg.reference);
var ref = refDoc.sprite.layer(0).cel(0).image;
function bounds(x0, x1) {
  var b = {x0:x1, y0:ref.height, x1:x0, y1:0};
  for (var y = 0; y < ref.height; y++) {
    for (var x = x0; x < x1; x++) {
      if (rgba.rgbaA(ref.getPixel(x, y)) < 160) continue;
      b.x0 = Math.min(b.x0, x); b.x1 = Math.max(b.x1, x + 1);
      b.y0 = Math.min(b.y0, y); b.y1 = Math.max(b.y1, y + 1);
    }
  }
  if (b.x1 <= b.x0 || b.y1 <= b.y0) throw new Error('Reference panel has no opaque artwork');
  return b;
}
var split = Math.floor(ref.width * 0.57);
var treeBounds = bounds(0, split), stumpBounds = bounds(split, ref.width);
var treeScale = Math.min(60 / (treeBounds.x1 - treeBounds.x0), 84 / (treeBounds.y1 - treeBounds.y0));
function resource(b) {
  var out = canvas(64, 96);
  var w = Math.round((b.x1 - b.x0) * treeScale);
  var h = Math.round((b.y1 - b.y0) * treeScale);
  var left = Math.floor((64 - w) / 2), top = 88 - h;
  for (var y = 0; y < h; y++) {
    for (var x = 0; x < w; x++) {
      var sx = b.x0 + Math.min(b.x1 - b.x0 - 1, Math.floor((x + 0.5) / treeScale));
      var sy = b.y0 + Math.min(b.y1 - b.y0 - 1, Math.floor((y + 0.5) / treeScale));
      var c = ref.getPixel(sx, sy);
      if (rgba.rgbaA(c) >= 160) put(out, left + x, top + y, nearestColour(c));
    }
  }
  return out;
}
var oak = resource(treeBounds), stump = resource(stumpBounds);
var grasses = [grassTile(0), grassTile(1), grassTile(2)];
var paths = [];
for (var mask = 0; mask < 16; mask++) paths.push(pathTile(mask));
} else {
  var master = read(cfg.sources.terrain);
  var oak = read(cfg.sources.oak), stump = read(cfg.sources.stump);
  if (oak.width !== 64 || oak.height !== 96 || stump.width !== 64 || stump.height !== 96)
    throw new Error('Resource masters must be 64x96');
  var grasses = [], paths = [];
  for (var i = 0; i < 3; i++) grasses.push(tileFromAtlas(master, i));
  for (var i = 0; i < 16; i++) paths.push(tileFromAtlas(master, i + 3));
}
// Check every compatible edge at the actual authored pixel boundary.
function same(a, b) { if (a !== b) throw new Error('Tile connection seam'); }
for (var a = 0; a < 3; a++) for (var b = 0; b < 3; b++) for (var p = 0; p < 32; p++) {
  same(grasses[a].pixels[p * 32 + 31], grasses[b].pixels[p * 32]);
  same(grasses[a].pixels[31 * 32 + p], grasses[b].pixels[p]);
}
for (var a = 0; a < 16; a++) for (var b = 0; b < 16; b++) for (var p = 0; p < 32; p++) {
  if (!!(a & 2) === !!(b & 8)) same(paths[a].pixels[p * 32 + 31], paths[b].pixels[p * 32]);
  if (!!(a & 4) === !!(b & 1)) same(paths[a].pixels[31 * 32 + p], paths[b].pixels[p]);
}
var terrain = canvas(128, 160);
grasses.concat(paths).forEach(function(tile, i) { blit(tile, terrain, (i % 4) * 32, Math.floor(i / 4) * 32); });
function save(image, fixture, target) {
  var doc = app.open(fixture), sprite = doc.sprite, img = sprite.layer(0).cel(0).image;
  if (img.width !== image.width || img.height !== image.height) throw new Error('Canvas fixture size mismatch');
  for (var y = 0; y < image.height; y++) for (var x = 0; x < image.width; x++)
    img.putPixel(x, y, image.pixels[y * image.width + x]);
  sprite.commit(); sprite.saveAs(target, false);
}
if (cfg.regenerate) {
  save(terrain, cfg.fixtures.terrain, cfg.sources.terrain);
  save(oak, cfg.fixtures.resource, cfg.sources.oak);
  save(stump, cfg.fixtures.resource, cfg.sources.stump);
}
var scene = canvas(640, 384), road = {};
for (var x = 0; x < 20; x++) road[x + ',7'] = true;
for (var y = 0; y <= 7; y++) road['10,' + y] = true;
for (var x = 10; x <= 16; x++) road[x + ',3'] = true;
for (var y = 3; y <= 5; y++) road['16,' + y] = true;
for (var y = 0; y < 12; y++) for (var x = 0; x < 20; x++) {
  var tile = grasses[hash(x, y, 101) % 3];
  if (road[x + ',' + y]) {
    var mask = (road[x + ',' + (y - 1)] || y === 0 ? 1 : 0)
      | (road[(x + 1) + ',' + y] || x === 19 ? 2 : 0)
      | (road[x + ',' + (y + 1)] ? 4 : 0)
      | (road[(x - 1) + ',' + y] || x === 0 ? 8 : 0);
    tile = paths[mask];
  }
  blit(tile, scene, x * 32, y * 32);
}
var objects = [
  [2, 3, oak], [5, 4, oak], [7, 5, stump], [14, 2, oak], [18, 5, oak],
  [3, 10, oak], [7, 11, oak], [12, 11, stump], [15, 10, oak], [18, 11, oak]
];
objects.sort(function(a, b) { return a[1] - b[1]; }).forEach(function(o) {
  var cx = o[0] * 32 + 16, cy = o[1] * 32 + 24;
  // Scene-only ground contact; the resource PNG retains a clean cutout.
  for (var y = -3; y <= 3; y++) for (var x = -13; x <= 13; x++) {
    if (x * x / 169 + y * y / 9 <= 1) {
      var index = (cy - 2 + y) * scene.width + cx + x;
      var c = scene.pixels[index];
      scene.pixels[index] = rgba.rgba(Math.floor(rgba.rgbaR(c) * 0.73),
        Math.floor(rgba.rgbaG(c) * 0.76), Math.floor(rgba.rgbaB(c) * 0.72), 255);
    }
  }
  blit(o[2], scene, cx - 32, cy - 88);
});
save(scene, cfg.fixtures.scene, cfg.previews.native);
var enlarged = canvas(1280, 768);
for (var y = 0; y < enlarged.height; y++) for (var x = 0; x < enlarged.width; x++)
  enlarged.pixels[y * enlarged.width + x] = scene.pixels[(y >> 1) * scene.width + (x >> 1)];
save(enlarged, cfg.fixtures.large, cfg.previews.large);
console.log('FOREST_PILOT_OK: 3 grass variants, 16 path masks, oak and stump; compatible edges verified.');
