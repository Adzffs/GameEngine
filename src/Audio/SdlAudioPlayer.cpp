#include "SdlAudioPlayer.h"
#include "../Core/Logger.h"
#include <algorithm>
#include <string>

SdlAudioPlayer::~SdlAudioPlayer() { Shutdown(); }

void SdlAudioPlayer::Shutdown()
{
    for (auto &voice : voices)
    {
        if (voice.stream) SDL_DestroyAudioStream(voice.stream);
        voice = {};
    }
    if (device) SDL_CloseAudioDevice(device);
    device = 0;
    for (auto &variants : clips)
        for (auto &clip : variants) clip.samples.clear();
    if (ownsAudioSubsystem) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    ownsAudioSubsystem = false;
    nextAllowed.fill(0);
    nextVariant.fill(0);
}

bool SdlAudioPlayer::Initialize(const std::filesystem::path &assetDirectory)
{
    Shutdown();
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        Logger::Warn(std::string("Sound unavailable: ") + SDL_GetError());
        return false;
    }
    ownsAudioSubsystem = true;
    const SDL_AudioSpec input{SDL_AUDIO_S16LE, 1, 48000};
    device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &input);
    if (!device)
    {
        Logger::Warn(std::string("Sound unavailable: ") + SDL_GetError());
        Shutdown();
        return false;
    }
    for (auto &voice : voices)
    {
        voice.stream = SDL_CreateAudioStream(&input, nullptr);
        if (!voice.stream || !SDL_BindAudioStream(device, voice.stream))
        {
            Logger::Warn(std::string("Sound stream unavailable: ") + SDL_GetError());
            Shutdown();
            return false;
        }
        SDL_SetAudioStreamGain(voice.stream, effectsGain * 0.45f);
    }
    std::filesystem::path directory = assetDirectory;
    if (directory.empty())
    {
        const char *base = SDL_GetBasePath();
        directory = std::filesystem::path(base ? base : ".") /
            "assets/embervale/audio-pilot";
    }
    int unavailable = 0;
    for (std::size_t cue = 0; cue < AudioCues.size(); ++cue)
    {
        for (int variant = 0; variant < AudioCues[cue].variants; ++variant)
        {
            const auto file = directory /
                (std::string(AudioCues[cue].file) + "-" + std::to_string(variant + 1) + ".wav");
            std::error_code error;
            const auto size = std::filesystem::file_size(file, error);
            // Reject oversized optional input before asking SDL to allocate it.
            if (error || size > 128 * 1024) { ++unavailable; continue; }
            SDL_AudioSpec format{};
            Uint8 *data = nullptr;
            Uint32 bytes = 0;
            const bool loaded = SDL_LoadWAV(file.string().c_str(), &format, &data, &bytes);
            if (loaded && format.format == input.format && format.channels == 1 &&
                format.freq == 48000 && bytes >= 2 && bytes <= 96000 && bytes % 2 == 0)
                clips[cue][variant].samples.assign(data, data + bytes);
            else ++unavailable;
            SDL_free(data);
        }
    }
    if (unavailable)
        Logger::Warn("Optional sound clips unavailable or invalid: " + std::to_string(unavailable));
    if (!SDL_ResumeAudioDevice(device))
    {
        Logger::Warn(std::string("Sound playback unavailable: ") + SDL_GetError());
        Shutdown();
        return false;
    }
    ApplySettings(AudioSettings{});
    return true;
}

void SdlAudioPlayer::ApplySettings(const AudioSettings &settings)
{
    const float gain = settings.GetGain();
    if (effectsGain == gain) return;
    effectsGain = gain;
    for (auto &voice : voices)
    {
        if (!voice.stream) continue;
        SDL_SetAudioStreamGain(voice.stream, gain * 0.45f);
        if (gain == 0.0f)
        {
            SDL_ClearAudioStream(voice.stream);
            voice.busyUntil = 0;
        }
    }
    // A muted cue is discarded by Play; unmuting cannot resurrect it.
}

bool SdlAudioPlayer::Play(AudioCue cue)
{
    const auto index = static_cast<std::size_t>(cue);
    if (!device || effectsGain <= 0 || index >= AudioCues.size()) return false;
    const Uint64 now = SDL_GetTicks();
    if (now < nextAllowed[index]) return false;
    Voice *available = nullptr;
    for (auto &voice : voices)
        if (now >= voice.busyUntil && SDL_GetAudioStreamQueued(voice.stream) == 0 &&
            SDL_GetAudioStreamAvailable(voice.stream) == 0)
        { available = &voice; break; }
    if (!available) return false; // Drop overload instead of accumulating latency.
    unsigned variant = nextVariant[index] % AudioCues[index].variants;
    for (int tried = 0; tried < AudioCues[index].variants &&
            clips[index][variant].samples.empty(); ++tried)
        variant = (variant + 1) % AudioCues[index].variants;
    const auto &data = clips[index][variant].samples;
    if (data.empty()) return false;
    if (!SDL_PutAudioStreamData(available->stream, data.data(), static_cast<int>(data.size())) ||
        !SDL_FlushAudioStream(available->stream))
    {
        SDL_ClearAudioStream(available->stream);
        return false;
    }
    available->busyUntil = now + data.size() * 1000 / 96000 + 50;
    nextAllowed[index] = now + AudioCues[index].minimumIntervalMs;
    nextVariant[index] = variant + 1;
    return true;
}

std::size_t SdlAudioPlayer::GetLoadedClipCount() const
{
    std::size_t count = 0;
    for (const auto &variants : clips)
        for (const auto &clip : variants) count += !clip.samples.empty();
    return count;
}

std::size_t SdlAudioPlayer::GetActiveVoiceCount() const
{
    return std::count_if(voices.begin(), voices.end(), [](const Voice &voice)
    {
        return voice.stream && (voice.busyUntil > SDL_GetTicks() ||
            SDL_GetAudioStreamQueued(voice.stream) > 0 ||
            SDL_GetAudioStreamAvailable(voice.stream) > 0);
    });
}
