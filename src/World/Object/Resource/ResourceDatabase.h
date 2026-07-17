#pragma once

#include "ResourceDefinition.h"

class ResourceDatabase
{
public:
    static const ResourceDefinition &Get(
        ResourceType resourceType);
};