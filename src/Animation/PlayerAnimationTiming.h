// Generated from assets/embervale/player-animation.json by tools/art/animation_common.py.
#pragma once
#include <array>

namespace PlayerAnimationTiming
{
    inline constexpr int WalkPoses = 8, MeleePoses = 8, ChoppingPoses = 8, MiningPoses = 6;
    inline constexpr std::array<int, 8> WalkMilliseconds{{110, 90, 100, 150, 110, 90, 100, 150}};
    inline constexpr int WalkCycleMilliseconds = 900;
    inline constexpr std::array<int, 8> ChoppingMilliseconds{{260, 200, 200, 80, 80, 80, 100, 200}};
    inline constexpr std::array<int, 6> MiningMilliseconds{{200, 200, 200, 200, 200, 200}};
    inline constexpr std::array<int, 4> WindupMilliseconds{{150, 220, 320, 280}};
    inline constexpr std::array<std::array<int, 5>, 4> RecoveryMilliseconds{{
        {{50, 40, 50, 50, 60}},
        {{70, 40, 70, 50, 70}},
        {{90, 60, 110, 80, 100}},
        {{80, 50, 90, 60, 80}}
    }};
}
