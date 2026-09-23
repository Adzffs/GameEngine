#pragma once
#include "QuestDefinition.h"
#include <vector>

namespace QuestDefinitionDatabase
{
    const std::vector<QuestDefinition> &GetAll();
    const QuestDefinition *TryGet(QuestId id);
    bool IsValidDefinition(const QuestDefinition &definition);
    bool IsValidCatalogue(const std::vector<QuestDefinition> &definitions);
}
