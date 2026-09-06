#include "DialoguePresentationState.h"

#include "../NPC/NpcDefinitionDatabase.h"
#include <algorithm>

void DialoguePresentationState::Synchronize(
    int localActorEntityID,
    const std::vector<NpcTalkEvent> &publishedEvents,
    const ActiveDialogueSession *activeSession)
{
    activeAuthoritativeSession = false;
    const NpcTalkEvent *latest = nullptr;
    for (const NpcTalkEvent &publishedEvent : publishedEvents)
    {
        if (publishedEvent.actorEntityID == localActorEntityID)
        {
            latest = &publishedEvent;
        }
    }

    if (latest != nullptr)
    {
        if (latest->isTerminal &&
            latest->sessionId == dismissedTerminalSessionId)
        {
            return;
        }

        const bool matchingSessionExists =
            activeSession != nullptr &&
            activeSession->actorEntityID == localActorEntityID &&
            activeSession->sessionId == latest->sessionId &&
            activeSession->npcEntityID == latest->npcEntityID &&
            activeSession->npcType == latest->npcType &&
            activeSession->dialogueId == latest->dialogueId &&
            activeSession->currentNodeId == latest->nodeId;
        if (!latest->isTerminal && !matchingSessionExists)
        {
            event.reset();
            npcName.clear();
            return;
        }

        if (latest->sessionId != dismissedTerminalSessionId)
        {
            dismissedTerminalSessionId = InvalidDialogueSessionId;
        }
        event = *latest;
        activeAuthoritativeSession = matchingSessionExists;
        const NpcDefinition *definition =
            NpcDefinitionDatabase::TryGet(latest->npcType);
        npcName = definition == nullptr ? "NPC" : definition->name;
        return;
    }

    if (!event.has_value()) return;

    const bool matchingSessionExists =
        activeSession != nullptr &&
        activeSession->actorEntityID == localActorEntityID &&
        activeSession->sessionId == event->sessionId;

    // World publishes the terminal node and closes its session in the same
    // tick. Keep that final server-authored snapshot until the player dismisses
    // it; every non-terminal view follows the authoritative session lifetime.
    if (!matchingSessionExists && !event->isTerminal)
    {
        event.reset();
        npcName.clear();
    }
}

std::optional<ServerCommandData>
DialoguePresentationState::MakeContinueCommand() const
{
    if (!event.has_value() ||
        event->nodeKind != DialogueNodeKind::CONTINUE ||
        event->sessionId == InvalidDialogueSessionId)
    {
        return std::nullopt;
    }
    return DialogueContinueCommand{
        event->actorEntityID,
        event->sessionId};
}

std::optional<ServerCommandData>
DialoguePresentationState::MakeChoiceCommand(
    std::size_t choiceIndex) const
{
    if (!event.has_value() ||
        event->nodeKind != DialogueNodeKind::CHOICE ||
        choiceIndex >= event->choices.size() ||
        event->sessionId == InvalidDialogueSessionId)
    {
        return std::nullopt;
    }
    return DialogueChooseCommand{
        event->actorEntityID,
        event->sessionId,
        event->choices[choiceIndex].id};
}

std::optional<ServerCommandData>
DialoguePresentationState::MakeCloseCommand() const
{
    if (!event.has_value() ||
        event->sessionId == InvalidDialogueSessionId)
    {
        return std::nullopt;
    }
    return DialogueCloseCommand{
        event->actorEntityID,
        event->sessionId};
}

bool DialoguePresentationState::CanTrade() const
{
    if (!event || event->isTerminal || !activeAuthoritativeSession) return false;
    // Trading is offered only on the Development Guide's authoritative
    // opening/service node, not throughout the conversation.
    if (event->nodeId != DialogueNodeId::DEVELOPMENT_GUIDE_WELCOME) return false;
    const NpcDefinition *definition = NpcDefinitionDatabase::TryGet(event->npcType);
    return definition != nullptr && std::find(definition->interactions.begin(), definition->interactions.end(), NpcInteractionType::TRADE) != definition->interactions.end();
}

std::optional<ServerCommandData> DialoguePresentationState::MakeTradeCommand() const
{
    if (!CanTrade()) return std::nullopt;
    return NpcInteractionCommand{event->actorEntityID, event->npcEntityID, NpcInteractionType::TRADE};
}

void DialoguePresentationState::DismissTerminal()
{
    if (event.has_value() && event->isTerminal)
    {
        dismissedTerminalSessionId = event->sessionId;
        event.reset();
        npcName.clear();
    }
}
