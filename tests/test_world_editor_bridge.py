"""Headless contract tests for the first Embervale graphical editor bridge."""

import hashlib
import json
import os
import tempfile
import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from embervale_world_editor import StudioBridge, StudioError, fitted_tile, scroll_fraction, tile_at

FAKE_TOOL = r'''#!/usr/bin/env python3
import hashlib
import json
import pathlib
import sys

_, flag, source, *extra = sys.argv
assert flag == '--session'
data = pathlib.Path(source).read_bytes()
digest = hashlib.sha256(data).hexdigest()
if extra and (extra != ['--expected-hash', digest]):
    print('CONTENT_CONFLICT: source SHA-256 differs from expected-hash', file=sys.stderr)
    sys.exit(4)
world = json.loads(data)
placements = {p['id']: (p['kind'], p['position']['x'], p['position']['y']) for p in world['placements']}
kind_char = {'resource': 'R', 'station': 'S', 'npc_spawn': 'N', 'monster_spawn': 'M'}
history = []
redone = []
terrain = world['terrain']['rows']
print('Embervale Studio — starter-world editing session')
print('Source SHA-256:', digest)
for line in sys.stdin:
    parts = line.rstrip('\n').split(' ')
    op = parts[0]
    if op == 'move':
        _, name, sx, sy = parts
        if name not in placements or int(sx) == 99:
            print('Error: walkable ground required', file=sys.stderr)
            continue
        kind, x, y = placements[name]
        if (int(sx), int(sy)) == (x, y):
            print('Unchanged', name)
            continue
        history.append(('move', name, (x, y), (int(sx), int(sy))))
        redone.clear()
        placements[name] = (kind, int(sx), int(sy))
        print('Moved', name)
    elif op == 'paint':
        _, sx, sy, glyph = parts
        x, y = int(sx), int(sy)
        if (not (0 <= y < len(terrain) and 0 <= x < len(terrain[y]))
                or glyph not in 'GDSW' or (x, y) == (0, 0)
                or any((x, y) == (px, py) for _, px, py in placements.values())):
            print('Error: paint rejected: occupied or invalid tile', file=sys.stderr)
            continue
        previous = terrain[y][x]
        if previous == glyph:
            print('Unchanged terrain at', (x, y))
            continue
        history.append(('paint', (x, y), previous, glyph))
        redone.clear()
        terrain[y] = terrain[y][:x] + glyph + terrain[y][x + 1:]
        print('Painted', (x, y), glyph)
    elif op == 'undo':
        if not history:
            print('Error: no undo available', file=sys.stderr)
            continue
        change = history.pop()
        redone.append(change)
        kind, target, old, new = change
        if kind == 'move':
            placements[target] = (placements[target][0], *old)
        else:
            x, y = target
            terrain[y] = terrain[y][:x] + old + terrain[y][x + 1:]
    elif op == 'redo':
        if not redone:
            print('Error: no redo available', file=sys.stderr)
            continue
        change = redone.pop()
        history.append(change)
        kind, target, old, new = change
        if kind == 'move':
            placements[target] = (placements[target][0], *new)
        else:
            x, y = target
            terrain[y] = terrain[y][:x] + new + terrain[y][x + 1:]
    elif op == 'save':
        target = pathlib.Path(line.rstrip('\n')[5:])
        if target.exists():
            print('Error: output already exists', file=sys.stderr)
        else:
            authored = dict(world)
            authored['placements'] = [
                {**p, 'position': {'x': placements[p['id']][1],
                                    'y': placements[p['id']][2]}}
                for p in world['placements']
            ]
            authored['terrain'] = {'rows': terrain}
            content = json.dumps(authored, sort_keys=True).encode('utf-8')
            target.write_bytes(content)
            print('Saved new draft:', target)
            print('draft-sha256:', hashlib.sha256(content).hexdigest())
    elif op == 'list':
        print('Existing placements (only R and S can be moved):')
        for name, (kind, x, y) in sorted(placements.items()):
            print(f'{kind_char[kind]}  {name}  ({x}, {y})')
    elif op == 'terrain':
        print('Terrain rows (GDSW):')
        for row in terrain:
            print(row)
        print('End terrain rows')
    elif op == 'status':
        print(f'Changed: yes; undo: {len(history)}; redo: {len(redone)}; original SHA-256: {digest}')
    elif op == 'quit':
        print('Session closed.')
        break
'''


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        root = Path(self.tmp.name)
        self.tool = root / 'fake_world_move.py'
        self.tool.write_text(FAKE_TOOL, encoding='utf-8')
        os.chmod(self.tool, 0o755)
        self.source = root / 'source.json'
        self.world = {
            'size': {'width': 4, 'height': 3},
            'terrain': {'rows': ['GGGG', 'GGWG', 'GGGG']},
            'player_spawn': {'x': 0, 'y': 0},
            'placements': [
                {'id': 'resource.tree.001', 'definition_id': 'tree', 'kind': 'resource', 'position': {'x': 1, 'y': 1}},
                {'id': 'npc.guide.001', 'definition_id': 'guide', 'kind': 'npc_spawn', 'position': {'x': 2, 'y': 2}}
            ]
        }
        self.source.write_text(json.dumps(self.world), encoding='utf-8')
        self.bridge = StudioBridge(self.tool, self.source)

    def test_move_undo_redo_authoritatively(self):
        self.bridge.move('resource.tree.001', 3, 1)
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (3, 1))
        self.bridge.undo()
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))
        self.bridge.redo()
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (3, 1))
        self.assertEqual(len(self.bridge.commands), 3)
        self.assertEqual(json.loads(self.source.read_text()), self.world)

    def test_failed_move_does_not_pollute_history(self):
        with self.assertRaisesRegex(StudioError, 'outside the starter world'):
            self.bridge.move('resource.tree.001', 99, 1)
        self.assertEqual(self.bridge.commands, [])
        with self.assertRaisesRegex(StudioError, 'walkable ground'):
            self.bridge.perform('move resource.tree.001 99 1')
        self.assertEqual(self.bridge.commands, [])

    def test_rejected_backend_operation_is_transactional(self):
        with self.assertRaisesRegex(StudioError, 'no undo available'):
            self.bridge.undo()
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))
        with self.assertRaisesRegex(StudioError, 'Only existing resource'):
            self.bridge.move('npc.guide.001', 1, 0)

    def test_source_change_conflicts_without_saving(self):
        self.source.write_text(json.dumps(self.world) + '\n', encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'CONTENT_CONFLICT'):
            self.bridge.move('resource.tree.001', 3, 1)
        self.assertEqual(self.bridge.commands, [])
        draft = self.source.with_name('draft.json')
        with self.assertRaisesRegex(StudioError, 'CONTENT_CONFLICT'):
            self.bridge.save(draft)
        self.assertFalse(draft.exists())

    def test_save_supports_spaces_and_stays_read_only(self):
        draft = self.source.with_name('new map draft.json')
        self.assertEqual(self.bridge.save(draft), str(draft))
        self.assertTrue(draft.exists())
        with self.assertRaisesRegex(StudioError, 'already exists'):
            self.bridge.save(draft)
        self.assertEqual(json.loads(self.source.read_text()), self.world)


    def test_save_rejects_wrong_backend_hash_without_accepting_draft(self):
        original = "print('draft-sha256:', hashlib.sha256(content).hexdigest())"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original, "print('draft-sha256:', '0' * 64)"), encoding='utf-8')
        draft = self.source.with_name('bad receipt.json')
        with self.assertRaisesRegex(StudioError, 'SHA-256 differs'):
            self.bridge.save(draft)
        self.assertTrue(draft.exists())  # Do not delete/rewrite an uncertain file.
        self.assertEqual(self.bridge.commands, [])

    def test_save_rejects_wrong_backend_destination(self):
        original = "print('Saved new draft:', target)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original, "print('Saved new draft:', target.with_name('unexpected.json'))"),
            encoding='utf-8')
        draft = self.source.with_name('requested.json')
        with self.assertRaisesRegex(StudioError, 'did not confirm the requested'):
            self.bridge.save(draft)
        self.assertTrue(draft.exists())
        self.assertEqual(self.bridge.commands, [])

    def test_save_rejects_missing_backend_file(self):
        original = "target.write_bytes(content)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(original, "pass"), encoding='utf-8')
        draft = self.source.with_name('not written.json')
        with self.assertRaisesRegex(StudioError, 'regular draft file'):
            self.bridge.save(draft)
        self.assertFalse(draft.exists())
        self.assertEqual(self.bridge.commands, [])


    def test_save_readback_matches_moved_and_painted_world(self):
        self.bridge.move('resource.tree.001', 3, 1)
        self.bridge.paint(1, 0, 'D')
        draft = self.source.with_name('edited draft.json')
        self.bridge.save(draft)
        authored = json.loads(draft.read_text(encoding='utf-8'))
        self.assertEqual(authored['placements'][0]['position'], {'x': 3, 'y': 1})
        self.assertEqual(authored['terrain']['rows'][0], 'GDGG')
        self.assertEqual(self.bridge.commands, ['move resource.tree.001 3 1', 'paint 1 0 D'])

    def test_save_rejects_correctly_hashed_wrong_terrain(self):
        original = "authored['terrain'] = {'rows': terrain}"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original, "authored['terrain'] = {'rows': ['DDDD', *terrain[1:]]}"),
            encoding='utf-8')
        draft = self.source.with_name('unexpected terrain.json')
        with self.assertRaisesRegex(StudioError, 'Saved draft differs'):
            self.bridge.save(draft)
        self.assertTrue(draft.exists())
        self.assertEqual(self.bridge.commands, [])

    def test_save_rejects_correctly_hashed_wrong_placement(self):
        original = "authored['terrain'] = {'rows': terrain}"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original,
            "authored['placements'][0]['position']['x'] = 0\\n            authored['terrain'] = {'rows': terrain}".replace('\\n', '\n')),
            encoding='utf-8')
        draft = self.source.with_name('unexpected placement.json')
        with self.assertRaisesRegex(StudioError, 'Saved draft differs'):
            self.bridge.save(draft)
        self.assertTrue(draft.exists())
        self.assertEqual(self.bridge.commands, [])

    def test_save_rejects_correctly_hashed_changed_spawn(self):
        original = "authored = dict(world)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original,
            "authored = dict(world)\\n            authored['player_spawn'] = {'x': 2, 'y': 0}".replace('\\n', '\n')),
            encoding='utf-8')
        draft = self.source.with_name('unexpected spawn.json')
        with self.assertRaisesRegex(StudioError, 'Saved draft differs'):
            self.bridge.save(draft)
        self.assertTrue(draft.exists())
        self.assertEqual(self.bridge.commands, [])

    def test_replay_rejects_untracked_terminal_commands_before_any_side_effect(self):
        draft = self.source.with_name('untracked save.json')
        for command in (f'save {draft}', 'status', 'list', 'terrain',
                        'help', 'map 0 0', 'quit', 'exit',
                        'move resource.tree.001 3 1 extra', 'paint 1 0 X'):
            with self.subTest(command=command):
                with self.assertRaisesRegex(StudioError, 'Only move, paint, undo and redo'):
                    self.bridge.perform(command)
        self.assertFalse(draft.exists())
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))
        self.assertEqual(self.bridge.terrain, self.world['terrain']['rows'])
        # The rejected requests have not poisoned later legitimate history.
        self.bridge.paint(1, 0, 'D')
        self.bridge.undo()
        self.assertEqual(self.bridge.terrain, self.world['terrain']['rows'])

    def test_protects_invalid_action_and_history_limit(self):
        with self.assertRaisesRegex(StudioError, 'Invalid placement ID'):
            self.bridge.move('resource.tree.001\nquit', 1, 0)
        self.bridge.commands = ['status'] * 256
        with self.assertRaisesRegex(StudioError, '256-command'):
            self.bridge.undo()

    def test_paint_updates_display_only_after_authoritative_history(self):
        self.bridge.paint(1, 0, 'D')
        self.assertEqual(self.bridge.terrain[0], 'GDGG')
        self.assertEqual(self.bridge.status.split('/')[1:], ['1', '0'])
        # Repainting an identical tile is not an edit and must not add a command.
        self.bridge.paint(1, 0, 'D')
        self.assertEqual(len(self.bridge.commands), 1)
        self.bridge.undo()
        self.assertEqual(self.bridge.terrain[0], 'GGGG')
        self.bridge.redo()
        self.assertEqual(self.bridge.terrain[0], 'GDGG')
        self.assertEqual(json.loads(self.source.read_text()), self.world)

    def test_move_and_paint_share_undo_redo_and_branch_correctly(self):
        self.bridge.move('resource.tree.001', 3, 1)
        self.bridge.paint(1, 0, 'D')
        self.bridge.undo()
        self.assertEqual(self.bridge.terrain[0], 'GGGG')
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (3, 1))
        self.bridge.undo()
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))
        self.bridge.redo()
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (3, 1))
        self.bridge.paint(0, 1, 'S')
        self.assertEqual(self.bridge.terrain[1], 'SGWG')
        self.assertEqual(self.bridge.status.split('/')[1:], ['2', '0'])
        # A new paint after undo discards the redo branch in both histories.
        with self.assertRaisesRegex(StudioError, 'no redo available'):
            self.bridge.redo()
        self.assertEqual(self.bridge.commands[-1], 'paint 0 1 S')
        self.bridge.undo()
        self.assertEqual(self.bridge.terrain[1], 'GGWG')
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (3, 1))

    def test_requires_authoritative_terrain_response(self):
        self.tool.write_text(FAKE_TOOL.replace("elif op == 'terrain':", "elif op == 'unused':"), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'did not return authoritative terrain'):
            StudioBridge(self.tool, self.source)

    def test_rejects_incomplete_authoritative_terrain(self):
        self.tool.write_text(FAKE_TOOL.replace("print(row)", "print(row[:-1])"), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'malformed authoritative terrain'):
            StudioBridge(self.tool, self.source)

    def test_rejects_disagreement_with_original_source(self):
        self.tool.write_text(FAKE_TOOL.replace("print(row)", "print(row.replace('G', 'D'))"), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'terrain differs from the source snapshot'):
            StudioBridge(self.tool, self.source)

    def test_startup_rejects_backend_placement_position_drift(self):
        original = "print(f'{kind_char[kind]}  {name}  ({x}, {y})')"
        altered = "print(f'{kind_char[kind]}  {name}  ({x + 1}, {y})')"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(original, altered), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'placements differ from the source'):
            StudioBridge(self.tool, self.source)

    def test_startup_rejects_backend_placement_kind_drift(self):
        original = "print(f'{kind_char[kind]}  {name}  ({x}, {y})')"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original, "print(f'R  {name}  ({x}, {y})')"), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'placements differ from the source'):
            StudioBridge(self.tool, self.source)

    def test_edit_rejects_spawn_drift_transactionally(self):
        original = "print(f'{kind_char[kind]}  {name}  ({x}, {y})')"
        altered = (
            """print(f'{kind_char[kind]}  {name}  """
            """({x + (1 if history and kind == "npc_spawn" else 0)}, {y})')"""
        )
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(original, altered), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'read-only actor spawn'):
            self.bridge.paint(1, 0, 'D')
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.terrain, self.world['terrain']['rows'])
        self.assertEqual(self.bridge.placements['npc.guide.001'][1:], (2, 2))

    def test_edit_rejects_backend_kind_drift(self):
        original = "print(f'{kind_char[kind]}  {name}  ({x}, {y})')"
        altered = (
            """print(f'{"S" if history and kind == "resource" else kind_char[kind]}  """
            """{name}  ({x}, {y})')"""
        )
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(original, altered), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'placement kind'):
            self.bridge.paint(1, 0, 'D')
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][0], 'R')

    def test_edit_rejects_out_of_bounds_backend_position(self):
        original = "print(f'{kind_char[kind]}  {name}  ({x}, {y})')"
        altered = (
            """print(f'{kind_char[kind]}  {name}  """
            """({x + (100 if history and kind == "resource" else 0)}, {y})')"""
        )
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(original, altered), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'out-of-bounds placement'):
            self.bridge.paint(1, 0, 'D')
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))

    def test_rejected_paint_preserves_source_and_history(self):
        with self.assertRaisesRegex(StudioError, 'occupied or invalid tile'):
            self.bridge.paint(1, 1, 'W')
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.terrain, self.world['terrain']['rows'])
        with self.assertRaisesRegex(StudioError, 'Invalid terrain brush'):
            self.bridge.paint(1, 0, 'X')
        with self.assertRaisesRegex(StudioError, 'outside the map'):
            self.bridge.paint(50, 0, 'W')
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(json.loads(self.source.read_text()), self.world)


    def test_move_rejects_backend_that_ignores_destination(self):
        original = "placements[name] = (kind, int(sx), int(sy))"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(original, "placements[name] = (kind, x, y)"),
                             encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'unexpected move or ground'):
            self.bridge.move('resource.tree.001', 3, 1)
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))

    def test_move_rejects_accidental_ground_mutation(self):
        original = "print('Moved', name)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original, "terrain[0] = 'D' + terrain[0][1:]\\n        print('Moved', name)".replace('\\n', '\n')),
            encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'ground change'):
            self.bridge.move('resource.tree.001', 3, 1)
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.terrain, self.world['terrain']['rows'])

    def test_paint_rejects_unexpected_placement_move(self):
        original = "print('Painted', (x, y), glyph)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original,
            "placements['resource.tree.001'] = ('resource', 3, 1)\\n        print('Painted', (x, y), glyph)".replace('\\n', '\n')),
            encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'placement change'):
            self.bridge.paint(1, 0, 'D')
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))

    def test_rejects_inconsistent_backend_undo_count(self):
        original = "undo: {len(history)}; redo: {len(redone)};"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(FAKE_TOOL.replace(
            original, "undo: 0; redo: {len(redone)};"), encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'inconsistent undo/redo'):
            self.bridge.move('resource.tree.001', 3, 1)
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))

    def test_undo_rejects_wrong_earlier_world_revision(self):
        self.bridge.move('resource.tree.001', 3, 1)
        saved_commands = list(self.bridge.commands)
        original = "placements[target] = (placements[target][0], *old)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(
            FAKE_TOOL.replace(original, "placements[target] = (placements[target][0], 0, 2)"),
            encoding='utf-8')
        # Counts and identities remain legal; only exact observed-state parity catches this.
        with self.assertRaisesRegex(StudioError, 'verified history checkpoint'):
            self.bridge.undo()
        self.assertEqual(self.bridge.commands, saved_commands)
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (3, 1))
        self.assertEqual(len(self.bridge._checkpoints), 2)

    def test_redo_rejects_wrong_later_world_revision(self):
        self.bridge.move('resource.tree.001', 3, 1)
        self.bridge.undo()
        saved_commands = list(self.bridge.commands)
        original = "placements[target] = (placements[target][0], *new)"
        self.assertIn(original, FAKE_TOOL)
        self.tool.write_text(
            FAKE_TOOL.replace(original, "placements[target] = (placements[target][0], 0, 2)"),
            encoding='utf-8')
        with self.assertRaisesRegex(StudioError, 'verified history checkpoint'):
            self.bridge.redo()
        self.assertEqual(self.bridge.commands, saved_commands)
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))
        self.assertEqual(len(self.bridge._checkpoints), 2)

    def test_new_paint_after_undo_replaces_observed_redo_branch(self):
        self.bridge.move('resource.tree.001', 3, 1)
        self.bridge.undo()
        self.bridge.paint(1, 0, 'D')
        self.assertEqual(len(self.bridge._checkpoints), 2)
        self.assertEqual(self.bridge._checkpoints[1][1], ('GDGG', 'GGWG', 'GGGG'))
        self.assertEqual(self.bridge.placements['resource.tree.001'][1:], (1, 1))
        with self.assertRaisesRegex(StudioError, 'no redo available'):
            self.bridge.redo()


class ViewportMathTests(unittest.TestCase):
    def test_boundary_grid_after_zoom(self):
        self.assertEqual(tile_at(0, 0, 22, 100, 100), (0, 0))
        self.assertEqual(tile_at(21.9, 21.9, 22, 100, 100), (0, 0))
        self.assertEqual(tile_at(22, 44, 22, 100, 100), (1, 2))
        self.assertEqual(tile_at(2199, 2199, 22, 100, 100), (99, 99))
        for x, y in [(-0.01, 10), (10, -1), (2200, 0), (0, 2200)]:
            self.assertIsNone(tile_at(x, y, 22, 100, 100))
        with self.assertRaises(ValueError):
            tile_at(0, 0, 0, 100, 100)

    def test_scroll_anchor_is_clamped(self):
        self.assertEqual(scroll_fraction(2200, 800, -1), 0)
        self.assertAlmostEqual(scroll_fraction(2200, 800, 1100 - 400), 700 / 2200)
        self.assertAlmostEqual(scroll_fraction(2200, 800, 999999), 1400 / 2200)
        self.assertEqual(scroll_fraction(200, 800, 90), 0)
        self.assertEqual(scroll_fraction(0, 800, 90), 0)

    def test_fit_view_respects_extremes(self):
        self.assertEqual(fitted_tile(800, 600, 100, 100), 6)
        self.assertEqual(fitted_tile(800, 600, 10, 10), 60)
        self.assertEqual(fitted_tile(100, 100, 100, 100), 4)
        self.assertEqual(fitted_tile(10000, 10000, 10, 10), 64)
        with self.assertRaises(ValueError):
            fitted_tile(100, 100, 0, 10)


if __name__ == '__main__':
    unittest.main()
