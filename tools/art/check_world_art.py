"""Validate current world art without exporting or rewriting any artwork."""
import argparse
import json
from pathlib import Path
import sys

from world_art_resources import REPO, ArtError, load_limits, load_manifests, renderer_layout, validate_manifests, validate_renderer_layout

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Read-only validation (the default)')
    parser.add_argument('--summary', action='store_true', help='Print one concise build message instead of JSON')
    parser.add_argument('--root', type=Path, default=REPO, help='Repository or complete copied authoring tree')
    args = parser.parse_args()
    try:
        root = args.root.resolve()
        limits = load_limits(root)
        manifests = load_manifests(root, limits)
        report = validate_manifests(root, manifests, limits)
        validate_renderer_layout(root / 'src/Graphics/ForestPilotLayout.h',
                                 renderer_layout(manifests), limits['max_manifest_bytes'] * 8)
        report.update(policy=limits, renderer_layout_matches=True,
                      accounting='Logical RGBA8 textures + alpha cache + one encoded/decoded image. Excludes driver, allocator and decoder internals.')
        if args.summary:
            print(f"World art verified: {report['images']} textures, {report['frames']} frames; "
                  f"{report['rgba_texture_bytes']} RGBA estimate bytes + {report['alpha_bytes']} alpha bytes.")
        else:
            print(json.dumps(report, indent=2))
        return 0
    except (OSError, ArtError, KeyError, TypeError, OverflowError) as error:
        print(f'World art rejected: {error}', file=sys.stderr)
        return 1

if __name__ == '__main__':
    raise SystemExit(main())
