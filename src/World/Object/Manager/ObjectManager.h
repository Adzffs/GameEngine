#pragma once

#include "../Resource/ResourceNode.h"
#include <vector>

class ObjectManager
{
public:
    ObjectManager();

    void Update();

    void CreateResource(int x, int y);

private:
    std::vector<ResourceNode> resources;

    int nextID = 1;
};