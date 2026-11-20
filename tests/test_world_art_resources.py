"""Resource failures that must not reach decoding or rewrite accepted art."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zlib

ART_TOOLS = Path(__file__).resolve().parents[1] / 'tools/art'
sys.path.insert(0, str(ART_TOOLS))
import world_art_resources as art
import build_forest_pilot as builder

def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

def png(w=1, h=1, raw=b'\0\xff\0\0\xff', depth=8, color=6, compressed=None):
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, depth, color, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw) if compressed is None else compressed) + chunk(b'IEND', b''))

class WorldArtResourceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='embervale-art-unit-')
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.folder = self.root / 'assets/embervale/forest-pilot'
        (self.folder / 'textures').mkdir(parents=True)
        (self.folder / 'source').mkdir()
        self.image = self.folder / 'textures/tiny.png'
        self.image.write_bytes(png())
        self.source = self.folder / 'source/tiny.ase'
        self.source.write_bytes(b'private checksum fixture; editor validation is separate')
        self.limits = art.load_limits()
        self.batch = {'schema': 'embervale.authoring-batch', 'version': 1, 'id': 'forest-pilot', 'native_tile_size': 32,
                      'images': {'tiny': {'image': 'textures/tiny.png', 'source': 'source/tiny.ase', 'size': [1, 1],
                                 'image_sha256': hashlib.sha256(self.image.read_bytes()).hexdigest(),
                                 'source_sha256': hashlib.sha256(self.source.read_bytes()).hexdigest()}},
                      'assets': [{'id': 'test.sprite', 'image': 'tiny', 'rect': [0, 0, 1, 1], 'pivot': [0, 0]}]}

    def validate(self, batch=None, limits=None, files=True):
        return art.validate_manifests(self.root, [batch or self.batch], limits or self.limits, files)

    def test_valid_accounting_and_validation_are_read_only(self):
        before = {p: p.read_bytes() for p in self.folder.rglob('*') if p.is_file()}
        report = self.validate()
        self.assertEqual((report['images'], report['frames'], report['rgba_texture_bytes'], report['alpha_bytes']), (1, 1, 4, 1))
        self.assertEqual(report['peak_image_staging_bytes'], len(png()) + art.PNG_SCRATCH_BYTES)
        self.assertEqual(before, {p: p.read_bytes() for p in self.folder.rglob('*') if p.is_file()})

    def test_resource_policy_rejects_wrong_types_unknown_and_duplicate_keys(self):
        policy = self.root / 'assets/embervale/world-art-limits.json'
        for field, value in [('version', True), ('max_images', 0), ('max_frames', 1.5), ('max_image_edge', 1 << 32)]:
            bad = dict(self.limits); bad[field] = value; policy.write_text(json.dumps(bad))
            with self.subTest(field=field), self.assertRaises(art.ArtError): art.load_limits(self.root)
        policy.write_text('{"version":1,"version":1}')
        with self.assertRaises(art.ArtError): art.load_limits(self.root)
        bad = dict(self.limits, unexpected=1); policy.write_text(json.dumps(bad))
        with self.assertRaises(art.ArtError): art.load_limits(self.root)

    def test_image_dimensions_reject_noninteger_zero_and_excessive_values(self):
        for size in ([0, 1], [-1, 1], [True, 1], [1.0, 1], [4097, 1], [4096, 4096], [1 << 100, 1], [1]):
            bad = copy.deepcopy(self.batch); bad['images']['tiny']['size'] = size
            with self.subTest(size=size), self.assertRaises(art.ArtError): self.validate(bad, files=False)

    def test_frame_geometry_reference_and_metadata_types(self):
        for key, value in [('rect', [-1, 0, 1, 1]), ('rect', [0, 0, 2, 1]), ('rect', [0, 0, 0, 1]),
                           ('rect', [0, 0, True, 1]), ('rect', [0.5, 0, 1, 1]), ('pivot', [2, 0]),
                           ('intended_ground_point_in_tile', [33, 0]), ('image', 'missing'),
                           ('animation_frame', 256), ('animation_frame', True), ('facing', 8), ('connection_mask', 16)]:
            bad = copy.deepcopy(self.batch); bad['assets'][0][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(art.ArtError): self.validate(bad, files=False)

    def test_duplicate_identifiers_and_texture_paths_reject(self):
        bad = copy.deepcopy(self.batch); bad['assets'].append(copy.deepcopy(bad['assets'][0]))
        with self.assertRaises(art.ArtError): self.validate(bad, files=False)
        bad = copy.deepcopy(self.batch); bad['images']['alias'] = copy.deepcopy(bad['images']['tiny'])
        with self.assertRaises(art.ArtError): self.validate(bad, files=False)

    def test_path_traversal_absolute_and_injected_ids_reject(self):
        for key, value in [('image', '../escape.png'), ('image', '/absolute.png'), ('image', 'textures/../escape.png'),
                           ('image', 'textures/a\\escape.png'), ('source', 'source/../master.ase')]:
            bad = copy.deepcopy(self.batch); bad['images']['tiny'][key] = value
            with self.subTest(value=value), self.assertRaises(art.ArtError): self.validate(bad, files=False)
        bad = copy.deepcopy(self.batch); bad['assets'][0]['id'] = 'bad"; injected'
        with self.assertRaises(art.ArtError): self.validate(bad, files=False)

    def test_individual_and_aggregate_memory_limits_reject_before_files(self):
        bad = copy.deepcopy(self.batch)
        bad['images']['other'] = dict(bad['images']['tiny'], image='textures/other.png', source='source/other.ase')
        for key, value in [('max_images', 1), ('max_texture_bytes', 4), ('max_alpha_bytes', 1)]:
            limits = dict(self.limits); limits[key] = value
            with self.subTest(key=key), self.assertRaises(art.ArtError): self.validate(bad, limits, files=False)
        limits = dict(self.limits, max_staging_bytes=self.limits['max_png_bytes'])
        with self.assertRaises(art.ArtError): self.validate(limits=limits, files=False)
        limits = dict(self.limits, max_catalogue_png_bytes=len(png()) - 1)
        with self.assertRaises(art.ArtError): self.validate(limits=limits)

    def test_action_table_requires_complete_unique_slots(self):
        batch = copy.deepcopy(self.batch)
        batch['assets'] = [dict(self.batch['assets'][0], id=f'character.player.{facing}.{pose}',
                                facing=facing, animation_frame=pose) for facing in range(8) for pose in range(4)]
        self.assertEqual(self.validate(batch, files=False)['frames'], 32)
        incomplete = copy.deepcopy(batch); incomplete['assets'].pop()
        with self.assertRaises(art.ArtError): self.validate(incomplete, files=False)
        batch['assets'].append(dict(batch['assets'][0], id='character.player.alias'))
        with self.assertRaises(art.ArtError): self.validate(batch, files=False)

    def test_png_header_dimensions_format_crc_chunk_bounds_and_end(self):
        corrupt = bytearray(png()); corrupt[45] ^= 1
        bad_length = bytearray(png()); bad_length[8:12] = b'\xff\xff\xff\xff'
        bads = [png(w=2), png(depth=16), png(color=2), bytes(corrupt), bytes(bad_length), png()[:-3],
                png() + b'x', png()[:-12] + chunk(b'tEXt', b'unsupported') + chunk(b'IEND', b'')]
        for data in bads:
            with self.subTest(data=data[:24]), self.assertRaises(art.ArtError): art.validate_png(data, [1, 1], self.limits)

    def test_decompression_bomb_short_trailing_stream_and_invalid_filter_reject(self):
        for data in [png(raw=b'\0' * 100000), png(raw=b'\0'), png(raw=b'\5\0\0\0\xff'),
                     png(compressed=zlib.compress(b'\0\xff\0\0\xff')[:-1]),
                     png(compressed=zlib.compress(b'\0\xff\0\0\xff') * 2)]:
            with self.subTest(length=len(data)), self.assertRaises(art.ArtError): art.validate_png(data, [1, 1], self.limits)

    def test_png_and_master_checksums_must_match_without_rewriting(self):
        self.image.write_bytes(png(raw=b'\0\0\0\xff\xff'))
        before = self.image.read_bytes()
        with self.assertRaises(art.ArtError): self.validate()
        self.assertEqual(self.image.read_bytes(), before)
        self.image.write_bytes(png()); self.source.write_bytes(b'changed master')
        with self.assertRaises(art.ArtError): self.validate()
        self.assertEqual(self.source.read_bytes(), b'changed master')

    def test_symlink_input_and_overlong_reads_reject(self):
        original = self.folder / 'textures/original.png'; self.image.rename(original)
        try: self.image.symlink_to(original)
        except OSError: self.skipTest('Symbolic links unavailable on this platform')
        with self.assertRaises(art.ArtError): self.validate()
        with self.assertRaises(art.ArtError): art.read_regular(original, len(png()) - 1)

    def test_manifest_versions_and_frame_count_are_bounded(self):
        for key, value in [('version', True), ('version', 3), ('schema', 'other'), ('native_tile_size', 64)]:
            bad = copy.deepcopy(self.batch); bad[key] = value
            with self.subTest(key=key), self.assertRaises(art.ArtError): self.validate(bad, files=False)
        bad = copy.deepcopy(self.batch); bad['assets'].append(dict(bad['assets'][0], id='second.sprite'))
        with self.assertRaises(art.ArtError): self.validate(bad, dict(self.limits, max_frames=1), files=False)
        path = self.root / 'small.json'; path.write_bytes(b' ' * 33)
        with self.assertRaises(art.ArtError): art.json_file(path, 32)
        path.write_bytes(b'{"version":1,"version":1}')
        with self.assertRaises(art.ArtError): art.json_file(path, 4096)

    def test_bad_export_candidate_preserves_previous_renderer_binding(self):
        path = self.root / 'src/Graphics/ForestPilotLayout.h'; path.parent.mkdir(parents=True)
        path.write_text('previous validated binding')
        bad = copy.deepcopy(self.batch); bad['assets'][0]['rect'] = [0, 0, 2, 1]
        with mock.patch.object(builder, 'REPO', self.root), \
             mock.patch.object(art, 'load_limits', return_value=self.limits), \
             mock.patch.object(art, 'load_manifests', return_value=[self.batch]):
            with self.assertRaises(art.ArtError): builder.write_renderer_layout(bad)
        self.assertEqual(path.read_text(), 'previous validated binding')

    def test_current_package_cli_reports_memory_and_preserves_all_artifacts(self):
        paths = list((art.REPO / 'assets/embervale').rglob('*')) + [art.REPO / 'src/Graphics/ForestPilotLayout.h']
        def hashes():return {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
        before = hashes()
        result = subprocess.run([sys.executable, str(ART_TOOLS / 'check_world_art.py'), '--check'],
                                check=True, capture_output=True, text=True, timeout=20)
        report = json.loads(result.stdout)
        self.assertEqual((report['batches'], report['images'], report['frames']), (8, 30, 1175))
        self.assertEqual((report['rgba_texture_bytes'], report['alpha_bytes']), (11681792, 2920448))
        self.assertTrue(report['renderer_layout_matches'])
        self.assertEqual(before, hashes())

if __name__ == '__main__':
    unittest.main()
