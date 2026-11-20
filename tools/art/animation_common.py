"""Shared authored timing; GIF metadata changes never alter image pixels."""
import json
from pathlib import Path
import struct

REPO = Path(__file__).resolve().parents[2]


def load_timing():
    timing = json.loads((REPO / 'assets/embervale/player-animation.json').read_text(encoding='utf-8'))
    if any(timing[key] != 8 for key in ['walk_poses', 'melee_poses', 'chopping_poses']):
        raise ValueError('Current authored atlases require eight poses per action')
    if timing['mining_poses'] != 6:
        raise ValueError('Existing mining masters require six poses')
    for key in ['walk_preview_ms', 'chopping_ms', 'mining_ms']:
        if len(timing[key]) != (6 if key == 'mining_ms' else 8) or any(ms < 10 or ms % 10 for ms in timing[key]):
            raise ValueError('Animation timing must contain eight positive 10ms durations')
    if sum(timing['chopping_ms']) != 1200 or sum(timing['mining_ms']) != 1200:
        raise ValueError('Chopping must retain its 1200ms cosmetic rhythm')
    if [profile['name'] for profile in timing['profiles']] != ['unarmed', 'sword', 'heavy', 'tool']:
        raise ValueError('Unknown melee timing profile order')
    for profile in timing['profiles']:
        if len(profile['recovery_ms']) != 5 or any(ms < 10 or ms % 10 for ms in profile['recovery_ms']):
            raise ValueError('Each weapon requires five positive 10ms recovery durations')
    return timing


def write_timing_header():
    timing = load_timing()
    rows = ',\n'.join('        {{' + ', '.join(str(ms) for ms in profile['recovery_ms']) + '}}'
                      for profile in timing['profiles'])
    header = '''// Generated from assets/embervale/player-animation.json by tools/art/animation_common.py.
#pragma once
#include <array>

namespace PlayerAnimationTiming
{
    inline constexpr int WalkPoses = 8, MeleePoses = 8, ChoppingPoses = 8, MiningPoses = 6;
    inline constexpr std::array<int, 8> WalkMilliseconds{{%s}};
    inline constexpr int WalkCycleMilliseconds = %s;
    inline constexpr std::array<int, 8> ChoppingMilliseconds{{%s}};
    inline constexpr std::array<int, 6> MiningMilliseconds{{%s}};
    inline constexpr std::array<int, 4> WindupMilliseconds{{%s}};
    inline constexpr std::array<std::array<int, 5>, 4> RecoveryMilliseconds{{
%s
    }};
}
''' % (', '.join(map(str, timing['walk_preview_ms'])), sum(timing['walk_preview_ms']),
       ', '.join(map(str, timing['chopping_ms'])), ', '.join(map(str, timing['mining_ms'])),
       ', '.join(str(profile['windup_ms']) for profile in timing['profiles']), rows)
    target = REPO / 'src/Animation/PlayerAnimationTiming.h'
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists() or target.read_text(encoding='utf-8') != header:
        target.write_bytes(header.encode('utf-8'))


def retime_gif(path, durations):
    """Edit only GIF Graphic Control Extension delays, in centiseconds."""
    data = bytearray(path.read_bytes())
    if data[:6] not in (b'GIF89a', b'GIF87a'):
        raise ValueError('Not a GIF')
    cursor = 13 + (3 * (2 ** ((data[10] & 7) + 1)) if data[10] & 128 else 0)
    controls = []
    images = 0

    def skip_blocks(position):
        while data[position]:
            position += data[position] + 1
        return position + 1

    while cursor < len(data):
        marker = data[cursor]
        if marker == 0x3B:
            break
        if marker == 0x21:
            label = data[cursor + 1]
            if label == 0xF9:
                if data[cursor + 2] != 4:
                    raise ValueError('Invalid GIF control extension')
                controls.append(cursor + 4)
            cursor = skip_blocks(cursor + 2)
        elif marker == 0x2C:
            flags = data[cursor + 9]
            cursor += 10
            if flags & 128:
                cursor += 3 * (2 ** ((flags & 7) + 1))
            cursor = skip_blocks(cursor + 1)  # Skip LZW code size and data blocks.
            images += 1
        else:
            raise ValueError('Unknown GIF block')
    if images != len(durations) or len(controls) != images:
        raise ValueError('GIF timing does not match its frame count')
    for position, milliseconds in zip(controls, durations):
        struct.pack_into('<H', data, position, milliseconds // 10)
    path.write_bytes(data)


if __name__ == '__main__':
    write_timing_header()
