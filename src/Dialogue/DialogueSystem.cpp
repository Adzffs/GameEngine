#include "DialogueSystem.h"

DialogueSessionId DialogueSystem::Start(
    int actorEntityID, int npcEntityID, NpcType npcType, DialogueId dialogueId,
    DialogueNodeId startNodeId, int currentTick)
{
    if (actorEntityID <= 0 || npcEntityID <= 0 || npcType == NpcType::NONE ||
        !IsValidDialogueId(dialogueId) || !IsValidDialogueNodeId(startNodeId) ||
        nextSessionId == InvalidDialogueSessionId)
        return InvalidDialogueSessionId;

    const DialogueSessionId sessionId = nextSessionId++;
    sessionsByActorID.insert_or_assign(actorEntityID,
        ActiveDialogueSession{sessionId, actorEntityID, npcEntityID, npcType,
                              dialogueId, startNodeId, currentTick});
    return sessionId;
}

bool DialogueSystem::Advance(int actorEntityID, DialogueSessionId sessionId,
                             DialogueNodeId nextNodeId, int currentTick)
{
    auto found = sessionsByActorID.find(actorEntityID);
    if (sessionId == InvalidDialogueSessionId || found == sessionsByActorID.end() ||
        found->second.sessionId != sessionId ||
        found->second.lastAdvancedTick == currentTick ||
        !IsValidDialogueNodeId(nextNodeId))
        return false;
    found->second.currentNodeId = nextNodeId;
    found->second.lastAdvancedTick = currentTick;
    return true;
}

bool DialogueSystem::Close(int actorEntityID, DialogueSessionId sessionId)
{
    const auto found = sessionsByActorID.find(actorEntityID);
    if (sessionId == InvalidDialogueSessionId || found == sessionsByActorID.end() ||
        found->second.sessionId != sessionId)
        return false;
    sessionsByActorID.erase(found);
    return true;
}

bool DialogueSystem::CancelActor(int actorEntityID)
{
    return sessionsByActorID.erase(actorEntityID) != 0;
}

std::size_t DialogueSystem::CancelTarget(int npcEntityID)
{
    std::size_t removed = 0;
    for (auto iterator = sessionsByActorID.begin(); iterator != sessionsByActorID.end();)
    {
        if (iterator->second.npcEntityID == npcEntityID)
        {
            iterator = sessionsByActorID.erase(iterator);
            ++removed;
        }
        else
            ++iterator;
    }
    return removed;
}

const ActiveDialogueSession *DialogueSystem::GetSession(int actorEntityID) const
{
    const auto found = sessionsByActorID.find(actorEntityID);
    return found == sessionsByActorID.end() ? nullptr : &found->second;
}

std::size_t DialogueSystem::GetSessionCount() const
{
    return sessionsByActorID.size();
}
