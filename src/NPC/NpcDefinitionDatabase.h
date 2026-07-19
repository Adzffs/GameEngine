#pragma once

#include "NpcDefinition.h"

#include <vector>

namespace NpcDefinitionDatabase
{
    const std::vector<NpcType> &GetAllNpcTypes();
    const NpcDefinition *TryGet(NpcType type);
}
