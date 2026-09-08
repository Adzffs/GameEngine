#include "TestSupport.h"

#include "../src/Core/Engine.h"
#include "../src/Core/EngineClickRouting.h"
#include "../src/NPC/NPC.h"

#include <variant>

struct GraphicsTestAccess
{
    static bool PushLeftClick(Graphics &graphics, float x, float y)
    {
        if ((SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS) == 0 &&
            !SDL_InitSubSystem(SDL_INIT_EVENTS))
            return false;
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
        if (!SDL_PushEvent(&event))
            return false;
        bool running = true;
        graphics.ProcessEvents(running);
        return running;
    }
    static bool Click(Graphics &graphics, float x, float y)
    {
        return graphics.HandleDialogueClick(x, y);
    }
    static bool HasWorldClick(const Graphics &graphics)
    {
        return graphics.clickPending;
    }
    static bool HasDialogueCommand(const Graphics &graphics)
    {
        return graphics.pendingDialogueCommand.has_value();
    }
    static bool IsStationMenuOpen(const Graphics &graphics)
    {
        return graphics.stationMenuOpen;
    }
};

struct EngineTestAccess
{
    static bool Initialize(Engine &engine)
    {
        return engine.InitializeWorldForRun();
    }
    static World &WorldOf(Engine &engine) { return engine.world; }
    static Graphics &GraphicsOf(Engine &engine) { return engine.graphics; }
    static int PlayerID(const Engine &engine) { return engine.playerID; }
    static void Synchronize(Engine &engine)
    {
        engine.SynchronizeDialoguePresentation();
    }
    static bool EnqueueDialogue(Engine &engine)
    {
        return engine.EnqueuePendingDialogueCommand();
    }
};

namespace
{
    void ClickCenter(Graphics &graphics, const SDL_FRect &rectangle)
    {
        GraphicsTestAccess::Click(
            graphics,
            rectangle.x + rectangle.w * 0.5f,
            rectangle.y + rectangle.h * 0.5f);
    }

    void AdvanceUntilDialogue(World &world, int playerID)
    {
        for (int tick = 0;
             tick < 12 && world.GetActiveDialogueSession(playerID) == nullptr;
             ++tick)
        {
            world.Update();
        }
    }
}

int main()
{
    TestContext test;

    {
        Graphics graphics;
        NpcTalkEvent event{
            44, 17, NpcType::DEVELOPMENT_GUIDE, 901,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
            "Let me show you the gathering basics.",
            DialogueNodeKind::CONTINUE, false, {}, false,
            QuestDialogueAction::ACCEPT_GATHERING_BASICS};
        ActiveDialogueSession session{
            901, 44, 17, NpcType::DEVELOPMENT_GUIDE,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 1};
        graphics.SynchronizeDialogue(44, {event}, &session);
        const SDL_FRect acceptRectangle =
            graphics.GetDialogueQuestButtonRectangle();
        ClickCenter(graphics, acceptRectangle);
        auto accept = graphics.ConsumeDialogueCommand();
        const auto* acceptCommand = accept
            ? std::get_if<QuestAcceptCommand>(&*accept) : nullptr;
        test.Expect(acceptCommand != nullptr &&
                        acceptCommand->actorEntityID == 44 &&
                        acceptCommand->npcEntityID == 17 &&
                        acceptCommand->dialogueSessionId == 901 &&
                        acceptCommand->questId == QuestId::GATHERING_BASICS,
                    "Rendered Accept hitbox forwards exact authoritative IDs");
        test.Expect(!GraphicsTestAccess::HasWorldClick(graphics) &&
                        !graphics.ConsumeDialogueCommand().has_value(),
                    "Accept click is modal and queues exactly one command");

        event.questAction = QuestDialogueAction::COMPLETE_GATHERING_BASICS;
        graphics.SynchronizeDialogue(44, {event}, &session);
        const SDL_FRect completeRectangle =
            graphics.GetDialogueQuestButtonRectangle();
        test.Expect(acceptRectangle.x == completeRectangle.x &&
                        acceptRectangle.y == completeRectangle.y &&
                        acceptRectangle.w == completeRectangle.w &&
                        acceptRectangle.h == completeRectangle.h,
                    "Accept and Complete rendering share their hit-test rectangle");
        ClickCenter(graphics, completeRectangle);
        auto complete = graphics.ConsumeDialogueCommand();
        const auto* completeCommand = complete
            ? std::get_if<QuestCompleteCommand>(&*complete) : nullptr;
        test.Expect(completeCommand != nullptr &&
                        completeCommand->actorEntityID == 44 &&
                        completeCommand->npcEntityID == 17 &&
                        completeCommand->dialogueSessionId == 901 &&
                        completeCommand->questId == QuestId::GATHERING_BASICS,
                    "Rendered Complete hitbox forwards exact authoritative IDs");
    }

    {
        Graphics graphics;
        const NpcTalkEvent event{
            4, 1, NpcType::DEVELOPMENT_GUIDE, 90,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
            "Welcome", DialogueNodeKind::CONTINUE, false, {}};
        ActiveDialogueSession session{
            90, 4, 1, NpcType::DEVELOPMENT_GUIDE,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 1};
        graphics.SynchronizeDialogue(4, {event}, &session);
        graphics.OpenStationMenu(StationType::FURNACE);
        graphics.SynchronizeDialogue(4, {event}, &session);
        test.Expect(!GraphicsTestAccess::IsStationMenuOpen(graphics),
                    "Dialogue synchronization prevents overlapping station input");

        ClickCenter(graphics, graphics.GetDialogueContinueButtonRectangle());
        test.Expect(GraphicsTestAccess::HasDialogueCommand(graphics) &&
                        !GraphicsTestAccess::HasWorldClick(graphics),
                    "Dialogue button click creates only dialogue input");

        std::optional<ServerCommandData> command = graphics.ConsumeDialogueCommand();
        test.Expect(command.has_value(),
                    "One UI click produces one command");
        test.Expect(!graphics.ConsumeDialogueCommand().has_value(),
                    "Repeated consumption does not duplicate the command");
        const auto *continued = command.has_value()
            ? std::get_if<DialogueContinueCommand>(&*command)
            : nullptr;
        test.Expect(continued != nullptr && continued->sessionId == 90,
                    "Client continue uses the displayed session ID");

        ClickCenter(graphics, graphics.GetDialogueContinueButtonRectangle());
        const NpcTalkEvent replacement{
            4, 1, NpcType::DEVELOPMENT_GUIDE, 91,
            DialogueId::DEVELOPMENT_GUIDE_INTRO,
            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
            "Replacement", DialogueNodeKind::CONTINUE, false, {}};
        ActiveDialogueSession replacementSession = session;
        replacementSession.sessionId = 91;
        graphics.SynchronizeDialogue(4, {replacement}, &replacementSession);
        test.Expect(!graphics.ConsumeDialogueCommand().has_value(),
                    "A newer session invalidates an unconsumed older control");
    }

    {
        Engine engine;
        test.Expect(EngineTestAccess::Initialize(engine),
                    "Engine dialogue fixture initializes");
        World &world = EngineTestAccess::WorldOf(engine);
        Graphics &graphics = EngineTestAccess::GraphicsOf(engine);
        const int playerID = EngineTestAccess::PlayerID(engine);
        auto *guide = dynamic_cast<NPC *>(world.GetEntityByID(1));
        test.Expect(guide != nullptr, "Development Guide exists");

        const SDL_FRect guideRectangle =
            graphics.GetFriendlyNpcScreenRectangle(*guide);
        EngineClickRouting::HandleWorldClick(
            graphics, world, playerID,
            guide->GetPosition().GetX(), guide->GetPosition().GetY(),
            static_cast<int>(guideRectangle.x + 1.0f),
            static_cast<int>(guideRectangle.y + 1.0f));
        test.Expect(world.GetActiveDialogueSession(playerID) == nullptr,
                    "NPC click does not open dialogue before a World tick");

        AdvanceUntilDialogue(world, playerID);
        EngineTestAccess::Synchronize(engine);
        test.Expect(graphics.GetDialoguePresentationState().IsOpen() &&
                        graphics.GetDialoguePresentationState().GetEvent()->nodeId ==
                            DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME,
                    "Engine presents the authoritative welcome event after approach");

        const int beforeX = world.GetEntityByID(playerID)->GetPosition().GetX();
        const int beforeY = world.GetEntityByID(playerID)->GetPosition().GetY();
        test.Expect(GraphicsTestAccess::PushLeftClick(
                        graphics, guideRectangle.x + 1.0f,
                        guideRectangle.y + 1.0f),
                    "SDL world-target click is submitted through ProcessEvents");
        int blockedTileX = 0;
        int blockedTileY = 0;
        test.Expect(!graphics.ConsumeClickedTile(blockedTileX, blockedTileY) &&
                        !graphics.ConsumeDialogueCommand().has_value() &&
                        !graphics.ConsumeShopCommand().has_value(),
                    "Dialogue modal SDL routing emits no world or unrelated command");
        world.Update();
        EngineTestAccess::Synchronize(engine);
        const Entity* unchangedPlayer = world.GetEntityByID(playerID);
        test.Expect(unchangedPlayer->GetPosition().GetX() == beforeX &&
                        unchangedPlayer->GetPosition().GetY() == beforeY &&
                        !world.HasActiveMovementPath(playerID) &&
                        world.GetActionForEntity(playerID) == nullptr &&
                        !world.HasPendingMeleeEngagement(playerID) &&
                        !world.HasActiveStation(playerID) &&
                        world.GetActiveShopSession(playerID) == nullptr,
                    "Blocked SDL click creates no movement, combat, gathering, station or shop interaction");
        test.Expect(world.GetActiveDialogueSession(playerID) != nullptr &&
                        graphics.GetDialoguePresentationState().IsOpen(),
                    "Dialogue remains authoritative and visible after blocked SDL click");

        ClickCenter(graphics, graphics.GetDialogueContinueButtonRectangle());
        test.Expect(EngineTestAccess::EnqueueDialogue(engine),
                    "Engine enqueues continue through the normal command queue");
        world.Update();
        EngineTestAccess::Synchronize(engine);
        test.Expect(graphics.GetDialoguePresentationState().GetEvent()->nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_EXPLANATION,
                    "First continue publishes the explanation");

        ClickCenter(graphics, graphics.GetDialogueContinueButtonRectangle());
        EngineTestAccess::EnqueueDialogue(engine);
        world.Update();
        EngineTestAccess::Synchronize(engine);
        test.Expect(graphics.GetDialoguePresentationState().GetEvent()->nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_TOPIC_PROMPT,
                    "Second continue publishes the choice node");

        ClickCenter(graphics, graphics.GetDialogueChoiceButtonRectangle(0));
        EngineTestAccess::EnqueueDialogue(engine);
        world.Update();
        EngineTestAccess::Synchronize(engine);
        test.Expect(graphics.GetDialoguePresentationState().GetEvent()->nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_GATHERING_RESPONSE,
                    "Exact first published choice selects the gathering branch");

        ClickCenter(graphics, graphics.GetDialogueContinueButtonRectangle());
        EngineTestAccess::EnqueueDialogue(engine);
        world.Update();
        EngineTestAccess::Synchronize(engine);
        test.Expect(graphics.GetDialoguePresentationState().GetEvent()->nodeId ==
                        DialogueNodeId::DEVELOPMENT_GUIDE_FUTURE &&
                        graphics.GetDialoguePresentationState().GetEvent()->isTerminal,
                    "Conversation reaches the published terminal node");

        ClickCenter(graphics, graphics.GetDialogueContinueButtonRectangle());
        test.Expect(!EngineTestAccess::EnqueueDialogue(engine) &&
                        !graphics.GetDialoguePresentationState().IsOpen(),
                    "Terminal close dismisses locally without a stale command");
    }

    return test.Finish();
}
