#pragma once

#include "AudioCue.h"
#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <vector>

class SdlAudioPlayer
{
public:
    static constexpr std::size_t MaximumVoices = 6;
    SdlAudioPlayer() = default;
    ~SdlAudioPlayer();
    SdlAudioPlayer(const SdlAudioPlayer &) = delete;
    SdlAudioPlayer &operator=(const SdlAudioPlayer &) = delete;

    bool Initialize(const std::filesystem::path &assetDirectory = {});
    void Shutdown();
    void ApplySettings(const AudioSettings &settings);
    bool Play(AudioCue cue);
    bool IsAvailable() const { return device != 0; }
    std::size_t GetLoadedClipCount() const;
    std::size_t GetActiveVoiceCount() const;

private:
    struct Clip { std::vector<Uint8> samples; };
    struct Voice { SDL_AudioStream *stream = nullptr; Uint64 busyUntil = 0; };
    std::array<std::array<Clip, 2>, AudioCues.size()> clips;
    std::array<Voice, MaximumVoices> voices;
    std::array<unsigned, AudioCues.size()> nextVariant{};
    std::array<Uint64, AudioCues.size()> nextAllowed{};
    SDL_AudioDeviceID device = 0;
    bool ownsAudioSubsystem = false;
    float effectsGain = 0.65f;
};
