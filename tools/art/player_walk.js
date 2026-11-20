// Native-pixel walk poses. Torso and thighs overlap at a retained pelvis;
// moving the upper body must never expose a transparent waist seam.
var playerDirectionNames = ['south', 'west', 'east', 'north',
  'southwest', 'southeast', 'northwest', 'northeast'];

// Author three-quarter views from the matching front/back outfit and profile.
// Keep the existing cardinal cells intact, including manually painted detail.
function playerDiagonalBase(atlas, direction) {
  var north = direction >= 6, west = direction === 4 || direction === 6;
  var sign = west ? -1 : 1, base = crop(atlas, 0, (north ? 3 : 0) * 48, 32, 48);
  var profile = crop(atlas, 0, (west ? 1 : 2) * 48, 32, 48), out = canvas(32, 48);
  for (var y = 0; y < 40; y++) for (var x = 0; x < 32; x++) {
    var c = base.pixels[y * 32 + x];
    if (!pc.rgbaA(c)) continue;
    var head = y < 21, far = sign * (x - 16) < 0;
    var tx = 16 + Math.round((x - 16) * 0.84) + (head ? sign : 0);
    var ty = y - (far && y >= 32 ? 1 : 0);
    put(out, tx, ty, c);
  }
  // The leading cheek/nose follows the side view; the other half of the head
  // retains the broader front/back silhouette rather than rotating the sprite.
  for (var y = 11; y < 20; y++) for (var x = 0; x < 32; x++) {
    var c = profile.pixels[y * 32 + x];
    if (pc.rgbaA(c) && sign * (x - 16) >= 2) put(out, x, y, c);
  }
  return out;
}

function playerWalkPose(base, direction, phase) {
  return rigWalk(base, direction, phase);
}

function inspectPlayerWalk(atlas) {
  var report = {frames:[], failedFrames:0};
  for (var direction = 0; direction < playerDirectionNames.length; direction++) {
    var neutral = crop(atlas, 0, direction * 48, 32, 48);
    for (var phase = 0; phase < 8; phase++) {
      var frame = crop(atlas, phase * 32, direction * 48, 32, 48);
      var labels = new Array(32 * 48).fill(-1), sizes = [];
      function solid(index) { return pc.rgbaA(frame.pixels[index]) >= 128; }
      for (var seed = 0; seed < labels.length; seed++) {
        if (!solid(seed) || labels[seed] !== -1) continue;
        var component = sizes.length, queue = [seed], size = 0;
        labels[seed] = component;
        for (var next = 0; next < queue.length; next++) {
          var index = queue[next], sx = index % 32, sy = Math.floor(index / 32);
          size++;
          for (var dy = -1; dy <= 1; dy++) for (var dx = -1; dx <= 1; dx++) {
            var x = sx + dx, y = sy + dy;
            if (x < 0 || y < 0 || x >= 32 || y >= 48) continue;
            var neighbour = y * 32 + x;
            if (solid(neighbour) && labels[neighbour] === -1) {
              labels[neighbour] = component; queue.push(neighbour);
            }
          }
        }
        sizes.push(size);
      }
      var main = -1, largest = 0;
      sizes.forEach(function(size, index) { if (size > largest) { main = index; largest = size; } });
      var detached = 0, lower = 0, missingWaist = 0;
      for (var y = 29; y < 40; y++) for (var x = 0; x < 32; x++) {
        var index = y * 32 + x;
        if (solid(index)) { lower++; if (labels[index] !== main) detached++; }
      }
      // Closed waist silhouette is an asset contract, independent of how a
      // pose is generated. Normal exports also check manually edited masters.
      for (var y = 26; y <= 32; y++) {
        var neutralInk=0,poseInk=0;
        for(var x=0;x<32;x++) {
          if(pc.rgbaA(neutral.pixels[y*32+x])>=128) neutralInk++;
          if(solid(y*32+x)) poseInk++;
        }
        if(neutralInk&&!poseInk) missingWaist++;
      }
      var ok = largest > 0 && lower > 0 && detached === 0 && missingWaist === 0;
      report.frames.push({direction:playerDirectionNames[direction], pose:phase,
        detachedLowerPixels:detached, missingWaistPixels:missingWaist, lowerPixels:lower, ok:ok});
      if (!ok) report.failedFrames++;
    }
  }
  return report;
}
