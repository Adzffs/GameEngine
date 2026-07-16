#pragma once

#include "../Resource/ResourceNode.h"
#include <vector>

class ObjectManager
{
public:
    ObjectManager();

    void Update();

    void CreateResource(int x, int y);

    const std::vector<ResourceNode> &GetResources() const;
    ResourceNode *GetResourceAt(int x, int y);
    ResourceNode *GetResourceByID(int id);

private:
    std::vector<ResourceNode> resources;

    int nextID = 1;
};