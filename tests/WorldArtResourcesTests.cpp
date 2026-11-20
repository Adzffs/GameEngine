#include "TestSupport.h"
#include "../src/Content/Assets/WorldArtResources.h"
#include <chrono>
#include <fstream>
#include <limits>
#include <zlib.h>

namespace
{
    template<class F> bool Reject(F operation)
    { try { operation(); } catch (const std::exception &) { return true; } return false; }
    std::vector<unsigned char> Png()
    {
        // Independent zlib/CRC fixture: one opaque red RGBA8 pixel.
        constexpr std::string_view hex = "89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c489"
            "0000000d49444154789c63f8cfc0f01f00050001ff89993d1d0000000049454e44ae426082";
        std::vector<unsigned char> bytes;
        const auto nibble = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
        for (std::size_t i = 0; i < hex.size(); i += 2) bytes.push_back((nibble(hex[i]) << 4) | nibble(hex[i + 1]));
        return bytes;
    }
    void Number(std::vector<unsigned char> &out, std::uint32_t n)
    { for (int shift : {24, 16, 8, 0}) out.push_back(static_cast<unsigned char>(n >> shift)); }
    void Chunk(std::vector<unsigned char> &out, std::string_view kind, std::span<const unsigned char> data)
    {
        Number(out, static_cast<std::uint32_t>(data.size()));
        const auto at = out.size(); out.insert(out.end(), kind.begin(), kind.end()); out.insert(out.end(), data.begin(), data.end());
        Number(out, static_cast<std::uint32_t>(crc32(0, out.data() + at, static_cast<uInt>(4 + data.size()))));
    }
    std::vector<unsigned char> Encoded(int width, int height, std::vector<unsigned char> raw, bool split = false,
        bool trailingStream = false)
    {
        uLongf size = compressBound(raw.size()); std::vector<unsigned char> compressed(size);
        if (compress2(compressed.data(), &size, raw.data(), raw.size(), Z_BEST_COMPRESSION) != Z_OK)
            throw std::runtime_error("fixture compression failed");
        compressed.resize(size);
        if (trailingStream) { const auto copy = compressed; compressed.insert(compressed.end(), copy.begin(), copy.end()); }
        std::vector<unsigned char> result{137, 80, 78, 71, 13, 10, 26, 10}, header;
        Number(header, width); Number(header, height); header.insert(header.end(), {8, 6, 0, 0, 0});
        Chunk(result, "IHDR", header);
        if (split) for (const auto byte : compressed) Chunk(result, "IDAT", std::span(&byte, 1));
        else Chunk(result, "IDAT", compressed);
        Chunk(result, "IEND", {});
        return result;
    }
}

int main()
{
    using namespace WorldArtResources;
    TestContext test;
    const Image image{"tiny.png", 1, 1, "assets/embervale/forest-pilot/textures"};
    const Frame frame{"test.sprite", 0, {0, 0, 1, 1}, {0, 0}, {16, 24}};
    const std::array images{image};
    const std::array frames{frame};
    const auto cost = ValidateCatalogue(images, frames);
    test.Expect(cost.textureBytes == 4 && cost.alphaBytes == 1 && cost.pixels == 1,
        "RGBA texture estimate and exact alpha storage are distinct");
    const auto bytes = Png();
    test.Expect(!Reject([&]{ ValidatePng(bytes, image); }), "Complete independent RGBA8 PNG passes preflight");
    auto streamed = image; streamed.width = streamed.height = 128;
    const auto streamedPng = Encoded(128, 128, std::vector<unsigned char>((128 * 4 + 1) * 128), true);
    test.Expect(!Reject([&]{ ValidatePng(streamedPng, streamed); }),
        "Streaming validation spans scratch-buffer and one-byte IDAT boundaries");
    const auto bomb = Encoded(1, 1, std::vector<unsigned char>(100000));
    test.Expect(Reject([&]{ ValidatePng(bomb, image); }), "Small declared dimensions cannot hide excessive decompressed data");
    const auto shortPixels = Encoded(1, 1, {0});
    test.Expect(Reject([&]{ ValidatePng(shortPixels, image); }), "Short decompressed image rejects");
    const auto invalidFilter = Encoded(1, 1, {5, 0, 0, 0, 255});
    test.Expect(Reject([&]{ ValidatePng(invalidFilter, image); }), "Unsupported PNG filter rejects before SDL");
    const auto trailing = Encoded(1, 1, {0, 0, 0, 0, 255}, false, true);
    test.Expect(Reject([&]{ ValidatePng(trailing, image); }), "A second compressed stream cannot hide behind a valid image");
    for (const auto size : std::array<std::array<int, 2>, 6>{{{0, 1}, {-1, 1}, {1, 0}, {4097, 1}, {4096, 4096},
            {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}}})
        test.Expect(Reject([&]{ MeasureImage(size[0], size[1]); }), "Unsafe dimensions reject before allocation");
    auto limits = DefaultLimits(); limits.stagingBytes = limits.pngBytes;
    test.Expect(Reject([&]{ MeasureImage(1, 1, limits); }), "Encoded-plus-decoded image obeys staging cap");
    auto other = image; other.filename = "other.png";
    std::array pair{image, other};
    limits = DefaultLimits(); limits.textureBytes = 4;
    test.Expect(Reject([&]{ ValidateCatalogue(pair, frames, limits); }), "Individually valid images cannot exceed aggregate texture cap");
    limits = DefaultLimits(); limits.alphaBytes = 1;
    test.Expect(Reject([&]{ ValidateCatalogue(pair, frames, limits); }), "Aggregate CPU alpha cap is independently enforced");
    limits = DefaultLimits(); limits.images = 1;
    test.Expect(Reject([&]{ ValidateCatalogue(pair, frames, limits); }), "Image count is bounded");
    auto duplicate = frame; duplicate.id = "another.sprite";
    std::array doubled{frame, duplicate};
    limits = DefaultLimits(); limits.frames = 1;
    test.Expect(Reject([&]{ ValidateCatalogue(images, doubled, limits); }), "Frame count is bounded");
    doubled[1].id = frame.id;
    test.Expect(Reject([&]{ ValidateCatalogue(images, doubled); }), "Duplicate frame IDs reject");
    pair[1] = image;
    test.Expect(Reject([&]{ ValidateCatalogue(pair, frames); }), "Duplicate texture paths reject");
    for (const auto name : {"../escape.png", "/absolute.png", "bad.bmp"})
    {
        auto bad = images; bad[0].filename = name;
        test.Expect(Reject([&]{ ValidateCatalogue(bad, frames); }), "Unsafe or unsupported texture leaf rejects");
    }
    auto badImages = images; badImages[0].directory = "assets/embervale/../textures";
    test.Expect(Reject([&]{ ValidateCatalogue(badImages, frames); }), "Texture directory traversal rejects");
    for (const auto rectangle : std::array<std::array<double, 4>, 6>{{{0, 0, 2, 1}, {-1, 0, 1, 1}, {0, 0, 0, 1},
        {0.5, 0, 1, 1}, {std::numeric_limits<double>::quiet_NaN(), 0, 1, 1},
        {0, 0, std::numeric_limits<double>::infinity(), 1}}})
    {
        auto bad = frames; bad[0].rect = rectangle;
        test.Expect(Reject([&]{ ValidateCatalogue(images, bad); }), "Invalid/non-finite atlas rectangle rejects");
    }
    auto badFrames = frames; badFrames[0].image = 1;
    test.Expect(Reject([&]{ ValidateCatalogue(images, badFrames); }), "Unknown image index rejects");
    badFrames = frames; badFrames[0].pivot = {2, 0};
    test.Expect(Reject([&]{ ValidateCatalogue(images, badFrames); }), "Out-of-frame pivot rejects");
    badFrames = frames; badFrames[0].ground = {33, 0};
    test.Expect(Reject([&]{ ValidateCatalogue(images, badFrames); }), "Out-of-tile ground point rejects");
    badFrames = frames; badFrames[0].pose = 256;
    test.Expect(Reject([&]{ ValidateCatalogue(images, badFrames); }), "Unbounded animation pose rejects");
    badFrames = frames; badFrames[0].id = "character.player.s.idle"; badFrames[0].facing = 0;
    test.Expect(Reject([&]{ ValidateCatalogue(images, badFrames); }), "Incomplete present action table rejects");
    std::vector<std::string> ids;
    std::vector<Frame> walking;
    ids.reserve(33); walking.reserve(33);
    for (int facing = 0; facing < 8; ++facing) for (int pose = 0; pose < 4; ++pose)
    {
        ids.push_back("character.player." + std::to_string(facing) + "." + std::to_string(pose));
        auto entry = frame; entry.id = ids.back(); entry.facing = facing; entry.pose = pose; walking.push_back(entry);
    }
    test.Expect(!Reject([&]{ ValidateCatalogue(images, walking); }), "Complete eight-way action table passes");
    auto alias = walking.front(); ids.push_back("character.player.alias"); alias.id = ids.back(); walking.push_back(alias);
    test.Expect(Reject([&]{ ValidateCatalogue(images, walking); }), "Different IDs cannot overwrite one runtime binding slot");
    auto broken = bytes; broken[45] ^= 1;
    test.Expect(Reject([&]{ ValidatePng(broken, image); }), "PNG chunk corruption rejects before decoder");
    broken = bytes; broken.resize(broken.size() - 3);
    test.Expect(Reject([&]{ ValidatePng(broken, image); }), "Truncated PNG rejects before decoder");
    broken = bytes; broken.push_back(0);
    test.Expect(Reject([&]{ ValidatePng(broken, image); }), "Data after PNG end rejects");
    auto mismatch = image; mismatch.width = 2;
    test.Expect(Reject([&]{ ValidatePng(bytes, mismatch); }), "Actual image dimensions must match catalogue");
    limits = DefaultLimits(); limits.pngBytes = bytes.size() - 1;
    test.Expect(Reject([&]{ ValidatePng(bytes, image, limits); }), "Encoded image cap applies before decode");
    const auto root = std::filesystem::temp_directory_path() / ("embervale-art-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    const auto file = root / "tiny.png";
    { std::ofstream output(file, std::ios::binary); output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size()); }
    test.Expect(ReadPng(file, image) == bytes, "Decoder input owns the exact bounded preflight bytes");
    test.Expect(Reject([&]{ ReadPng(root, image); }), "Directories cannot be decoded as images");
    std::error_code error;
    std::filesystem::create_symlink(file, root / "linked.png", error);
    if (!error) test.Expect(Reject([&]{ ReadPng(root / "linked.png", image); }), "Symbolic-link image rejects");
    test.Expect(ReadPng(file, image) == bytes, "Rejection preserves original input bytes");
    std::filesystem::remove_all(root, error);
    return test.Finish();
}
