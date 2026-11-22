"""Verify visible tool clearance, shared materials and preserved authored sources."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/art'))
from build_modular_axe_pilot import PILOT, SOURCES, build, check
from modular_equipment import load_recipe, validate_recipe


def png_pixels(path):
    """Read RGBA8 test outputs independently of the LibreSprite compositor."""
    data = path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('Expected PNG')
    at, compressed = 8, bytearray()
    while at < len(data):
        length = struct.unpack_from('>I', data, at)[0]
        kind, payload = data[at+4:at+8], data[at+8:at+8+length]
        if kind == b'IHDR':
            width, height, depth, colour, _, _, interlace = struct.unpack('>IIBBBBB', payload)
            if (depth, colour, interlace) != (8, 6, 0):
                raise ValueError('Expected non-interlaced RGBA8')
        elif kind == b'IDAT':
            compressed.extend(payload)
        at += length + 12
    raw, result, stride = zlib.decompress(compressed), bytearray(), width * 4
    previous = bytearray(stride)
    for y in range(height):
        mode = raw[y * (stride + 1)]
        row = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for x in range(stride):
            a, b, c = row[x-4] if x >= 4 else 0, previous[x], previous[x-4] if x >= 4 else 0
            if mode == 1: predictor = a
            elif mode == 2: predictor = b
            elif mode == 3: predictor = (a + b) // 2
            elif mode == 4:
                p = a + b - c
                distances = [abs(p-a), abs(p-b), abs(p-c)]
                predictor = [a,b,c][distances.index(min(distances))]
            elif mode == 0: predictor = 0
            else: raise ValueError('Unsupported PNG filter')
            row[x] = (row[x] + predictor) & 255
        result.extend(row)
        previous = row
    return width, height, bytes(result)


def inventory(folder):
    return {p.relative_to(folder).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in folder.rglob('*') if p.is_file()}


class RecipeTests(unittest.TestCase):
    def setUp(self):
        self.recipe = json.loads((PILOT / 'recipe.json').read_text())

    def test_current_recipe_matches_shared_action_timing(self):
        self.assertEqual(load_recipe(PILOT / 'recipe.json')['frame_durations_ms'],
                         json.loads((ROOT / 'assets/embervale/player-animation.json').read_text())['chopping_ms'])

    def test_bad_grips_directions_durations_and_masks_reject(self):
        cases = []
        for field, value in [('facing', 'west'), ('version', True), ('shape_axis', [False, -1]),
                             ('body_source', '../outside.ase'), ('frame_durations_ms', [150] * 7)]:
            bad = copy.deepcopy(self.recipe); bad[field] = value; cases.append(bad)
        for field, value in [('main_hand', [float('nan'), 40]), ('aim', [0, -2]),
                             ('off_hand', [80, 80]), ('foreground', {}),
                             ('pose', True), ('tool_in_front', 1)]:
            bad = copy.deepcopy(self.recipe); bad['frames'][0][field] = value; cases.append(bad)
        for bad in cases:
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                validate_recipe(bad)

    def test_unstable_depth_and_invalid_foreground_regions_reject(self):
        for change in ('version', 'order', 'legacy-depth', 'missing-hand', 'empty',
                       'bounds', 'boolean', 'duplicate', 'unsorted', 'detached', 'missing-grip'):
            bad = copy.deepcopy(self.recipe)
            frame = bad['frames'][0]
            if change == 'version': bad['version'] = 1
            elif change == 'order': bad['composition'] = 'tool-body'
            elif change == 'legacy-depth': frame['tool_in_front'] = False
            elif change == 'missing-hand': del frame['foreground']['off_hand']
            elif change == 'empty': frame['foreground']['head'] = []
            elif change == 'bounds': frame['foreground']['head'][0] = [28, -1, 40]
            elif change == 'boolean': frame['foreground']['head'][0][0] = True
            elif change == 'duplicate': frame['foreground']['head'].insert(1, frame['foreground']['head'][0])
            elif change == 'unsorted': frame['foreground']['head'].reverse()
            elif change == 'detached': frame['foreground']['off_hand'] = [[70, 70, 70]]
            elif change == 'missing-grip': frame['foreground']['main_hand'] = [[50, 48, 48]]
            with self.subTest(change=change), self.assertRaises(ValueError): validate_recipe(bad)

    def test_ambiguous_materials_and_out_of_budget_inputs_reject(self):
        for change in ('id', 'item', 'colour', 'count', 'extra'):
            bad = copy.deepcopy(self.recipe)
            if change in ('id', 'item'):
                bad['materials'][1][change] = bad['materials'][0][change]
            elif change == 'colour': bad['materials'][0]['metal'][0] = 'not-rgb'
            elif change == 'count': bad['materials'] *= 3
            else: bad['unknown'] = 1
            with self.subTest(change=change), self.assertRaises(ValueError): validate_recipe(bad)


@unittest.skipUnless(os.environ.get('EMBERVALE_ART_EDITOR'), 'Set EMBERVALE_ART_EDITOR for real LibreSprite integration')
class RealExporterTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='embervale-modular-test-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.pilot = self.root / 'pilot'
        shutil.copytree(PILOT, self.pilot)
        self.editor = Path(os.environ['EMBERVALE_ART_EDITOR'])

    def test_reposed_body_preserves_head_feet_and_repeat_export_preserves_every_master(self):
        _, _, legacy = png_pixels(ROOT / 'assets/embervale/woodcutting-pilot/textures/player.png')
        w, h, body = png_pixels(self.pilot / 'textures/body.png')
        self.assertEqual((w, h), (768, 96))
        self.assertNotEqual(body, legacy[192*768*4:288*768*4])
        recipe = json.loads((self.pilot / 'recipe.json').read_text())
        for p, frame in enumerate(recipe['frames']):
            for y,x0,x1 in frame['foreground']['head']:
                for x in range(x0,x1+1):
                    at=(y*768+p*96+x)*4
                    self.assertEqual(body[at:at+4], legacy[192*768*4+at:192*768*4+at+4])
        self.assertEqual(body[56*768*4:], legacy[248*768*4:288*768*4])
        before = inventory(self.pilot)
        build(self.editor, self.pilot)
        self.assertEqual(inventory(self.pilot), before)
        self.assertEqual(check(self.pilot)['validation']['bodyAndHandleMismatchPixels'], 0)

    def test_palette_only_edit_leaves_other_tiers_and_wood_unchanged(self):
        recipe = json.loads((self.pilot / 'recipe.json').read_text())
        before_tools = png_pixels(self.pilot / 'textures/tools.png')[2]
        before_composites = png_pixels(self.pilot / 'textures/composites.png')[2]
        before_body = (self.pilot / 'textures/body.png').read_bytes()
        masters = {name: (self.pilot / name).read_bytes() for name in SOURCES}
        # Bronze midtone is deliberately also the handle's RGB. A global colour
        # replacement would wrongly recolour the protected wooden shaft.
        self.assertEqual(recipe['materials'][0]['metal'][1], 'b27c48')
        recipe['materials'][0]['metal'][1] = 'ee00cc'
        (self.pilot / 'recipe.json').write_text(json.dumps(recipe), encoding='utf-8')
        build(self.editor, self.pilot)
        after_tools = png_pixels(self.pilot / 'textures/tools.png')[2]
        after_composites = png_pixels(self.pilot / 'textures/composites.png')[2]
        row_bytes = 768 * 96 * 4
        self.assertEqual(after_tools[row_bytes:], before_tools[row_bytes:])
        self.assertEqual(after_composites[row_bytes:], before_composites[row_bytes:])
        self.assertNotEqual(after_composites[:row_bytes], before_composites[:row_bytes])
        self.assertIn(bytes.fromhex('b27c48ff'), after_tools[:row_bytes])
        for i in range(0, row_bytes, 4):
            if before_tools[i:i+4] != after_tools[i:i+4]:
                self.assertEqual(before_tools[i:i+4], bytes.fromhex('b27c48ff'))
                self.assertEqual(after_tools[i:i+4], bytes.fromhex('ee00ccff'))
        self.assertEqual((self.pilot / 'textures/body.png').read_bytes(), before_body)
        self.assertEqual({name: (self.pilot / name).read_bytes() for name in SOURCES}, masters)

    def test_tool_clears_the_entire_body_except_both_grips_in_every_exported_pose(self):
        build(self.editor, self.pilot)
        recipe = json.loads((self.pilot / 'recipe.json').read_text())
        _, _, body = png_pixels(self.pilot / 'textures/body.png')
        _, _, tools = png_pixels(self.pilot / 'textures/tools.png')
        _, _, combined = png_pixels(self.pilot / 'textures/composites.png')
        def pixel(data, pose, x, y, tier=0):
            at = ((tier * 96 + y) * 768 + pose * 96 + x) * 4
            return data[at:at+4]
        for tier in range(3):
            for p, frame in enumerate(recipe['frames']):
                foreground = {(x,y) for spans in frame['foreground'].values()
                              for y,x0,x1 in spans for x in range(x0,x1+1)}
                grips = {(x,y) for role in ('main_hand','off_hand')
                         for y,x0,x1 in frame['foreground'][role] for x in range(x0,x1+1)}
                head = {(x,y) for y,x0,x1 in frame['foreground']['head'] for x in range(x0,x1+1)}
                for role in ('main_hand','off_hand'):
                    x,y=map(round,frame[role])
                    self.assertEqual(pixel(body,p,x,y).hex(), 'edbd89ff')
                    self.assertEqual(pixel(combined,p,x,y,tier), pixel(body,p,x,y))
                for y in range(96):
                    for x in range(96):
                        b, t = pixel(body,p,x,y), pixel(tools,p,x,y,tier)
                        # Inspect the unmasked tool. Hiding its middle with a
                        # foreground mask must never count as body clearance.
                        if b[3] and t[3]:
                            self.assertIn((x,y), grips,
                                          f'Tool passes through body: material {tier}, pose {p}, ({x},{y})')
                            self.assertNotIn((x,y), head)
                        expected = b if (x,y) in foreground or not t[3] else t
                        self.assertEqual(pixel(combined,p,x,y,tier), expected,
                                         f'Wrong occlusion: material {tier}, pose {p}, ({x},{y})')
            for y in range(96):
                for x in range(96):
                    self.assertEqual(pixel(combined,0,x,y,tier), pixel(combined,7,x,y,tier))

    def test_the_owner_rejected_mask_only_pose_is_refused_before_output_replacement(self):
        archive = self.pilot / 'reports/rejected-masking'
        shutil.copyfile(archive / 'recipe.json', self.pilot / 'recipe.json')
        shutil.copyfile(archive / 'body.ase', self.pilot / 'source/body.ase')
        validate_recipe(json.loads((self.pilot / 'recipe.json').read_text()))
        before = inventory(self.pilot)
        with self.assertRaisesRegex(RuntimeError, 'Tool intersects body outside the two grip regions'):
            build(self.editor, self.pilot)
        self.assertEqual(inventory(self.pilot), before)

    def test_foreground_mask_on_empty_body_pixel_cannot_replace_usable_outputs(self):
        path = self.pilot / 'recipe.json'
        recipe = json.loads(path.read_text())
        recipe['frames'][0]['foreground']['head'][0] = [0, 0, 0]
        validate_recipe(recipe)  # Structurally valid; decoded artwork must reject.
        path.write_text(json.dumps(recipe), encoding='utf-8')
        before = inventory(self.pilot)
        with self.assertRaisesRegex(RuntimeError, 'Foreground mask references empty body pixels'):
            build(self.editor, self.pilot)
        self.assertEqual(inventory(self.pilot), before)

    def test_rejected_recipe_and_bootstrap_preserve_export(self):
        recipe_path = self.pilot / 'recipe.json'
        before = inventory(self.pilot)
        with self.assertRaises(ValueError): build(self.editor, self.pilot, import_legacy=True)
        self.assertEqual(inventory(self.pilot), before)
        bad = json.loads(recipe_path.read_text()); bad['frames'].pop()
        recipe_path.write_text(json.dumps(bad), encoding='utf-8')
        invalid = inventory(self.pilot)
        with self.assertRaises(ValueError): build(self.editor, self.pilot)
        self.assertEqual(inventory(self.pilot), invalid)

    def test_text_line_endings_are_portable_without_relaxing_binary_hashes(self):
        for name in ('recipe.json', 'previews/validation.json'):
            path = self.pilot / name
            original = path.read_bytes().replace(b'\r\n', b'\n')
            for data in (original, original.replace(b'\n', b'\r\n')):
                path.write_bytes(data)
                check(self.pilot)
        image = self.pilot / 'textures/axe.png'
        image.write_bytes(image.read_bytes() + b'changed')
        with self.assertRaises(ValueError): check(self.pilot)

    def test_real_invalid_semantic_mask_cannot_replace_usable_outputs(self):
        script = self.root / 'bad-mask.js'
        target = self.pilot / 'source/metal-mask.ase'
        script.write_text('var s=app.open(' + json.dumps(target.as_posix()) + ').sprite;'
                          's.layer(0).cel(0).image.putPixel(16,12,app.pixelColor.rgba(255,0,255,255));'
                          's.commit();s.saveAs(' + json.dumps(target.as_posix()) + ',false);', encoding='utf-8')
        subprocess.run([str(self.editor), '-b', '--script', str(script)], cwd=self.editor.parent,
                       check=True, capture_output=True, timeout=30)
        before = inventory(self.pilot)
        with self.assertRaises(RuntimeError): build(self.editor, self.pilot)
        self.assertEqual(inventory(self.pilot), before)


if __name__ == '__main__':
    unittest.main()
