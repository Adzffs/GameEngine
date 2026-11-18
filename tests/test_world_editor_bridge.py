"""Headless contract tests for the first Embervale graphical editor bridge."""

import hashlib
import json
import os
import tempfile
import textwrap
import unittest
from pathlib import Path

from embervale_world_editor import StudioBridge, StudioError

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
        history.append((name, (x, y), (int(sx), int(sy))))
        redone.clear()
        placements[name] = (kind, int(sx), int(sy))
        print('Moved', name)
    elif op == 'undo':
        if not history:
            print('Error: no undo available', file=sys.stderr)
            continue
        name, old, new = history.pop()
        redone.append((name, old, new))
        placements[name] = (placements[name][0], *old)
    elif op == 'redo':
        if not redone:
            print('Error: no redo available', file=sys.stderr)
            continue
        name, old, new = redone.pop()
        history.append((name, old, new))
        placements[name] = (placements[name][0], *new)
    elif op == 'save':
        target = pathlib.Path(line.rstrip('\n')[5:])
        if target.exists():
            print('Error: output already exists', file=sys.stderr)
        else:
            target.write_text('mock output', encoding='utf-8')
            print('Saved new draft:', target)
    elif op == 'list':
        print('Existing placements (only R and S can be moved):')
        for name, (kind, x, y) in sorted(placements.items()):
            print(f'{kind_char[kind]}  {name}  ({x}, {y})')
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
        self.tool = root / 'fake_world_move'
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

    def test_protects_invalid_action_and_history_limit(self):
        with self.assertRaisesRegex(StudioError, 'Invalid placement ID'):
            self.bridge.move('resource.tree.001\nquit', 1, 0)
        self.bridge.commands = ['status'] * 256
        with self.assertRaisesRegex(StudioError, '256-command'):
            self.bridge.undo()


if __name__ == '__main__':
    unittest.main()
