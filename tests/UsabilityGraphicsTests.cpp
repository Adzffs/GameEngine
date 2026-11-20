#include "TestSupport.h"
#include "../src/Graphics/Graphics.h"
#include <variant>
#include <sstream>

struct GraphicsTestAccess
{
    static bool Guide(const Graphics &g) { return g.gameGuideVisible; }
    static void HideGuide(Graphics &g) { g.gameGuideVisible = false; }
    static const std::string &Search(const Graphics &g) { return g.bankSearch; }
    static void Drag(Graphics &g) { g.craftingDragActive = true; g.craftingDragSlotIndices = {0, 1}; g.worldContextMenu.open = true; }
    static bool DragActive(const Graphics &g) { return g.craftingDragActive; }
    static bool Context(const Graphics &g) { return g.worldContextMenu.open; }
    static bool CraftRequest(const Graphics &g) { return g.pendingCraftingInteraction.has_value(); }
    static auto Message(const Graphics &g) { return g.GetSaveFailureMessage(); }
    static auto Window(const Graphics &g) { return g.window; }
    static void PendingCraft(Graphics &g) { g.pendingCraftingInteraction = Graphics::CraftingInteractionRequest{}; }
    static void ClearCraft(Graphics &g) { g.pendingCraftingInteraction.reset(); }
    static void QueueGrid(Graphics &g) { g.requestedCraftingGrid.fill(ItemType::NONE); g.requestedCraftingGrid[4] = ItemType::LOG; g.craftingGridRequestPending = true; }
    static bool HasGrid(const Graphics &g) { return g.craftingGridRequestPending && g.requestedCraftingGrid[4] == ItemType::LOG; }
    static void ClearGrid(Graphics &g) { g.craftingGridRequestPending = false; }
    static bool SidePanel(const Graphics &g) { return g.sidePanelOpen; }
    static void OpenSidePanel(Graphics &g) { g.sidePanelOpen = true; }
    static std::size_t Focus(const Graphics &g) { return g.dialogueFocusIndex; }
};

namespace
{
    void Push(Graphics &graphics, SDL_Event event)
    {
        if (!SDL_PushEvent(&event)) throw std::runtime_error(SDL_GetError());
        bool running = true; graphics.ProcessEvents(running);
    }
    void Key(Graphics &graphics, SDL_Scancode code, bool repeat = false, SDL_Keymod mod = SDL_KMOD_NONE)
    {
        SDL_Event e{}; e.type = SDL_EVENT_KEY_DOWN; e.key.scancode = code; e.key.repeat = repeat; e.key.mod = mod; Push(graphics, e);
    }
    void Talk(Graphics &graphics, bool terminal = false)
    {
        static DialogueSessionId nextSession = 42;
        const auto sessionId = nextSession++;
        NpcTalkEvent event{};
        event.actorEntityID = 4; event.npcEntityID = 1; event.npcType = NpcType::DEVELOPMENT_GUIDE;
        event.sessionId = sessionId; event.dialogueId = DialogueId::DEVELOPMENT_GUIDE_INTRO;
        event.nodeId = DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME; event.text = "Published node";
        event.nodeKind = terminal ? DialogueNodeKind::TERMINAL : DialogueNodeKind::CONTINUE; event.isTerminal = terminal;
        ActiveDialogueSession session{sessionId, 4, 1, NpcType::DEVELOPMENT_GUIDE,
            DialogueId::DEVELOPMENT_GUIDE_INTRO, DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME, 1};
        graphics.SynchronizeDialogue(4, {event}, terminal ? nullptr : &session);
    }
    void Click(Graphics &graphics, const SDL_FRect &rect)
    {
        SDL_Event event{}; event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = rect.x + rect.w * 0.5f; event.button.y = rect.y + rect.h * 0.5f;
        Push(graphics, event);
    }
}

int main()
{
    TestContext test;
    Graphics graphics;
    if (!graphics.Initialize()) { test.Expect(false, "Usability renderer initializes"); return test.Finish(); }
    graphics.ShowGameGuide(); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(!GraphicsTestAccess::Guide(graphics), "Escape dismisses the welcome/help overlay");
    GraphicsTestAccess::HideGuide(graphics);
    graphics.ShowGameGuide(); Key(graphics, SDL_SCANCODE_R);
    test.Expect(!graphics.ConsumeRunToggle(), "Help consumes background movement hotkeys");
    GraphicsTestAccess::HideGuide(graphics);
    graphics.OpenStationMenu(StationType::BANK);
    Key(graphics, SDL_SCANCODE_H);
    SDL_Event text{}; text.type = SDL_EVENT_TEXT_INPUT; text.text.text = "h"; Push(graphics, text);
    test.Expect(!GraphicsTestAccess::Guide(graphics) && GraphicsTestAccess::Search(graphics) == "h",
        "Typing h in the bank search does not open help");
    GraphicsTestAccess::HideGuide(graphics);
    graphics.ShowGameGuide(); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(!GraphicsTestAccess::Guide(graphics) && graphics.IsStationMenuOpen(),
        "Escape closes the top help overlay without closing the bank behind it");
    GraphicsTestAccess::HideGuide(graphics);
    graphics.OpenStationMenu(StationType::CRAFTING_TABLE);
    graphics.SetCraftingInteractionInFlight(true); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(graphics.IsStationMenuOpen(), "Escape cannot abandon an in-flight crafting interaction");
    graphics.SetCraftingInteractionInFlight(false);
    GraphicsTestAccess::PendingCraft(graphics); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(graphics.IsStationMenuOpen() && GraphicsTestAccess::CraftRequest(graphics),
        "Escape preserves a crafting request queued in the same frame");
    Click(graphics, graphics.GetCraftingTableLayout().closeButton);
    test.Expect(graphics.IsStationMenuOpen() && GraphicsTestAccess::CraftRequest(graphics),
        "Mouse Close also preserves the crafting request queued in the same frame");
    GraphicsTestAccess::ClearCraft(graphics);
    if (!graphics.IsStationMenuOpen()) graphics.OpenStationMenu(StationType::CRAFTING_TABLE);
    GraphicsTestAccess::QueueGrid(graphics);
    Click(graphics, graphics.GetCraftingTableLayout().closeButton);
    test.Expect(graphics.IsStationMenuOpen() && GraphicsTestAccess::HasGrid(graphics),
        "Mouse Close cannot discard an unconsumed crafting-grid recipe");
    GraphicsTestAccess::ClearGrid(graphics);
    if (!graphics.IsStationMenuOpen()) graphics.OpenStationMenu(StationType::CRAFTING_TABLE);
    GraphicsTestAccess::Drag(graphics);
    SDL_Event lost{}; lost.type = SDL_EVENT_WINDOW_FOCUS_LOST; Push(graphics, lost);
    test.Expect(!GraphicsTestAccess::DragActive(graphics) && !GraphicsTestAccess::Context(graphics),
        "Focus loss cancels unfinished pointer gestures and context menus");
    SDL_Event released{}; released.type = SDL_EVENT_MOUSE_BUTTON_UP; released.button.button = SDL_BUTTON_LEFT; Push(graphics, released);
    test.Expect(!GraphicsTestAccess::CraftRequest(graphics), "A release after focus loss cannot submit a stale drag");
    Talk(graphics); Key(graphics, SDL_SCANCODE_ESCAPE);
    const auto close = graphics.ConsumeDialogueCommand();
    test.Expect(close && std::holds_alternative<DialogueCloseCommand>(*close),
        "Escape routes an authoritative dialogue close for the displayed session");
    Talk(graphics, true); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(!graphics.GetDialoguePresentationState().IsOpen() && !graphics.ConsumeDialogueCommand(),
        "Escape dismisses terminal text without a redundant server command");
    Talk(graphics); Key(graphics, SDL_SCANCODE_RETURN, true);
    test.Expect(!graphics.ConsumeDialogueCommand(), "Key repeat cannot advance dialogue");
    Key(graphics, SDL_SCANCODE_RETURN);
    auto keyboardContinue = graphics.ConsumeDialogueCommand();
    test.Expect(keyboardContinue && std::holds_alternative<DialogueContinueCommand>(*keyboardContinue),
        "Default Enter activates the published Continue control");
    graphics.SetDialogueCommandInFlight(true);
    Key(graphics, SDL_SCANCODE_RETURN); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(!graphics.ConsumeDialogueCommand(), "Enter and Escape preserve the in-flight dialogue command");
    graphics.SetDialogueCommandInFlight(false);
    const auto defaultFocus = GraphicsTestAccess::Focus(graphics);
    Key(graphics, SDL_SCANCODE_TAB); Key(graphics, SDL_SCANCODE_TAB, false, SDL_KMOD_SHIFT);
    test.Expect(GraphicsTestAccess::Focus(graphics) == defaultFocus, "Tab and Shift-Tab traverse in opposite directions");
    Key(graphics, SDL_SCANCODE_UP); Key(graphics, SDL_SCANCODE_SPACE);
    const auto keyboardClose = graphics.ConsumeDialogueCommand();
    test.Expect(keyboardClose && std::holds_alternative<DialogueCloseCommand>(*keyboardClose),
        "Arrow navigation and Space activate the selected Close control");
    Talk(graphics, true); Key(graphics, SDL_SCANCODE_RETURN);
    GraphicsTestAccess::OpenSidePanel(graphics); Key(graphics, SDL_SCANCODE_ESCAPE);
    test.Expect(!GraphicsTestAccess::SidePanel(graphics), "Escape closes the side panel when no modal is open");
    const auto message = GraphicsTestAccess::Message(graphics);
    test.Expect(message.window == GraphicsTestAccess::Window(graphics) && message.window != nullptr,
        "Native save-failure choices belong to the live game window");
    test.Expect(message.numbuttons == 2 && message.buttons[0].buttonID == 0 &&
        (message.buttons[0].flags & SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT) != 0 &&
        (message.buttons[0].flags & SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT) != 0 &&
        message.buttons[1].buttonID == 1 && message.buttons[1].flags == 0,
        "Return and Escape safely resume; unsaved exit requires explicit selection");
    std::istringstream messageLines(message.message); std::string line; bool boundedLines = true;
    while (std::getline(messageLines, line)) boundedLines = boundedLines && line.size() <= 70;
    test.Expect(boundedLines && std::string(message.message).find("discards progress") != std::string::npos,
        "Save-error text has bounded native lines and explains progress loss");
    SDL_Event quit{}; quit.type = SDL_EVENT_KEY_DOWN; quit.key.scancode = SDL_SCANCODE_Q; quit.key.mod = SDL_KMOD_CTRL;
    SDL_PushEvent(&quit); bool running = true; graphics.ProcessEvents(running);
    test.Expect(!running, "Ctrl-Q enters the normal guarded shutdown path");
    return test.Finish();
}
