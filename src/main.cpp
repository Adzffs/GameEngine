#include <iostream>
#include "Core/Engine.h"
#include "Core/ContentStartupValidation.h"

int main()
{
    if (!ContentStartupValidation::ValidateAndLog())
    {
        return 1;
    }

    Engine engine;

    engine.Run();

    return 0;
}