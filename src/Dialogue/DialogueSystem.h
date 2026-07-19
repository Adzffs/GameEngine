#pragma once

#include "DialogueId.h"
#include "DialogueChoiceId.h"
#include "DialogueNodeId.h"
#include "DialogueSessionId.h"
#include "../NPC/NpcType.h"

#include <cstddef>
#include <map>
#include <optional>

struct ActiveDialogueSession
{
    DialogueSessionId sessionId = InvalidDialogueSessionId;
    int actorEntityID = 0;
    int npcEntityID = 0;
    NpcType npcType = NpcType::NONE;
    DialogueId dialogueId = DialogueId::NONE;
    DialogueNodeId currentNodeId = DialogueNodeId::NONE;
    int lastAdvancedTick = 0;
};

class DialogueSystem
{
public:
    DialogueSessionId Start(int actorEntityID, int npcEntityID, NpcType npcType,
                            DialogueId dialogueId, DialogueNodeId startNodeId,
                            int currentTick);
    bool Advance(int actorEntityID, DialogueSessionId sessionId,
                 DialogueNodeId nextNodeId, int currentTick);
    bool CommitContinuation(int actorEntityID, DialogueSessionId sessionId,
                            DialogueNodeId nextNodeId, int currentTick);
    bool CommitChoice(int actorEntityID, DialogueSessionId sessionId,
                      DialogueChoiceId choiceId, DialogueNodeId destinationNodeId,
                      int currentTick);
    bool Close(int actorEntityID, DialogueSessionId sessionId);
    bool CancelActor(int actorEntityID);
    std::size_t CancelTarget(int npcEntityID);
    const ActiveDialogueSession *GetSession(int actorEntityID) const;
    std::size_t GetSessionCount() const;

private:
    DialogueSessionId nextSessionId = 1;
    std::map<int, ActiveDialogueSession> sessionsByActorID;
};
