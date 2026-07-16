#include "InputManager.h"

MovementRequest InputManager::GetMovementRequest(int entityID)
{
    return MovementRequest(entityID, 1, 0);
}