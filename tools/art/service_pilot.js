// LibreSprite performs all visible pixel work; Python supplies empty technical canvases.
var cfg = __CONFIG__, pc = app.pixelColor;
function colour(hex) { return pc.rgba(parseInt(hex.slice(0, 2), 16), parseInt(hex.slice(2, 4), 16), parseInt(hex.slice(4, 6), 16), 255); }
var palette = cfg.palette.map(colour);
function canvas(w, h) { return {width:w, height:h, pixels:new Array(w * h).fill(0)}; }
function put(out, x, y, c) { if (x >= 0 && y >= 0 && x < out.width && y < out.height) out.pixels[y * out.width + x] = c; }
function read(path) {
  var s = app.open(path).sprite, layer = s.layer(0), cel = layer.cel(0), img = cel.image;
  if (s.layerCount !== 1 || layer.celCount !== 1) throw new Error('Service masters require one image layer and one frame');
  var out = canvas(s.width, s.height);
  for (var y = 0; y < img.height; y++) for (var x = 0; x < img.width; x++) put(out, cel.x + x, cel.y + y, img.getPixel(x, y));
  return out;
}
function save(image, fixture, path) {
  var s = app.open(fixture).sprite, img = s.layer(0).cel(0).image;
  if (image.width !== s.width || image.height !== s.height) throw new Error('Incorrect service canvas');
  for (var y = 0; y < image.height; y++) for (var x = 0; x < image.width; x++) img.putPixel(x, y, image.pixels[y * image.width + x]);
  s.commit(); s.saveAs(path, false);
}
function blit(src, dst, dx, dy) {
  for (var y = 0; y < src.height; y++) for (var x = 0; x < src.width; x++) {
    var c = src.pixels[y * src.width + x];
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
function scale(src, factor) {
  var out = canvas(src.width * factor, src.height * factor);
  for (var y = 0; y < out.height; y++) for (var x = 0; x < out.width; x++)
    out.pixels[y * out.width + x] = src.pixels[Math.floor(y / factor) * src.width + Math.floor(x / factor)];
  return out;
}
var bank, table;
if (cfg.regenerate) {
  var ref = read(cfg.reference);
  function panel(index, maxWidth, maxHeight) {
    var x0 = Math.floor(index * ref.width / 2), x1 = Math.floor((index + 1) * ref.width / 2);
    var b = {x0:x1, y0:ref.height, x1:x0, y1:0};
    for (var y = 0; y < ref.height; y++) for (var x = x0; x < x1; x++) {
      if (pc.rgbaA(ref.pixels[y * ref.width + x]) < 192) continue;
      b.x0 = Math.min(b.x0, x); b.y0 = Math.min(b.y0, y); b.x1 = Math.max(b.x1, x + 1); b.y1 = Math.max(b.y1, y + 1);
    }
    if (b.x1 <= b.x0 || b.y1 <= b.y0) throw new Error('Empty service reference panel');
    var factor = Math.min(maxWidth / (b.x1 - b.x0), maxHeight / (b.y1 - b.y0));
    var w = Math.round((b.x1 - b.x0) * factor), h = Math.round((b.y1 - b.y0) * factor);
    var out = canvas(48, 48), left = Math.floor((48 - w) / 2), top = 40 - h;
    for (var y = 0; y < h; y++) for (var x = 0; x < w; x++) {
      var sx = Math.min(b.x1 - 1, b.x0 + Math.floor((x + 0.5) / factor));
      var sy = Math.min(b.y1 - 1, b.y0 + Math.floor((y + 0.5) / factor));
      var c = ref.pixels[sy * ref.width + sx];
      if (pc.rgbaA(c) >= 192) put(out, left + x, top + y, nearest(c));
    }
    return out;
  }
  bank = panel(0, 36, 28); table = panel(1, 40, 30);
  save(bank, cfg.fixtures.sprite, cfg.sources.bank);
  save(table, cfg.fixtures.sprite, cfg.sources['crafting-table']);
} else {
  bank = read(cfg.sources.bank); table = read(cfg.sources['crafting-table']);
}
[bank, table].forEach(function(image) {
  if (image.width !== 48 || image.height !== 48) throw new Error('Service master geometry changed');
  var ink = 0;
  image.pixels.forEach(function(c) {
    if (pc.rgbaA(c) !== 0 && pc.rgbaA(c) !== 255) throw new Error('Service sprites require binary alpha');
    if (pc.rgbaA(c)) ink++;
  });
  if (ink === 0) throw new Error('Empty service master');
});
var grass = read(cfg.forest), terrain = read(cfg.environment), board = canvas(320, 128);
for (var y = 0; y < board.height; y++) for (var x = 0; x < board.width; x++) {
  var ground = x < 160 ? grass.pixels[(y % 32) * grass.width + x % 32]
    : terrain.pixels[(y % 32) * terrain.width + 96 + x % 32];
  put(board, x, y, ground);
}
blit(bank, board, 16, 32); blit(table, board, 88, 32);
blit(bank, board, 176, 32); blit(table, board, 248, 32);
save(scale(board, 2), cfg.fixtures['service-pilot-2x.png'], cfg.previews['service-pilot-2x.png']);
var inspection = canvas(96, 48);
for (var y = 0; y < inspection.height; y++) for (var x = 0; x < inspection.width; x++)
  put(inspection, x, y, ((x >> 3) + (y >> 3)) % 2 ? colour('b4c5cf') : colour('e5ded0'));
blit(bank, inspection, 0, 0); blit(table, inspection, 48, 0);
save(scale(inspection, 4), cfg.fixtures['service-inspection-4x.png'], cfg.previews['service-inspection-4x.png']);
console.log('SERVICE_PILOT_OK: bank and crafting table; shared foot pivot; binary alpha; saved masters preserved unless regenerating.');
