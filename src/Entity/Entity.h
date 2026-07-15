#pragma once

class Entity
{
public:
    Entity(int id);

    int GetID();

    void Update();

private:
    int id;
};