#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

// SDL-free preflight for the current world-art catalogue. Memory values account
// for logical RGBA8 textures, retained alpha and one encoded/decoded staging
// image; allocator, decoder internals and driver overhead are not measured.
namespace WorldArtResources
{
    inline constexpr std::uint64_t PreflightScratchBytes = 64 * 1024;
    struct Limits
    {
        std::uint64_t images, frames, imageEdge, imagePixels, pngBytes, manifestBytes;
        std::uint64_t cataloguePngBytes, textureBytes, alphaBytes, stagingBytes;
    };
    const Limits &DefaultLimits();

    struct Image
    {
        std::string_view filename;
        int width, height;
        std::string_view directory;
    };
    struct Frame
    {
        std::string_view id;
        std::size_t image;
        std::array<double, 4> rect;
        std::array<double, 2> pivot, ground;
        int connectionMask = -1, variant = -1, facing = -1, pose = 0;
    };
    struct Cost
    {
        std::uint64_t pixels = 0, textureBytes = 0, alphaBytes = 0, stagingBytes = 0;
    };

    Cost MeasureImage(int width, int height, const Limits &limits = DefaultLimits());
    // Reject bad paths/identities, duplicate IDs/binding slots, unsafe geometry,
    // incomplete present action/variation tables and aggregate budget overflow.
    Cost ValidateCatalogue(std::span<const Image> images, std::span<const Frame> frames,
        const Limits &limits = DefaultLimits());

    // The runtime contract is RGBA8, non-interlaced PNG with IHDR/IDAT/IEND only.
    // Chunk bounds/CRC/order, exact dimensions and bounded streaming inflate
    // (including row filter bytes) are checked before SDL decode.
    void ValidatePng(std::span<const unsigned char> bytes, const Image &expected,
        const Limits &limits = DefaultLimits());
    // Own bounded regular-file bytes, so the decoder consumes the same content
    // that passed preflight rather than reopening a mutable filename.
    std::vector<unsigned char> ReadPng(const std::filesystem::path &path,
        const Image &expected, const Limits &limits = DefaultLimits());
}
