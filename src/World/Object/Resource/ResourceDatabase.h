#pragma once

#include "ResourceDefinition.h"

#include <vector>

class ResourceDatabase
{
public:
    static const ResourceDefinition &Get(
        ResourceType resourceType);

    static const std::vector<ResourceType> &
    GetAllResourceTypes();
};