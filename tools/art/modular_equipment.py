"""Bounded authoring contract for the isolated east-facing axe proof."""
import math
import re

from world_art_resources import json_file


def require(condition, message):
    if not condition:
        raise ValueError(message)


def point(value, bounds=None):
    require(isinstance(value, list) and len(value) == 2 and
            all(type(n) in (int, float) and math.isfinite(n) for n in value), 'Invalid finite point')
    if bounds:
        require(all(0 <= value[i] < bounds[i] for i in range(2)), 'Point outside declared space')
    return value


def validate_recipe(recipe):
    keys = {'schema', 'version', 'motion', 'facing', 'cell_size', 'ground_pivot', 'body_source',
            'shape_source', 'mask_source', 'shape_size', 'shape_grip', 'shape_axis',
            'cutting_edge_local', 'timing_source', 'frame_durations_ms', 'frames', 'materials', 'provenance'}
    require(isinstance(recipe, dict) and set(recipe) == keys, 'Unsupported recipe fields')
    require(recipe['schema'] == 'embervale.modular-equipment-pilot' and
            type(recipe['version']) is int and recipe['version'] == 1, 'Unsupported recipe version')
    require(recipe['motion'] == 'woodcutting.standard.east' and recipe['facing'] == 'east',
            'This proof supports east-facing woodcutting only')
    require(recipe['cell_size'] == [96, 96] and recipe['ground_pivot'] == [48, 64] and
            recipe['shape_size'] == [32, 40] and recipe['shape_axis'] == [0, -1], 'Unexpected source geometry')
    for name in ('cell_size', 'ground_pivot', 'shape_size', 'shape_axis'):
        require(isinstance(recipe[name], list) and all(type(n) is int for n in recipe[name]),
                'Source geometry must contain integer coordinates')
    require(recipe['timing_source'] == '../player-animation.json', 'Use the shared player timing source')
    for key, name in [('body_source', 'body'), ('shape_source', 'axe'), ('mask_source', 'metal-mask')]:
        require(recipe[key] == f'source/{name}.ase', 'Source paths must use the declared local masters')
    point(recipe['shape_grip'], recipe['shape_size'])
    point(recipe['cutting_edge_local'], recipe['shape_size'])
    durations = recipe['frame_durations_ms']
    require(isinstance(durations, list) and len(durations) == 8 and
            all(type(ms) is int and 0 < ms <= 1000 and ms % 10 == 0 for ms in durations) and
            sum(durations) == 1200, 'Chopping requires eight bounded holds totalling 1200 ms')
    frames = recipe['frames']
    require(isinstance(frames, list) and len(frames) == 8, 'Eight authored hand-track poses required')
    for index, frame in enumerate(frames):
        require(isinstance(frame, dict) and set(frame) == {'pose', 'main_hand', 'off_hand', 'aim',
                'front_hand_pixels', 'tool_in_front'}, 'Invalid hand-track fields')
        require(type(frame['pose']) is int and frame['pose'] == index, 'Pose IDs must be ordered and complete')
        main = point(frame['main_hand'], [96, 96])
        off = point(frame['off_hand'], [96, 96])
        aim = point(frame['aim'])
        require(abs(math.hypot(*aim) - 1) < 0.00001, 'Tool aim must be normalized')
        require(type(frame['tool_in_front']) is bool, 'Draw order must be explicit')
        # The canonical blade extends to the right of its upward shaft. This
        # facing must preserve the cutting edge toward the tree on screen-right.
        require(-aim[1] > 0, 'The axe cutting edge must face east')
        lateral = abs((off[0] - main[0]) * -aim[1] + (off[1] - main[1]) * aim[0])
        along = (off[0] - main[0]) * aim[0] + (off[1] - main[1]) * aim[1]
        require(lateral <= 1.5 and -6 <= along <= 1, 'Secondary hand no longer fits the shared shaft')
        hand = frame['front_hand_pixels']
        require(isinstance(hand, list) and 0 < len(hand) <= 9, 'A bounded front-hand mask is required')
        seen = set()
        for pixel in hand:
            point(pixel, [96, 96])
            require(all(type(n) is int for n in pixel) and tuple(pixel) not in seen,
                    'Hand mask pixels must be unique integer coordinates')
            require(math.hypot(pixel[0] - main[0], pixel[1] - main[1]) <= 2,
                    'Front-hand mask must remain at the main grip')
            seen.add(tuple(pixel))
    materials = recipe['materials']
    require(isinstance(materials, list) and 1 <= len(materials) <= 8, 'Material count outside proof budget')
    ids, items = set(), set()
    for material in materials:
        require(isinstance(material, dict) and set(material) == {'id', 'item', 'metal'}, 'Invalid material fields')
        identity, item = material['id'], material['item']
        require(isinstance(identity, str) and re.fullmatch(r'[a-z][a-z0-9-]{0,23}', identity) and identity not in ids,
                'Duplicate or invalid material ID')
        require(isinstance(item, str) and re.fullmatch(r'[A-Z][A-Z0-9_]{0,47}', item) and item not in items,
                'Duplicate or invalid item visual mapping')
        colours = material['metal']
        require(isinstance(colours, list) and len(colours) == 3 and
                all(isinstance(c, str) and re.fullmatch(r'[0-9a-f]{6}', c) for c in colours),
                'Metal requires shadow, midtone and highlight colours')
        ids.add(identity)
        items.add(item)
    require(isinstance(recipe['provenance'], dict) and len(recipe['provenance']) <= 8 and
            all(isinstance(k, str) and isinstance(v, str) and len(k) <= 64 and len(v) <= 2048
                for k, v in recipe['provenance'].items()), 'Invalid provenance')
    return recipe


def load_recipe(path):
    from animation_common import load_timing
    recipe = validate_recipe(json_file(path, 65536))
    require(recipe['frame_durations_ms'] == load_timing()['chopping_ms'],
            'Review and current chopping timing must agree')
    return recipe
