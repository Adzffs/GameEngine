"""Read-only resource validation for the current eight world-art batches."""
from __future__ import annotations
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import zlib

REPO = Path(__file__).resolve().parents[2]
BATCH_NAMES = ('forest-pilot', 'cast-pilot', 'environment-pilot', 'service-pilot',
               'building-pilot', 'variation-pilot', 'woodcutting-pilot', 'mining-pilot')
BATCH_VERSIONS = {name: (2 if name in ('building-pilot', 'variation-pilot') else 1) for name in BATCH_NAMES}
LIMIT_KEYS = {'max_images', 'max_frames', 'max_image_edge', 'max_image_pixels', 'max_png_bytes',
              'max_manifest_bytes', 'max_catalogue_png_bytes', 'max_texture_bytes', 'max_alpha_bytes',
              'max_staging_bytes'}
TOKEN = re.compile(r'[a-z0-9._-]{1,128}\Z')
PNG_SCRATCH_BYTES = 65536
BINDINGS = {
    'character.player.': (1, 4, True), 'character.woodcutting.': (1, 6, True),
    'character.mining.': (1, 6, True), 'tool.axe.': (3, 6, True), 'tool.pickaxe.': (3, 6, True),
    **{f'variation.{name}.': (count, 1, False) for name, count in
       [('grass', 162), ('paths', 204), ('water', 48), ('stone', 12), ('flora', 40),
        ('walls', 96), ('roof', 72), ('floors', 9), ('trees', 3), ('stumps', 3), ('rocks', 9)]},
}

class ArtError(ValueError):
    pass


def validate_renderer_layout(path, expected, limit):
    # Git may check text bindings out as CRLF on Windows. Only newline spelling
    # is canonicalized; geometry, identifiers and all binary checks stay exact.
    actual = read_regular(path, limit)
    require(actual.replace(b'\r\n', b'\n') == expected.encode('utf-8'),
            'Generated renderer layout is stale; validate/export the intended manifests before building')

def require(condition, message):
    if not condition:
        raise ArtError(message)

def pairs(values):
    out = {}
    for key, value in values:
        require(key not in out, f'Duplicate JSON field: {key}')
        out[key] = value
    return out

def read_regular(path: Path, limit: int) -> bytes:
    require(not path.is_symlink() and path.is_file(), f'Expected a regular non-symlink file: {path}')
    require(path.stat().st_size <= limit, f'File exceeds {limit} bytes: {path}')
    with path.open('rb') as stream:
        data = stream.read(limit + 1)
    require(len(data) <= limit, f'File grew beyond its limit: {path}')
    return data

def json_file(path, limit):
    try:
        return json.loads(read_regular(Path(path), limit), object_pairs_hook=pairs,
                          parse_constant=lambda _: (_ for _ in ()).throw(ArtError('Non-finite JSON value')))
    except (ValueError, UnicodeError, RecursionError) as error:
        raise ArtError(f'Invalid JSON in {path}: {error}') from error

def load_limits(root=REPO):
    value = json_file(Path(root) / 'assets/embervale/world-art-limits.json', 4096)
    require(isinstance(value, dict) and set(value) == LIMIT_KEYS | {'version'}, 'Unsupported world-art policy fields')
    require(type(value['version']) is int and value['version'] == 1, 'Unsupported world-art policy version')
    for key in LIMIT_KEYS:
        require(type(value[key]) is int and 0 < value[key] <= 2147483647, f'Invalid resource policy: {key}')
    return value

def integers(value, length, context):
    require(isinstance(value, list) and len(value) == length and all(type(v) is int for v in value),
            f'{context} must contain {length} integers')
    return value

def measure_image(size, limits):
    w, h = integers(size, 2, 'Image size')
    require(0 < w <= limits['max_image_edge'] and 0 < h <= limits['max_image_edge'], 'Image edge exceeds limits')
    pixels = w * h
    require(pixels <= limits['max_image_pixels'] and pixels * 4 <= limits['max_texture_bytes'] and
            pixels <= limits['max_alpha_bytes'], 'Image exceeds pixel/texture/alpha budget')
    require(max(pixels * 4, PNG_SCRATCH_BYTES) + limits['max_png_bytes'] <= limits['max_staging_bytes'], 'Image exceeds staging budget')
    return pixels

def validate_png(data: bytes, size, limits):
    """CRC/order/header preflight plus bounded streaming IDAT verification; no pixel buffer."""
    measure_image(size, limits)
    w, h = size
    require(57 <= len(data) <= limits['max_png_bytes'] and data[:8] == b'\x89PNG\r\n\x1a\n', 'Invalid/oversized PNG')
    at, seen_header, seen_data, ended = 8, False, False, False
    compressed = bytearray()
    while at < len(data):
        require(len(data) - at >= 12, 'Truncated PNG chunk')
        length = struct.unpack_from('>I', data, at)[0]
        require(length <= len(data) - at - 12, 'PNG chunk exceeds file bounds')
        kind = data[at + 4:at + 8]
        payload = data[at + 8:at + 8 + length]
        require(zlib.crc32(data[at + 4:at + 8 + length]) == struct.unpack_from('>I', data, at + 8 + length)[0],
                'PNG chunk checksum mismatch')
        if not seen_header:
            require(kind == b'IHDR' and length == 13 and payload[:8] == struct.pack('>II', w, h),
                    'PNG dimensions do not match manifest')
            require(payload[8:] == bytes([8, 6, 0, 0, 0]), 'PNG must be non-interlaced RGBA8')
            seen_header = True
        elif kind == b'IDAT':
            compressed.extend(payload)
            seen_data = True
        elif kind == b'IEND' and seen_data and length == 0 and at + 12 == len(data):
            ended = True
        else:
            raise ArtError('Unsupported PNG chunk or chunk order')
        at += length + 12
    require(seen_header and seen_data and ended, 'PNG is missing image data/end')
    stride, produced = w * 4 + 1, 0
    expected = stride * h
    inflater = zlib.decompressobj()
    pending = compressed
    try:
        while pending:
            block = inflater.decompress(pending, min(65536, expected - produced + 1))
            require(produced + len(block) <= expected, 'PNG decompression exceeds declared dimensions')
            first_filter = (-produced) % stride
            require(all(block[i] <= 4 for i in range(first_filter, len(block), stride)), 'Invalid PNG row filter')
            produced += len(block)
            remaining = inflater.unconsumed_tail
            require(block or len(remaining) < len(pending), 'PNG decompressor made no progress')
            pending = remaining
        require(inflater.eof and not inflater.unused_data and produced == expected, 'Incomplete/trailing PNG pixel stream')
    except zlib.error as error:
        raise ArtError(f'Invalid PNG pixel stream: {error}') from error

def relative_leaf(value, directory, suffix):
    require(isinstance(value, str) and value.startswith(directory + '/'), f'Expected {directory} file')
    name = value[len(directory) + 1:]
    require(bool(TOKEN.fullmatch(name)) and name not in ('.', '..') and name.endswith(suffix), 'Unsafe asset path')
    return name

def validate_manifests(root, manifests, limits, check_files=True):
    """Validate all metadata/budgets first, then bounded files; return portable accounting."""
    require(0 < len(manifests) <= len(BATCH_NAMES), 'Unsupported batch count')
    images, assets, batch_ids = [], [], set()
    for batch in manifests:
        require(isinstance(batch, dict) and batch.get('schema') == 'embervale.authoring-batch' and
                type(batch.get('version')) is int, 'Unsupported art manifest schema/version')
        identity = batch.get('id')
        require(identity in BATCH_NAMES and identity not in batch_ids, 'Unknown/duplicate art batch')
        require(batch['version'] == BATCH_VERSIONS[identity], 'Unsupported art manifest version for batch')
        batch_ids.add(identity)
        require(type(batch.get('native_tile_size')) is int and batch['native_tile_size'] == 32, 'Unsupported art tile size')
        require(isinstance(batch.get('images'), dict) and batch['images'], 'Batch has no images')
        require(isinstance(batch.get('assets'), list) and batch['assets'], 'Batch has no frames')
        images.extend((batch, name, image) for name, image in batch['images'].items())
        assets.extend((batch, asset) for asset in batch['assets'])
    require(len(images) <= limits['max_images'] and len(assets) <= limits['max_frames'], 'Catalogue count exceeds limits')
    paths, texture, alpha = set(), 0, 0
    image_reports = []
    for batch, name, image in images:
        require(bool(TOKEN.fullmatch(name)) and isinstance(image, dict), 'Invalid image entry')
        leaf = relative_leaf(image.get('image'), 'textures', '.png')
        relative_leaf(image.get('source'), 'source', '.ase')
        key = (batch['id'], leaf)
        require(key not in paths, 'Duplicate texture path')
        paths.add(key)
        pixels = measure_image(image.get('size'), limits)
        texture += pixels * 4
        alpha += pixels
        require(texture <= limits['max_texture_bytes'] and alpha <= limits['max_alpha_bytes'], 'Catalogue exceeds texture/alpha budget')
        image_reports.append({'batch': batch['id'], 'image': name, 'path': f"assets/embervale/{batch['id']}/{image['image']}",
                              'width': image['size'][0], 'height': image['size'][1], 'pixels': pixels,
                              'rgba_texture_bytes': pixels * 4, 'alpha_bytes': pixels})
    ids, slots = set(), {prefix: set() for prefix in BINDINGS}
    for batch, frame in assets:
        require(isinstance(frame, dict), 'Invalid frame entry')
        identity = frame.get('id')
        require(isinstance(identity, str) and bool(TOKEN.fullmatch(identity)) and identity not in ('.', '..') and identity not in ids,
                'Invalid/duplicate frame ID')
        ids.add(identity)
        require(isinstance(frame.get('image'), str) and frame['image'] in batch['images'], 'Frame references unknown image')
        w, h = batch['images'][frame['image']]['size']
        x, y, fw, fh = integers(frame.get('rect'), 4, 'Frame rectangle')
        require(x >= 0 and y >= 0 and fw > 0 and fh > 0 and x + fw <= w and y + fh <= h,
                'Frame rectangle is outside texture')
        px, py = integers(frame.get('pivot'), 2, 'Frame pivot')
        require(0 <= px <= fw and 0 <= py <= fh, 'Frame pivot is outside rectangle')
        ground = integers(frame.get('intended_ground_point_in_tile', [0, 0]), 2, 'Ground point')
        require(all(0 <= n <= 32 for n in ground), 'Ground point is outside tile')
        values = [frame.get(k, default) for k, default in
                  [('connection_mask', -1), ('variant', -1), ('facing', -1), ('animation_frame', 0)]]
        require(all(type(n) is int for n in values), 'Frame selectors must be integers')
        connection, variant, facing, pose = values
        require(-1 <= connection <= 15 and -1 <= variant <= 4095 and -1 <= facing < 8 and 0 <= pose <= 255,
                'Frame selector is outside bounds')
        for prefix, (variants, poses, action) in BINDINGS.items():
            if not identity.startswith(prefix):
                continue
            if action:
                tier = 0 if variants == 1 else variant
                require(facing >= 0 and pose < poses and 0 <= tier < variants, 'Action frame cannot bind to runtime table')
                slot = (tier * 8 + facing) * poses + pose
            else:
                require(0 <= variant < variants, 'Variant cannot bind to runtime table')
                slot = variant
            require(slot not in slots[prefix], 'Duplicate runtime frame binding')
            slots[prefix].add(slot)
    for prefix, values in slots.items():
        variants, poses, action = BINDINGS[prefix]
        require(not values or len(values) == variants * (8 if action else 1) * poses, 'Incomplete runtime action/variant table')
    walk = next((batch['walk']['cycle_distance_tiles'] for batch in manifests if 'walk' in batch), 1.0)
    require(type(walk) in (float, int) and math.isfinite(walk) and 0 < walk <= 8, 'Invalid player walk-cycle distance')
    encoded, peak_staging = 0, 0
    if check_files:
        for (batch, name, image), row in zip(images, image_reports):
            folder = Path(root) / 'assets/embervale' / batch['id']
            require(not folder.is_symlink() and not (folder / 'textures').is_symlink() and
                    not (folder / 'source').is_symlink(), 'Art directories must not be symbolic links')
            data = read_regular(folder / image['image'], limits['max_png_bytes'])
            validate_png(data, image['size'], limits)
            require(hashlib.sha256(data).hexdigest() == image.get('image_sha256'), 'PNG checksum differs from manifest')
            source = read_regular(folder / image['source'], limits['max_manifest_bytes'] * 8)
            require(hashlib.sha256(source).hexdigest() == image.get('source_sha256'), 'Editable source checksum differs from manifest')
            encoded += len(data)
            require(encoded <= limits['max_catalogue_png_bytes'], 'Catalogue exceeds encoded PNG budget')
            staging = len(data) + max(row['rgba_texture_bytes'], PNG_SCRATCH_BYTES)
            require(staging <= limits['max_staging_bytes'], 'Image exceeds actual staging budget')
            row.update(png_bytes=len(data), png_sha256=image['image_sha256'], source_sha256=image['source_sha256'])
            peak_staging = max(peak_staging, staging)
    return {'batches': len(manifests), 'images': len(images), 'frames': len(assets), 'png_bytes': encoded,
            'rgba_texture_bytes': texture, 'alpha_bytes': alpha, 'retained_pixel_bytes': texture + alpha,
            'peak_image_staging_bytes': peak_staging,
            'logical_pixel_and_staging_upper_bound_bytes': texture + alpha + peak_staging,
            'textures': image_reports}

def load_manifests(root=REPO, limits=None):
    limits = limits or load_limits(root)
    return [json_file(Path(root) / f'assets/embervale/{name}/manifest.json', limits['max_manifest_bytes'])
            for name in BATCH_NAMES]

def renderer_layout(manifests):
    """Preserve existing checked-in geometry exactly; caller validates first."""
    images = [(batch, name, image) for batch in manifests for name, image in batch['images'].items()]
    assets = [(batch, asset) for batch in manifests for asset in batch['assets']]
    walk_cycle = float(next((batch['walk']['cycle_distance_tiles'] for batch in manifests if 'walk' in batch), 1.0))
    lines = ['// Generated from world-art batch manifests by tools/art/build_*_pilot.py.',
             '#pragma once', '#include <SDL3/SDL.h>', '#include <array>', '#include <cstddef>',
             '#include <string_view>', '', 'namespace ForestPilotLayout', '{',
             '    struct Image { std::string_view filename; int width, height; std::string_view directory; };',
             '    struct Frame { std::string_view id; std::size_t image; SDL_FRect source;',
             '        SDL_FPoint pivot, ground; int connectionMask, grassVariant, facing, animationFrame; };',
             '    inline constexpr int TileSize = 32;',
             f'    inline constexpr float PlayerWalkCycleTiles = {walk_cycle}f;',
             f'    inline constexpr std::array<Image, {len(images)}> Images{{{{']
    for batch, name, image in images:
        w, h = image['size']
        lines.append(f'        {{{json.dumps(Path(image["image"]).name)}, {w}, {h}, '
                     f'{json.dumps("assets/embervale/" + batch["id"] + "/textures")}}},')
    lines += ['    }};', f'    inline constexpr std::array<Frame, {len(assets)}> Frames{{{{']
    def point(values):return '{' + ', '.join(f'{n}.0f' for n in values) + '}'
    for batch, frame in assets:
        index = next(i for i, (owner, name, _) in enumerate(images) if owner is batch and name == frame['image'])
        lines.append('        {' + json.dumps(frame['id']) + ', ' + str(index) + ', ' + point(frame['rect']) + ', ' +
                     point(frame['pivot']) + ', ' + point(frame.get('intended_ground_point_in_tile', [0, 0])) + ', ' +
                     ', '.join(str(frame.get(k, default)) for k, default in
                               [('connection_mask', -1), ('variant', -1), ('facing', -1), ('animation_frame', 0)]) + '},')
    return '\n'.join(lines + ['    }};', '}', ''])
