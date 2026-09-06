#include "TestSupport.h"

#include "../src/Core/Engine.h"
#include "../src/Core/EngineClickRouting.h"
#include "../src/NPC/NPC.h"

#include <variant>

struct GraphicsTestAccess
{
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
