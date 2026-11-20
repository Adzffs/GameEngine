#include "TestSupport.h"
#include "../src/Graphics/ForestSprites.h"
#include <chrono>
#include <fstream>
#include <limits>

struct WorldArtResourceTestAccess
{
    static void Initialize(ForestSprites &cache, SDL_Renderer *renderer,
        std::span<const ForestPilotLayout::Image> images, std::span<const ForestPilotLayout::Frame> frames,
        const std::filesystem::path &root, const WorldArtResources::Limits &limits = WorldArtResources::DefaultLimits())
    { cache.InitializeCatalogue(renderer, images, frames, {root}, limits); }
};

int main()
{
    TestContext test;
    if (!SDL_Init(SDL_INIT_VIDEO)) { test.Expect(false, "Resource guard SDL initializes"); return test.Finish(); }
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Resource guards", 128, 128, SDL_WINDOW_HIDDEN, &window, &renderer))
    { test.Expect(false, "Resource guard renderer initializes"); SDL_Quit(); return test.Finish(); }
    {
        ForestSprites cache;
        cache.Initialize(renderer);
#if SDL_VERSION_ATLEAST(3, 4, 0)
        test.ExpectEqual(cache.LoadedImageCount(), ForestPilotLayout::Images.size(), "Complete published art passes real bounded decode/cache");
        test.Expect(cache.GetResourceUsage().textureBytes > 0 && cache.GetResourceUsage().alphaBytes > 0,
            "Real cache reports logical texture and CPU alpha costs");
        const auto root = std::filesystem::temp_directory_path() / ("embervale-art-render-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        const auto folder = root / "assets/embervale/forest-pilot/textures";
        std::filesystem::create_directories(folder);
        const auto source = std::filesystem::path(reinterpret_cast<const char8_t *>(SDL_GetBasePath())) /
            "assets/embervale/forest-pilot/textures/oak.png";
        const auto target = folder / "oak.png";
        std::filesystem::copy_file(source, target);
        const std::array images{ForestPilotLayout::Image{"oak.png", 64, 96, "assets/embervale/forest-pilot/textures"}};
        const std::array frames{ForestPilotLayout::Frame{"resource.tree.normal.intact", 0,
            {0, 0, 64, 96}, {32, 88}, {16, 24}, -1, -1, -1, 0}};
        WorldArtResourceTestAccess::Initialize(cache, renderer, images, frames, root);
        test.ExpectEqual(cache.LoadedImageCount(), std::size_t{1}, "Private complete texture is cached");
        auto usage = cache.GetResourceUsage();
        test.Expect(usage.textureBytes == 24576 && usage.alphaBytes == 6144,
            "Known 64x96 image accounts for four-byte RGBA estimate and one-byte alpha");
        test.Expect(usage.peakStagingBytes == WorldArtResources::PreflightScratchBytes + std::filesystem::file_size(target),
            "Image staging includes the bounded owned input and decoded RGBA estimate");
        auto badFrames = frames; badFrames[0].source.w = 65;
        WorldArtResourceTestAccess::Initialize(cache, renderer, images, badFrames, root);
        test.Expect(cache.LoadedImageCount() == 0 && !cache.HasTree() && cache.GetResourceUsage().alphaBytes == 0,
            "Invalid frame reinitialization releases textures/alpha and clears stale bindings");
        badFrames = frames; badFrames[0].source.x = std::numeric_limits<float>::quiet_NaN();
        WorldArtResourceTestAccess::Initialize(cache, renderer, images, badFrames, root);
        test.Expect(cache.LoadedImageCount() == 0 && cache.GetResourceUsage().encodedBytes == 0,
            "Non-finite geometry is rejected before file read/decode");
        auto limits = WorldArtResources::DefaultLimits(); limits.textureBytes = 24575;
        WorldArtResourceTestAccess::Initialize(cache, renderer, images, frames, root, limits);
        test.Expect(cache.LoadedImageCount() == 0 && cache.GetResourceUsage().encodedBytes == 0,
            "Over-budget catalogue is rejected before file read/decode");
        { std::ofstream broken(target, std::ios::binary); broken << "not a PNG"; }
        WorldArtResourceTestAccess::Initialize(cache, renderer, images, frames, root);
        test.Expect(cache.LoadedImageCount() == 0 && !cache.HasTree() && cache.GetResourceUsage().textureBytes == 0,
            "Corrupt optional PNG falls back without allocating texture/alpha");
        std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing);
        limits = WorldArtResources::DefaultLimits(); limits.cataloguePngBytes = std::filesystem::file_size(target) - 1;
        WorldArtResourceTestAccess::Initialize(cache, renderer, images, frames, root, limits);
        test.Expect(cache.LoadedImageCount() == 0, "Encoded catalogue budget refuses decode");
        const std::array twoImages{images[0], ForestPilotLayout::Image{"missing.png", 64, 96, images[0].directory}};
        WorldArtResourceTestAccess::Initialize(cache, renderer, twoImages, frames, root);
        test.Expect(cache.LoadedImageCount() == 1 && cache.HasTree(), "Missing optional texture leaves valid cached images usable");
        cache.Clear();
        test.Expect(cache.GetResourceUsage().textureBytes == 0 && cache.GetResourceUsage().alphaBytes == 0 &&
            cache.GetResourceUsage().peakStagingBytes == 0 && !cache.HasTree(), "Clear releases cache and resets accounting/bindings");
        std::error_code ignored; std::filesystem::remove_all(root, ignored);
#endif
    }
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    return test.Finish();
}
