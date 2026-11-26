#include "TestSupport.h"

#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/World/Map.h"
#include <algorithm>

struct GraphicsTestAccess
{
    static void UpdateCamera(
        Graphics &graphics,
        float playerX,
        float playerY,
        const Map &map)
    {
        graphics.UpdateCamera({playerX, playerY}, map);
    }

    static SDL_FPoint ScreenToWorldPoint(
        const Graphics &graphics,
        float screenX,
        float screenY)
    {
        return graphics.ScreenToWorldPoint(screenX, screenY);
    }

    static float CameraX(const Graphics &graphics)
    {
        return graphics.cameraPixelX;
    }

    static float CameraY(const Graphics &graphics)
    {
        return graphics.cameraPixelY;
    }

    static void SetPresentationDelta(Graphics &graphics, float seconds)
    {
        graphics.presentationDeltaSeconds = seconds;
    }

    static void DrawWalkingFrame(Graphics &graphics, Map &map, Player &player, int y)
    {
        graphics.lastPresentationTick = 0;
        graphics.presentationDeltaSeconds = 0.04f;
        player.GetPosition().SetPosition(40, y);
        graphics.Render(map, {}, {}, {}, {}, {}, 40, y, player.GetInventory(),
            player.GetEquipment(), player, nullptr);
    }

    static float FootScreenY(const Graphics &graphics)
    {
        return graphics.visualPlayerY * Graphics::TileSize + 24.0f - graphics.cameraPixelY;
    }

    static void SetMinimapOrigin(Graphics &graphics, int x, int y)
    {
        graphics.minimapOriginX = x;
        graphics.minimapOriginY = y;
    }

    static bool HandleMinimapClick(Graphics &graphics, float x, float y)
    {
        return graphics.HandleMinimapClick(x, y);
    }
};

int main()
{
    TestContext test;
    Map map(100, 100);
    Graphics graphics;

    GraphicsTestAccess::UpdateCamera(graphics, 0, 0, map);
    test.ExpectEqual(GraphicsTestAccess::CameraX(graphics), 0.0f,
        "Camera clamps at the left map boundary");
    test.ExpectEqual(GraphicsTestAccess::CameraY(graphics), 0.0f,
        "Camera clamps at the top map boundary");

    GraphicsTestAccess::SetPresentationDelta(graphics, 0.05f);
    GraphicsTestAccess::UpdateCamera(graphics, 30, 20, map);
    test.Expect(GraphicsTestAccess::CameraX(graphics) > 0.0f,
        "Camera follows the player beyond the starter viewport");
    test.Expect(GraphicsTestAccess::CameraY(graphics) > 0.0f,
        "Camera follows the player vertically beyond the starter viewport");

    const SDL_FPoint world = GraphicsTestAccess::ScreenToWorldPoint(
        graphics, 125.0f, 80.0f);
    test.ExpectNear(world.x,
        125.0f + GraphicsTestAccess::CameraX(graphics), 0.001f,
        "Screen-to-world conversion includes horizontal camera movement");
    test.ExpectNear(world.y,
        80.0f + GraphicsTestAccess::CameraY(graphics), 0.001f,
        "Screen-to-world conversion includes vertical camera movement");

    for (int index = 0; index < 10; ++index)
        graphics.AdjustCameraZoom(1);
    test.ExpectNear(graphics.GetCameraZoom(), 1.5f, 0.001f,
        "Zoom-in input clamps at a readable maximum");
    const SDL_FPoint zoomedWorld = GraphicsTestAccess::ScreenToWorldPoint(
        graphics, 150.0f, 90.0f);
    test.ExpectNear(zoomedWorld.x,
        100.0f + GraphicsTestAccess::CameraX(graphics), 0.001f,
        "Zoomed screen coordinates preserve exact world targeting");
    test.ExpectNear(zoomedWorld.y,
        60.0f + GraphicsTestAccess::CameraY(graphics), 0.001f,
        "Zoomed vertical coordinates preserve exact world targeting");
    for (int index = 0; index < 20; ++index)
        graphics.AdjustCameraZoom(-1);
    test.ExpectNear(graphics.GetCameraZoom(), 0.75f, 0.001f,
        "Zoom-out input clamps before world details become unreadable");

    Graphics anchoredZoomGraphics;
    const SDL_FPoint beforeAnchoredZoom = GraphicsTestAccess::ScreenToWorldPoint(
        anchoredZoomGraphics, 420.0f, 260.0f);
    anchoredZoomGraphics.AdjustCameraZoom(1, 420.0f, 260.0f);
    const SDL_FPoint afterAnchoredZoom = GraphicsTestAccess::ScreenToWorldPoint(
        anchoredZoomGraphics, 420.0f, 260.0f);
    test.ExpectNear(afterAnchoredZoom.x, beforeAnchoredZoom.x, 0.001f,
        "Zoom keeps the horizontal world point beneath the cursor fixed");
    test.ExpectNear(afterAnchoredZoom.y, beforeAnchoredZoom.y, 0.001f,
        "Zoom keeps the vertical world point beneath the cursor fixed");

    Graphics rapidZoomGraphics;
    GraphicsTestAccess::UpdateCamera(rapidZoomGraphics, 40, 40, map);
    const SDL_FPoint rapidZoomAnchorBefore = GraphicsTestAccess::ScreenToWorldPoint(
        rapidZoomGraphics, 520.0f, 360.0f);
    for (int index = 0; index < 5; ++index)
    {
        rapidZoomGraphics.AdjustCameraZoom(1, 520.0f, 360.0f);
        rapidZoomGraphics.AdjustCameraZoom(-1, 520.0f, 360.0f);
    }
    rapidZoomGraphics.AdjustCameraZoom(1, 520.0f, 360.0f);
    GraphicsTestAccess::UpdateCamera(rapidZoomGraphics, 40, 40, map);
    const SDL_FPoint rapidZoomAnchorAfter = GraphicsTestAccess::ScreenToWorldPoint(
        rapidZoomGraphics, 520.0f, 360.0f);
    test.ExpectNear(rapidZoomAnchorAfter.x, rapidZoomAnchorBefore.x, 0.001f,
        "Rapid zoom input and the following render share one horizontal transform");
    test.ExpectNear(rapidZoomAnchorAfter.y, rapidZoomAnchorBefore.y, 0.001f,
        "Rapid zoom input and the following render share one vertical transform");

    GraphicsTestAccess::SetMinimapOrigin(graphics, 20, 30);
    test.Expect(GraphicsTestAccess::HandleMinimapClick(
        graphics, 1040.0f + 5.0f * 7.0f + 1.0f,
        42.0f + 4.0f * 7.0f + 1.0f),
        "Click inside the minimap is consumed as navigation");
    int walkX = -1;
    int walkY = -1;
    test.Expect(graphics.ConsumeWalkTile(walkX, walkY),
        "Minimap click creates a walk request");
    test.ExpectEqual(walkX, 25,
        "Minimap click converts its local column to a world tile");
    test.ExpectEqual(walkY, 34,
        "Minimap click converts its local row to a world tile");

    Graphics walkingCamera;
    if (!walkingCamera.Initialize())
    {
        test.Expect(false, "Walking camera renderer initializes");
        return test.Finish();
    }
    Player walker(1, PlayerInitializationMode::FRESH_START);
    GraphicsTestAccess::DrawWalkingFrame(walkingCamera, map, walker, 40);
    float previousY = GraphicsTestAccess::FootScreenY(walkingCamera);
    float largestBackwardStep = 0.0f;
    for (int frame = 0; frame < 30; ++frame)
    {
        GraphicsTestAccess::DrawWalkingFrame(walkingCamera, map, walker, 39 - frame / 15);
        const float y = GraphicsTestAccess::FootScreenY(walkingCamera);
        largestBackwardStep = std::max(largestBackwardStep, y - previousY);
        previousY = y;
    }
    test.Expect(largestBackwardStep < 0.1f,
        "A northward walk keeps moving forward on screen across server tile updates");

    return test.Finish();
}
