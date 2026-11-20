#include "WorldArtResources.h"
#include "Generated/WorldArtLimits.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <zlib.h>

namespace WorldArtResources
{
    namespace
    {
        [[noreturn]] void Fail(std::string_view message)
        { throw std::runtime_error("world art: " + std::string(message)); }

        bool Token(std::string_view text)
        {
            return !text.empty() && text.size() <= 128 && text != "." && text != ".." &&
                std::all_of(text.begin(), text.end(), [](char c) {
                    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
                });
        }
        bool Directory(std::string_view path)
        {
            constexpr std::string_view start = "assets/embervale/", end = "/textures";
            return path.starts_with(start) && path.ends_with(end) && path.size() > start.size() + end.size() &&
                Token(path.substr(start.size(), path.size() - start.size() - end.size()));
        }
        void Add(std::uint64_t &total, std::uint64_t amount, std::uint64_t limit, std::string_view message)
        {
            if (total > limit || amount > limit - total) Fail(message);
            total += amount;
        }
        bool Pixel(double value) { return std::isfinite(value) && value == std::floor(value); }
        std::uint32_t Number(std::span<const unsigned char> bytes, std::size_t offset)
        {
            return (std::uint32_t(bytes[offset]) << 24) | (std::uint32_t(bytes[offset + 1]) << 16) |
                (std::uint32_t(bytes[offset + 2]) << 8) | bytes[offset + 3];
        }
        std::uint32_t Crc(std::span<const unsigned char> bytes)
        {
            std::uint32_t crc = 0xffffffffU;
            for (const auto byte : bytes)
            {
                crc ^= byte;
                for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((0U - (crc & 1U)) & 0xedb88320U);
            }
            return ~crc;
        }
        struct Binding { std::string_view prefix; int variants, poses; bool action; };
        constexpr std::array bindings{
            Binding{"character.player.", 1, 8, true}, Binding{"character.woodcutting.", 1, 8, true},
            Binding{"character.mining.", 1, 6, true}, Binding{"tool.axe.", 3, 8, true},
            Binding{"tool.pickaxe.", 3, 6, true},
            Binding{"character.melee.", 5, 16, true}, Binding{"equipment.melee.", 9, 16, true},
            Binding{"character.melee.", 5, 16, true}, Binding{"equipment.melee.", 9, 16, true}, Binding{"variation.grass.", 162, 1, false},
            Binding{"variation.paths.", 204, 1, false}, Binding{"variation.water.", 48, 1, false},
            Binding{"variation.stone.", 12, 1, false}, Binding{"variation.flora.", 40, 1, false},
            Binding{"variation.walls.", 96, 1, false}, Binding{"variation.roof.", 72, 1, false},
            Binding{"variation.floors.", 9, 1, false}, Binding{"variation.trees.", 3, 1, false},
            Binding{"variation.stumps.", 3, 1, false}, Binding{"variation.rocks.", 9, 1, false}
        };
    }

    const Limits &DefaultLimits() { return WorldArtDefaultLimits; }

    Cost MeasureImage(int width, int height, const Limits &limits)
    {
        if (width <= 0 || height <= 0 || std::uint64_t(width) > limits.imageEdge ||
            std::uint64_t(height) > limits.imageEdge) Fail("image dimensions exceed the positive edge limit");
        const auto pixels = std::uint64_t(width) * std::uint64_t(height);
        if (pixels > limits.imagePixels || pixels > limits.textureBytes / 4 || pixels > limits.alphaBytes)
            Fail("image exceeds the pixel/texture/alpha budget");
        const auto rgba = pixels * 4;
        const auto scratch = std::max(rgba, PreflightScratchBytes);
        if (scratch > limits.stagingBytes || limits.pngBytes > limits.stagingBytes - scratch)
            Fail("image exceeds the encoded plus decoded staging budget");
        return {pixels, rgba, pixels, scratch + limits.pngBytes};
    }

    Cost ValidateCatalogue(std::span<const Image> images, std::span<const Frame> frames, const Limits &limits)
    {
        if (images.empty() || images.size() > limits.images || frames.empty() || frames.size() > limits.frames)
            Fail("catalogue image/frame count exceeds limits or is empty");
        Cost total;
        std::set<std::pair<std::string_view, std::string_view>> paths;
        for (const auto &image : images)
        {
            if (!Token(image.filename) || !image.filename.ends_with(".png") || !Directory(image.directory) ||
                !paths.emplace(image.directory, image.filename).second) Fail("invalid or duplicate texture path");
            const auto cost = MeasureImage(image.width, image.height, limits);
            Add(total.textureBytes, cost.textureBytes, limits.textureBytes, "catalogue exceeds texture budget");
            Add(total.alphaBytes, cost.alphaBytes, limits.alphaBytes, "catalogue exceeds alpha budget");
            total.pixels += cost.pixels;
            total.stagingBytes = std::max(total.stagingBytes, cost.stagingBytes);
        }
        std::set<std::string_view> ids;
        std::array<std::set<int>, bindings.size()> slots;
        for (const auto &frame : frames)
        {
            if (!Token(frame.id) || !ids.insert(frame.id).second) Fail("invalid or duplicate frame ID");
            if (frame.image >= images.size()) Fail("frame references an unknown image");
            const auto &image = images[frame.image];
            const auto [x, y, w, h] = frame.rect;
            if (!std::all_of(frame.rect.begin(), frame.rect.end(), Pixel) || x < 0 || y < 0 || w <= 0 || h <= 0 ||
                w > image.width || h > image.height || x > image.width - w || y > image.height - h)
                Fail("frame rectangle is outside its texture");
            if (!Pixel(frame.pivot[0]) || !Pixel(frame.pivot[1]) || frame.pivot[0] < 0 || frame.pivot[1] < 0 ||
                frame.pivot[0] > w || frame.pivot[1] > h) Fail("frame pivot is outside its rectangle");
            if (!std::all_of(frame.ground.begin(), frame.ground.end(), [](double n) { return Pixel(n) && n >= 0 && n <= 32; }))
                Fail("frame ground point is outside its tile");
            if (frame.connectionMask < -1 || frame.connectionMask > 15 || frame.variant < -1 || frame.variant > 4095 ||
                frame.facing < -1 || frame.facing > 7 || frame.pose < 0 || frame.pose > 255)
                Fail("frame selector is outside supported bounds");
            for (std::size_t i = 0; i < bindings.size(); ++i)
            {
                const auto &binding = bindings[i];
                if (!frame.id.starts_with(binding.prefix)) continue;
                int slot = frame.variant;
                if (binding.action)
                {
                    const int variant = binding.variants == 1 ? 0 : frame.variant;
                    if (frame.facing < 0 || frame.pose >= binding.poses || variant < 0 || variant >= binding.variants)
                        Fail("action frame cannot bind to the runtime table");
                    slot = (variant * 8 + frame.facing) * binding.poses + frame.pose;
                }
                else if (slot < 0 || slot >= binding.variants) Fail("variant cannot bind to the runtime table");
                if (!slots[i].insert(slot).second) Fail("duplicate runtime frame binding");
            }
        }
        for (std::size_t i = 0; i < bindings.size(); ++i)
        {
            const auto &b = bindings[i];
            if (!slots[i].empty() && slots[i].size() != std::size_t(b.variants * (b.action ? 8 : 1) * b.poses))
                Fail("incomplete runtime action/variant table");
        }
        return total;
    }

    void ValidatePng(std::span<const unsigned char> bytes, const Image &expected, const Limits &limits)
    {
        (void)MeasureImage(expected.width, expected.height, limits);
        constexpr std::array<unsigned char, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
        if (bytes.size() < 57 || bytes.size() > limits.pngBytes ||
            !std::equal(signature.begin(), signature.end(), bytes.begin())) Fail("invalid or oversized PNG");
        bool header = false, data = false, ended = false;
        struct Inflater
        {
            z_stream stream{};
            Inflater() { if (inflateInit(&stream) != Z_OK) Fail("cannot initialize bounded PNG inflate"); }
            ~Inflater() { inflateEnd(&stream); }
        } inflater;
        auto &stream = inflater.stream;
        std::array<unsigned char, PreflightScratchBytes> output;
        const auto stride = std::uint64_t(expected.width) * 4 + 1;
        const auto expectedBytes = stride * expected.height;
        std::uint64_t produced = 0;
        bool complete = false;
        for (std::size_t at = 8; at < bytes.size();)
        {
            if (bytes.size() - at < 12) Fail("truncated PNG chunk");
            const auto size = Number(bytes, at);
            if (size > bytes.size() - at - 12) Fail("PNG chunk exceeds file bounds");
            const std::string_view type(reinterpret_cast<const char *>(bytes.data() + at + 4), 4);
            if (Crc(bytes.subspan(at + 4, std::size_t(size) + 4)) != Number(bytes, at + 8 + size))
                Fail("PNG chunk checksum mismatch");
            if (!header)
            {
                if (type != "IHDR" || size != 13 || Number(bytes, at + 8) != std::uint32_t(expected.width) ||
                    Number(bytes, at + 12) != std::uint32_t(expected.height)) Fail("PNG header dimensions do not match catalogue");
                if (bytes[at + 16] != 8 || bytes[at + 17] != 6 || bytes[at + 18] != 0 ||
                    bytes[at + 19] != 0 || bytes[at + 20] != 0) Fail("PNG must be non-interlaced RGBA8");
                header = true;
            }
            else if (type == "IDAT")
            {
                if (complete && size != 0) Fail("trailing PNG compressed data");
                stream.next_in = const_cast<unsigned char *>(bytes.data() + at + 8);
                stream.avail_in = size;
                while (stream.avail_in > 0)
                {
                    const auto count = static_cast<uInt>(std::min<std::uint64_t>(output.size(), expectedBytes - produced + 1));
                    stream.next_out = output.data(); stream.avail_out = count;
                    const auto inputBefore = stream.avail_in;
                    const int status = inflate(&stream, Z_NO_FLUSH);
                    const auto generated = count - stream.avail_out;
                    if (generated > expectedBytes - produced) Fail("PNG inflate exceeds declared dimensions");
                    const auto firstFilter = (stride - produced % stride) % stride;
                    for (auto i = firstFilter; i < generated; i += stride)
                        if (output[i] > 4) Fail("invalid PNG row filter");
                    produced += generated;
                    if (status == Z_STREAM_END)
                    {
                        if (stream.avail_in != 0) Fail("trailing PNG compressed data");
                        complete = true; break;
                    }
                    if (status != Z_OK || (generated == 0 && inputBefore == stream.avail_in))
                        Fail("invalid PNG compressed stream");
                }
                data = true;
            }
            else if (type == "IEND" && data && size == 0 && at + 12 == bytes.size()) ended = true;
            else Fail("unsupported PNG chunk or chunk order");
            at += std::size_t(size) + 12;
        }
        if (!header || !data || !ended || !complete || produced != expectedBytes)
            Fail("PNG is missing complete image data/end");
    }

    std::vector<unsigned char> ReadPng(const std::filesystem::path &path, const Image &expected, const Limits &limits)
    {
        (void)MeasureImage(expected.width, expected.height, limits);
        std::error_code error;
        const auto status = std::filesystem::symlink_status(path, error);
        if (error || !std::filesystem::is_regular_file(status)) Fail("PNG must be a regular non-symlink file");
        const auto size = std::filesystem::file_size(path, error);
        if (error || size > limits.pngBytes) Fail("PNG exceeds encoded byte limit");
        std::ifstream file(path, std::ios::binary);
        if (!file) Fail("cannot open PNG");
        std::vector<unsigned char> bytes;
        std::array<char, 8192> chunk;
        while (file)
        {
            file.read(chunk.data(), chunk.size());
            const auto count = static_cast<std::size_t>(file.gcount());
            if (count > limits.pngBytes - bytes.size()) Fail("PNG grew beyond encoded byte limit");
            bytes.insert(bytes.end(), chunk.begin(), chunk.begin() + count);
        }
        if (file.bad() || !file.eof()) Fail("PNG read failed");
        ValidatePng(bytes, expected, limits);
        return bytes;
    }
}
