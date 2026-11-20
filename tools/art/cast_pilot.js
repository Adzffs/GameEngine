// LibreSprite 1.1-dev: native pixel cleanup, pose authoring and previews.
var cfg = __CONFIG__, pc = app.pixelColor;
function colour(hex) { return pc.rgba(parseInt(hex.slice(0, 2), 16), parseInt(hex.slice(2, 4), 16), parseInt(hex.slice(4, 6), 16), 255); }
var palette = cfg.palette.map(colour);
function canvas(w, h) { return {width:w, height:h, pixels:new Array(w * h).fill(0)}; }
function put(out, x, y, c) { if (x >= 0 && y >= 0 && x < out.width && y < out.height) out.pixels[y * out.width + x] = c; }
function blit(src, dst, dx, dy) {
  for (var y = 0; y < src.height; y++) for (var x = 0; x < src.width; x++) {
    var c = src.pixels[y * src.width + x];
    if (pc.rgbaA(c)) put(dst, dx + x, dy + y, c);
  }
}
function read(path) {
  var s = app.open(path).sprite, layer = s.layer(0), cel = layer.cel(0), img = cel.image;
  if (s.layerCount !== 1 || layer.celCount !== 1) throw new Error('Cast masters require one image layer and one frame');
  var out = canvas(s.width, s.height);
  for (var y = 0; y < img.height; y++) for (var x = 0; x < img.width; x++) put(out, cel.x + x, cel.y + y, img.getPixel(x, y));
  return out;
}
function crop(src, left, top, w, h) {
  var out = canvas(w, h);
  for (var y = 0; y < h; y++) for (var x = 0; x < w; x++) out.pixels[y * w + x] = src.pixels[(top + y) * src.width + left + x];
  return out;
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
function save(image, fixture, path) {
  var s = app.open(fixture).sprite, img = s.layer(0).cel(0).image;
  if (image.width !== s.width || image.height !== s.height) throw new Error('Incorrect cast canvas');
  for (var y = 0; y < image.height; y++) for (var x = 0; x < image.width; x++) img.putPixel(x, y, image.pixels[y * image.width + x]);
  s.commit(); s.saveAs(path, false);
}
function scale(src, factor) {
  var out = canvas(src.width * factor, src.height * factor);
  for (var y = 0; y < out.height; y++) for (var x = 0; x < out.width; x++) out.pixels[y * out.width + x] = src.pixels[Math.floor(y / factor) * src.width + Math.floor(x / factor)];
  return out;
}
var player, mara, furnace;
if (cfg.regenerate) {
  var ref = read(cfg.reference);
  function panel(index, w, h, targetHeight, pivotY, maxWidth) {
    var x0 = Math.floor((index % 3) * ref.width / 3), x1 = Math.floor(((index % 3) + 1) * ref.width / 3);
    var y0 = Math.floor(Math.floor(index / 3) * ref.height / 2), y1 = Math.floor((Math.floor(index / 3) + 1) * ref.height / 2);
    var b = {x0:x1, y0:y1, x1:x0, y1:y0};
    for (var y = y0; y < y1; y++) for (var x = x0; x < x1; x++) {
      if (pc.rgbaA(ref.pixels[y * ref.width + x]) < 192) continue;
      b.x0 = Math.min(b.x0, x); b.y0 = Math.min(b.y0, y);
      b.x1 = Math.max(b.x1, x + 1); b.y1 = Math.max(b.y1, y + 1);
    }
    if (b.x1 <= b.x0 || b.y1 <= b.y0) throw new Error('Empty cast reference panel');
    var factor = Math.min(targetHeight / (b.y1 - b.y0), maxWidth / (b.x1 - b.x0));
    var width = Math.round((b.x1 - b.x0) * factor), height = Math.round((b.y1 - b.y0) * factor);
    var out = canvas(w, h), left = Math.floor((w - width) / 2), top = pivotY - height;
    for (var y = 0; y < height; y++) for (var x = 0; x < width; x++) {
      var sx = Math.min(b.x1 - 1, b.x0 + Math.floor((x + 0.5) / factor));
      var sy = Math.min(b.y1 - 1, b.y0 + Math.floor((y + 0.5) / factor));
      var c = ref.pixels[sy * ref.width + sx];
      if (pc.rgbaA(c) >= 192) put(out, left + x, top + y, nearest(c));
    }
    return out;
  }
  player = canvas(128, 192);
  for (var direction = 0; direction < 4; direction++) {
    var base = panel(direction, 32, 48, 36, 40, 26);
    for (var phase = 0; phase < 4; phase++) blit(playerWalkPose(base, direction, phase), player, phase * 32, direction * 48);
  }
  mara = panel(4, 32, 48, 36, 40, 26);
  furnace = panel(5, 48, 64, 48, 56, 40);
} else {
  player = read(cfg.sources.player); mara = read(cfg.sources.mara); furnace = read(cfg.sources.furnace);
}
if (cfg.regenerate || cfg.addDiagonals) {
  if (player.width !== 128 || player.height !== 192) throw new Error('Adding diagonals requires the four-direction 128x192 master');
  var expanded = canvas(128, 384);
  blit(player, expanded, 0, 0);
  for (var direction = 4; direction < playerDirectionNames.length; direction++) {
    var base = playerDiagonalBase(player, direction);
    for (var phase = 0; phase < 4; phase++)
      blit(playerWalkPose(base, direction, phase), expanded, phase * 32, direction * 48);
  }
  player = expanded;
}
if (cfg.rebuildWalk) {
  if (player.width !== 128 || player.height !== 384) throw new Error('Rebuilding walking requires the eight-direction 128x384 master');
  var repaired = canvas(128, 384);
  for (var direction = 0; direction < playerDirectionNames.length; direction++) {
    var base = crop(player, 0, direction * 48, 32, 48);
    for (var phase = 0; phase < 4; phase++) {
      var frame = phase === 1 || phase === 3 ? playerWalkPose(base, direction, phase)
        : crop(player, phase * 32, direction * 48, 32, 48);
      blit(frame, repaired, phase * 32, direction * 48);
    }
  }
  player = repaired;
}
if (player.width !== 128 || player.height !== 384 || mara.width !== 32 || mara.height !== 48 || furnace.width !== 48 || furnace.height !== 64) throw new Error('Cast master geometry changed; use --add-diagonals to extend an old player master');
[player, mara, furnace].forEach(function(image) {
  image.pixels.forEach(function(c) { if (pc.rgbaA(c) !== 0 && pc.rgbaA(c) !== 255) throw new Error('Cast sprites require binary alpha'); });
});
var walkReport = inspectPlayerWalk(player);
console.log('WALK_VALIDATION_JSON=' + JSON.stringify(walkReport));
if (walkReport.failedFrames) {
  var bad = walkReport.frames.filter(function(frame) { return !frame.ok; })[0];
  throw new Error('Disconnected player walk: ' + bad.direction + ' pose ' + bad.pose
    + ', missing waist pixels=' + bad.missingWaistPixels + ', detached leg pixels=' + bad.detachedLowerPixels);
}
if (cfg.regenerate || cfg.rebuildWalk || cfg.addDiagonals) save(player, cfg.fixtures.player, cfg.sources.player);
if (cfg.regenerate) {
  save(mara, cfg.fixtures.mara, cfg.sources.mara);
  save(furnace, cfg.fixtures.furnace, cfg.sources.furnace);
}
var terrain = read(cfg.terrain);
function board(phase) {
  var out = canvas(320, 176);
  for (var y = 0; y < out.height; y++) for (var x = 0; x < out.width; x++) out.pixels[y * out.width + x] = terrain.pixels[(y % 32) * terrain.width + (x % 32)];
  for (var direction = 0; direction < playerDirectionNames.length; direction++)
    blit(crop(player, phase * 32, direction * 48, 32, 48), out, 24 + direction % 4 * 44, 40 + Math.floor(direction / 4) * 64);
  blit(mara, out, 204, 40); blit(furnace, out, 260, 24);
  return out;
}
save(scale(board(0), 2), cfg.fixtures.board, cfg.previews.board);
for (var phase = 0; phase < 4; phase++) save(scale(board(phase), 2), cfg.fixtures.board, cfg.walkFrames[phase]);

function inspection(phase) {
  var out = canvas(160, 128);
  for (var y = 0; y < out.height; y++) for (var x = 0; x < out.width; x++)
    out.pixels[y * out.width + x] = ((x >> 3) + (y >> 3)) % 2 ? colour('b4c5cf') : colour('e5ded0');
  for (var direction = 0; direction < playerDirectionNames.length; direction++)
    blit(crop(player, phase * 32, direction * 48, 32, 48), out, 4 + direction % 4 * 40, 8 + Math.floor(direction / 4) * 64);
  return out;
}
for (var phase = 0; phase < 4; phase++) save(scale(inspection(phase), 4), cfg.fixtures.inspection, cfg.inspectionFrames[phase]);
var contactSheet = canvas(160, 448);
for (var y = 0; y < contactSheet.height; y++) for (var x = 0; x < contactSheet.width; x++)
  contactSheet.pixels[y * contactSheet.width + x] = ((x >> 3) + (y >> 3)) % 2 ? colour('b4c5cf') : colour('e5ded0');
for (var direction = 0; direction < playerDirectionNames.length; direction++) for (var phase = 0; phase < 4; phase++)
  blit(crop(player, phase * 32, direction * 48, 32, 48), contactSheet, phase * 40 + 4, direction * 56 + 4);
save(scale(contactSheet, 4), cfg.fixtures.contacts, cfg.previews.contacts);
console.log('CAST_PILOT_OK: eight player directions, four walk poses, Mara and furnace; binary alpha; waist continuity checked.');
