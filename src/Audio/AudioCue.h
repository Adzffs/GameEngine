#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

enum class AudioCue : std::size_t
{
    WOOD_IMPACT, STONE_IMPACT, SWING, HEAVY_SWING, HIT, BLOCK, MISS,
    GATHER, CRAFT, EQUIP, INVENTORY, EAT, COUNT
};

struct AudioCueDefinition
{
    const char *file;
    int variants;
    int minimumIntervalMs;
};

inline constexpr std::array<AudioCueDefinition, static_cast<std::size_t>(AudioCue::COUNT)> AudioCues{{
    {"wood", 2, 250}, {"stone", 2, 250}, {"swing", 1, 100},
    {"heavy-swing", 1, 100}, {"hit", 2, 100}, {"block", 1, 100},
    {"miss", 1, 100}, {"gather", 1, 180}, {"craft", 1, 120},
    {"equip", 1, 100}, {"inventory", 1, 90}, {"eat", 1, 150}
}};

class AudioSettings
{
public:
    int GetEffectsVolume() const { return effectsVolume; }
    void SetEffectsVolume(int percent) { effectsVolume = std::clamp(percent, 0, 100); }
    bool IsMuted() const { return muted; }
    void ToggleMute() { muted = !muted; }
    float GetGain() const { return muted ? 0.0f : effectsVolume / 100.0f; }

private:
    int effectsVolume = 65;
    bool muted = false;
};
