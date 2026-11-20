#include "TestSupport.h"
#include "../src/Audio/SdlAudioPlayer.h"
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char **argv)
{
    TestContext test;
    SdlAudioPlayer player;
    if (argc > 1 && std::string(argv[1]) == "--unavailable")
    {
        test.Expect(!player.Initialize(), "Unavailable audio driver is a nonfatal initialization failure");
        test.Expect(!player.Play(AudioCue::HIT), "Unavailable audio safely drops cues");
        return test.Finish();
    }
    test.Expect(argc == 2 && player.Initialize(argv[1]), "Dummy device initializes real SDL streams");
    if (!player.IsAvailable()) return test.Finish();
    test.ExpectEqual(player.GetLoadedClipCount(), std::size_t{15}, "All original PCM variants load");
    test.Expect(player.Play(AudioCue::WOOD_IMPACT), "Valid cue supplies a real audio stream");
    test.Expect(!player.Play(AudioCue::WOOD_IMPACT), "Repeated cue respects its minimum interval");
    for (auto cue : {AudioCue::STONE_IMPACT, AudioCue::HIT, AudioCue::BLOCK,
                     AudioCue::MISS, AudioCue::CRAFT})
        test.Expect(player.Play(cue), "Independent bounded voice accepts a cue");
    test.ExpectEqual(player.GetActiveVoiceCount(), SdlAudioPlayer::MaximumVoices,
        "Playback saturates at the fixed voice budget");
    test.Expect(!player.Play(AudioCue::EQUIP), "Saturation drops instead of queuing extra latency");
    AudioSettings settings;
    settings.ToggleMute();
    player.ApplySettings(settings);
    test.ExpectEqual(player.GetActiveVoiceCount(), std::size_t{0}, "Muting clears pending playback");
    test.Expect(!player.Play(AudioCue::EAT), "Muted events are discarded");
    settings.ToggleMute();
    settings.SetEffectsVolume(0);
    player.ApplySettings(settings);
    test.Expect(!player.Play(AudioCue::EAT), "Zero effects volume discards playback");
    settings.SetEffectsVolume(50);
    player.ApplySettings(settings);
    test.Expect(player.Play(AudioCue::EAT), "Unmuted new event plays without a stale queue");
    test.Expect(!player.Play(AudioCue::COUNT), "Invalid cue is rejected");
    player.Shutdown();
    test.Expect(!player.IsAvailable(), "Audio resources shut down independently from graphics");

    const auto directory = std::filesystem::temp_directory_path() /
        ("embervale-audio-invalid-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directories(directory);
    {
        std::ofstream oversized(directory / "wood-1.wav", std::ios::binary);
        const std::string bytes(129 * 1024, 'x');
        oversized.write(bytes.data(), bytes.size());
        std::ofstream invalid(directory / "hit-1.wav"); invalid << "invalid WAV";
    }
    std::filesystem::copy_file(std::filesystem::path(argv[1]) / "wood-2.wav",
        directory / "wood-2.wav");
    test.Expect(player.Initialize(directory), "Missing/invalid optional files keep the device usable");
    test.ExpectEqual(player.GetLoadedClipCount(), std::size_t{1},
        "Only the valid alternate survives missing, invalid and oversized inputs");
    test.Expect(player.Play(AudioCue::WOOD_IMPACT), "A valid alternate replaces a missing first variant");
    test.Expect(!player.Play(AudioCue::HIT), "Rejected clip does not play");
    player.Shutdown();
    std::filesystem::remove_all(directory);
    return test.Finish();
}
