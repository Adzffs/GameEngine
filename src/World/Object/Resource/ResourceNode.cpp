#include "ResourceNode.h"
#include <iostream>

ResourceNode::ResourceNode(int id, int x, int y)
    : WorldObject(id, x, y)
{
    amount = 10;
}

void ResourceNode::Gather()
{
    if (amount > 0)
    {
        amount--;

        std::cout
            << "Gathered resource. Remaining: "
            << amount
            << std::endl;
    }
    else
    {
        std::cout
            << "Resource depleted"
            << std::endl;
    }
}

void ResourceNode::Interact()
{
    Gather();
}