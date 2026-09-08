#pragma once

#include "DialogueSystem.h"
#include "../Command/ServerCommand.h"
#include "../NPC/NpcTalkEvent.h"

#include <optional>
#include <string>
#include <vector>

class DialoguePresentationState
{
public:
    void Synchronize(
        int localActorEntityID,
        const std::vector<NpcTalkEvent> &publishedEvents,
        const ActiveDialogueSession *activeSession);

    bool IsOpen() const { return event.has_value(); }
    const NpcTalkEvent *GetEvent() const
    {
        return event.has_value() ? &*event : nullptr;
    }
    const std::string &GetNpcName() const { return npcName; }

    std::optional<ServerCommandData> MakeContinueCommand() const;
    std::optional<ServerCommandData> MakeChoiceCommand(
        std::size_t choiceIndex) const;
    std::optional<ServerCommandData> MakeCloseCommand() const;
    std::optional<ServerCommandData> MakeTradeCommand() const;
    std::optional<ServerCommandData> MakeQuestCommand() const;
    bool CanTrade() const;
    void DismissTerminal();
    void Dismiss() { event.reset(); npcName.clear(); }

private:
    std::optional<NpcTalkEvent> event;
    std::string npcName;
    DialogueSessionId dismissedTerminalSessionId = InvalidDialogueSessionId;
    bool activeAuthoritativeSession = false;
};
