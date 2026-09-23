#include "TestSupport.h"

#include "../src/Graphics/Graphics.h"
#include "../src/Player/Player.h"
#include "../src/Quest/QuestSystem.h"

#include <iostream>

struct GraphicsTestAccess
{
    static SDL_Renderer* Renderer(Graphics& graphics) { return graphics.renderer; }
    static void DrawQuest(Graphics& graphics, const Player& player)
    {
        graphics.DrawQuestPanel(player);
    }
    static void DrawSkills(Graphics& graphics, const Player& player)
    {
        graphics.DrawSkills(player);
    }
    static void DrawInventory(Graphics& graphics, const Inventory& inventory)
    {
        graphics.DrawInventory(inventory);
    }
    static void DrawQuestRows(Graphics& graphics,
        const std::vector<QuestPresentation>& quests)
    {
        graphics.DrawQuestRows(quests);
    }
};

namespace
{
bool IsInside(const SDL_FRect &bounds, const SDL_FPoint &point)
{
    return point.x >= bounds.x && point.x < bounds.x + bounds.w &&
        point.y >= bounds.y && point.y < bounds.y + bounds.h;
}

bool IsInside(const SDL_FRect &outer, const SDL_FRect &inner)
{
    return inner.x >= outer.x && inner.y >= outer.y &&
        inner.x + inner.w <= outer.x + outer.w &&
        inner.y + inner.h <= outer.y + outer.h;
}

bool Overlaps(const SDL_FRect &left, const SDL_FRect &right)
{
    return left.x < right.x + right.w && left.x + left.w > right.x &&
        left.y < right.y + right.h && left.y + left.h > right.y;
}

struct Pixel
{
    Uint8 red = 0;
    Uint8 green = 0;
    Uint8 blue = 0;
    Uint8 alpha = 0;
};

Pixel ReadPixel(SDL_Surface* surface, int x, int y)
{
    Pixel pixel;
    SDL_ReadSurfacePixel(
        surface, x, y,
        &pixel.red, &pixel.green, &pixel.blue, &pixel.alpha);
    return pixel;
}

bool HasYellowText(SDL_Surface* surface, const SDL_FPoint& origin)
{
    for (int y = static_cast<int>(origin.y); y < static_cast<int>(origin.y) + 9; ++y)
        for (int x = static_cast<int>(origin.x); x < static_cast<int>(origin.x) + 145; ++x)
        {
            const Pixel pixel = ReadPixel(surface, x, y);
            if (pixel.red == 255 && pixel.green == 215 && pixel.blue == 100)
                return true;
        }
    return false;
}

void SelectTab(Graphics &graphics, std::size_t index)
{
    const SDL_FRect panel = graphics.GetSidePanelRectangle();
    constexpr float tabWidth = 25.0f;
    constexpr float tabHeight = 30.0f;
    constexpr float tabGap = 2.0f;
    graphics.HandleSidePanelClick(
        panel.x + static_cast<float>(index) * (tabWidth + tabGap) + tabWidth / 2.0f,
        panel.y - tabHeight - 4.0f + tabHeight / 2.0f);
}
}

int main()
{
    TestContext test;
    Graphics graphics;
    Player player(100, PlayerInitializationMode::EMPTY);

    const SDL_FRect panel = graphics.GetSidePanelRectangle();
    test.ExpectNear(panel.x, 1068.0f, 0.001f, "Quest panel uses established side-panel X");
    test.ExpectNear(panel.y, 352.0f, 0.001f, "Quest panel uses established side-panel Y");
    test.ExpectNear(panel.w, 192.0f, 0.001f, "Quest panel uses established side-panel width");
    test.ExpectNear(panel.h, 348.0f, 0.001f, "Quest panel uses established side-panel height");

    SelectTab(graphics, 1);
    auto view = graphics.BuildQuestPanelView(player);
    test.Expect(view.visible, "Quest content is selected only through the QU tab");
    test.Expect(view.heading == "QUESTS" && view.title == "Gathering Basics",
                "Quest panel has the expected heading and title");
    test.Expect(IsInside(view.bounds, view.headingPosition) &&
                    IsInside(view.bounds, view.titlePosition) &&
                    IsInside(view.bounds, view.statePosition) &&
                    IsInside(view.bounds, view.progressPosition),
                "Every quest text origin lies inside the side panel");
    test.Expect(IsInside(view.contentBounds, view.headingPosition) &&
                    IsInside(view.contentBounds, view.titlePosition) &&
                    IsInside(view.contentBounds, view.statePosition) &&
                    IsInside(view.contentBounds, view.progressPosition),
                "Every quest text origin lies inside the shared content bounds");
    test.Expect(view.state == "AVAILABLE" && view.progress == "Logs: 0 / 10",
                "Available quest has a clear label");

    QuestSystem::TryRestore(player.GetQuestJournal(), {{QuestId::GATHERING_BASICS, QuestState::ACTIVE, 0}});
    view = graphics.BuildQuestPanelView(player);
    test.Expect(view.state == "ACTIVE" && view.progress == "Logs: 0 / 10",
                "Active quest displays authoritative zero progress");
    QuestSystem::TryRestore(player.GetQuestJournal(), {{QuestId::GATHERING_BASICS, QuestState::ACTIVE, 6}});
    view = graphics.BuildQuestPanelView(player);
    test.Expect(view.progress == "Logs: 6 / 10",
                "Active quest displays authoritative updated progress");

    QuestSystem::TryRestore(player.GetQuestJournal(), {{QuestId::GATHERING_BASICS, QuestState::READY_TO_COMPLETE, 10}});
    view = graphics.BuildQuestPanelView(player);
    test.Expect(view.state == "READY TO COMPLETE" &&
                    view.progress == "Logs: 10 / 10" &&
                    player.GetQuestJournal().TryGet(QuestId::GATHERING_BASICS)->progress == 10,
                "Ready quest has a clear completion label");

    QuestSystem::TryRestore(player.GetQuestJournal(), {{QuestId::GATHERING_BASICS, QuestState::COMPLETED, 10}});
    view = graphics.BuildQuestPanelView(player);
    test.Expect(view.state == "COMPLETED" && view.progress == "Logs: 10 / 10",
                "Completed quest has a clear label");

    SelectTab(graphics, 0);
    test.Expect(!graphics.BuildQuestPanelView(player).visible,
                "Switching to Skills hides quest content");
    SelectTab(graphics, 2);
    test.Expect(!graphics.BuildQuestPanelView(player).visible,
                "Switching to Inventory hides quest content");
    SelectTab(graphics, 1);
    test.Expect(graphics.BuildQuestPanelView(player).visible,
                "Switching back to Quests restores quest content without tab leakage");

    const std::vector<QuestPresentation> syntheticQuests{
        {static_cast<QuestId>(10), QuestState::AVAILABLE, 0, 10,
            "Available Quest", "Logs"},
        {static_cast<QuestId>(11), QuestState::ACTIVE, 6, 10,
            "Active Quest", "Logs"},
        {static_cast<QuestId>(12), QuestState::READY_TO_COMPLETE, 10, 10,
            "Ready Quest", "Logs"},
        {static_cast<QuestId>(13), QuestState::COMPLETED, 10, 10,
            "Completed Quest", "Logs"},
        {static_cast<QuestId>(14), QuestState::ACTIVE, 2, 10,
            "Overflow Quest", "Logs"}};
    const auto syntheticRows = graphics.BuildQuestRowLayout(syntheticQuests);
    bool retainedOrder = syntheticRows.size() == syntheticQuests.size();
    bool insideContent = true;
    bool disjoint = true;
    bool consistentSpacing = true;
    for (std::size_t index = 0; index < syntheticRows.size(); ++index)
    {
        retainedOrder = retainedOrder &&
            syntheticRows[index].quest.id == syntheticQuests[index].id;
        insideContent = insideContent &&
            IsInside(view.contentBounds, syntheticRows[index].bounds);
        if (index > 0)
        {
            disjoint = disjoint && !Overlaps(
                syntheticRows[index - 1].bounds, syntheticRows[index].bounds);
            consistentSpacing = consistentSpacing &&
                syntheticRows[index].originY -
                    syntheticRows[index - 1].originY == 88.0f;
        }
    }
    test.Expect(retainedOrder,
                "Synthetic quest rows retain deterministic catalogue order");
    test.Expect(disjoint && consistentSpacing,
                "Synthetic quest rows are distinct with consistent spacing");
    test.Expect(insideContent && !syntheticRows[0].clipped &&
                    syntheticRows[3].clipped && syntheticRows[4].clipped,
                "Rows remain bounded and overflow rows are marked clipped");
    test.Expect(Graphics::GetQuestStateLabel(QuestState::AVAILABLE) == "AVAILABLE" &&
                    Graphics::GetQuestStateLabel(QuestState::ACTIVE) == "ACTIVE" &&
                    Graphics::GetQuestStateLabel(QuestState::READY_TO_COMPLETE) ==
                        "READY TO COMPLETE" &&
                    Graphics::GetQuestStateLabel(QuestState::COMPLETED) == "COMPLETED" &&
                    Graphics::GetQuestProgressLabel(syntheticQuests[1]) ==
                        "Logs: 6 / 10",
                "Runtime row labels cover every visible quest state");

    Graphics nativeGraphics;
    if (nativeGraphics.Initialize())
    {
        SDL_Renderer* renderer = GraphicsTestAccess::Renderer(nativeGraphics);
        int outputWidth = 0;
        int outputHeight = 0;
        int logicalWidth = 0;
        int logicalHeight = 0;
        SDL_RendererLogicalPresentation presentation =
            SDL_LOGICAL_PRESENTATION_DISABLED;
        test.Expect(SDL_GetRenderOutputSize(renderer, &outputWidth, &outputHeight),
                    "Native renderer reports its runtime output size");
        test.Expect(SDL_GetRenderLogicalPresentation(
                        renderer, &logicalWidth, &logicalHeight, &presentation) &&
                        logicalWidth == 1280 && logicalHeight == 720 &&
                        presentation == SDL_LOGICAL_PRESENTATION_LETTERBOX,
                    "Native renderer uses the fixed logical client layout");

        const auto layout = nativeGraphics.GetSidePanelLayout();
        std::cout << "[RUNTIME] output=" << outputWidth << 'x' << outputHeight
                  << " panel={" << layout.panelBounds.x << ',' << layout.panelBounds.y
                  << ',' << layout.panelBounds.w << ',' << layout.panelBounds.h << "}"
                  << " content={" << layout.contentBounds.x << ',' << layout.contentBounds.y
                  << ',' << layout.contentBounds.w << ',' << layout.contentBounds.h << "}"
                  << std::endl;

        SDL_SetRenderDrawColor(renderer, 5, 7, 9, 255);
        SDL_RenderClear(renderer);
        GraphicsTestAccess::DrawQuestRows(nativeGraphics, syntheticQuests);
        SDL_Surface* syntheticPixels = SDL_RenderReadPixels(renderer, nullptr);
        test.Expect(syntheticPixels != nullptr &&
                        ReadPixel(syntheticPixels,
                            static_cast<int>(layout.contentBounds.x) - 2,
                            static_cast<int>(layout.contentBounds.y) + 40).red == 5 &&
                        HasYellowText(syntheticPixels,
                            syntheticRows.front().titlePosition),
                    "Native multi-row drawing is clipped to shared Quest content bounds");
        if (syntheticPixels != nullptr) SDL_DestroySurface(syntheticPixels);

        Player nativePlayer(101, PlayerInitializationMode::EMPTY);
        SelectTab(nativeGraphics, 1);
        const std::array<std::pair<QuestState, int>, 5> states{{
            {QuestState::AVAILABLE, 0},
            {QuestState::ACTIVE, 0},
            {QuestState::ACTIVE, 6},
            {QuestState::READY_TO_COMPLETE, 10},
            {QuestState::COMPLETED, 10}}};
        for (const auto& [state, progress] : states)
        {
            QuestSystem::TryRestore(nativePlayer.GetQuestJournal(), {{QuestId::GATHERING_BASICS, state, progress}});
            const auto nativeView = nativeGraphics.BuildQuestPanelView(nativePlayer);
            SDL_SetRenderDrawColor(renderer, 5, 7, 9, 255);
            SDL_RenderClear(renderer);
            GraphicsTestAccess::DrawQuest(nativeGraphics, nativePlayer);
            SDL_Surface* pixels = SDL_RenderReadPixels(renderer, nullptr);
            test.Expect(pixels != nullptr,
                        "Native Quest render can be read back for every state");
            if (pixels != nullptr)
            {
                const Pixel outside = ReadPixel(
                    pixels,
                    static_cast<int>(layout.panelBounds.x) - 2,
                    static_cast<int>(layout.panelBounds.y) + 2);
                const Pixel body = ReadPixel(
                    pixels,
                    static_cast<int>(layout.panelBounds.x) + 2,
                    static_cast<int>(layout.panelBounds.y) + 2);
                test.Expect(outside.red == 5 && outside.green == 7 && outside.blue == 9,
                            "Quest render does not paint over the world outside the panel");
                test.Expect(body.red == 30 && body.green == 30 && body.blue == 30,
                            "Quest render draws the standard dark panel body");
                test.Expect(HasYellowText(pixels, nativeView.titlePosition),
                            "Quest title is rendered inside the panel for every state");
                SDL_DestroySurface(pixels);
            }
        }

        SDL_SetRenderDrawColor(renderer, 5, 7, 9, 255);
        SDL_RenderClear(renderer);
        GraphicsTestAccess::DrawSkills(nativeGraphics, nativePlayer);
        SDL_Surface* skillPixels = SDL_RenderReadPixels(renderer, nullptr);
        SDL_SetRenderDrawColor(renderer, 5, 7, 9, 255);
        SDL_RenderClear(renderer);
        GraphicsTestAccess::DrawInventory(nativeGraphics, nativePlayer.GetInventory());
        SDL_Surface* inventoryPixels = SDL_RenderReadPixels(renderer, nullptr);
        test.Expect(skillPixels != nullptr && inventoryPixels != nullptr &&
                        ReadPixel(skillPixels,
                            static_cast<int>(layout.panelBounds.x) + 2,
                            static_cast<int>(layout.panelBounds.y) + 2).red == 30 &&
                        ReadPixel(inventoryPixels,
                            static_cast<int>(layout.panelBounds.x) + 2,
                            static_cast<int>(layout.panelBounds.y) + 2).red == 30,
                    "Skills, Inventory and Quests render the same runtime panel body");
        if (skillPixels != nullptr) SDL_DestroySurface(skillPixels);
        if (inventoryPixels != nullptr) SDL_DestroySurface(inventoryPixels);
    }
    else
    {
        test.Expect(false, "Native SDL renderer initializes for Quest rendering regression");
    }

    return test.Finish();
}
