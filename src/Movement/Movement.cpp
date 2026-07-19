#include "Movement.h"
#include "../World/Map.h"

#include <limits>

bool Movement::Move(Entity &entity, Map &map, int x, int y)
{
    Position &position = entity.GetPosition();

    const bool isHorizontalStep =
        (x == -1 || x == 1) && y == 0;

    const bool isVerticalStep =
        x == 0 && (y == -1 || y == 1);

    if (!isHorizontalStep && !isVerticalStep)
    {
        return false;
    }

    const long long candidateX =
        static_cast<long long>(position.GetX()) + x;

    const long long candidateY =
        static_cast<long long>(position.GetY()) + y;

    if (candidateX < std::numeric_limits<int>::min() ||
        candidateX > std::numeric_limits<int>::max() ||
        candidateY < std::numeric_limits<int>::min() ||
        candidateY > std::numeric_limits<int>::max())
    {
        return false;
    }

    const int newX = static_cast<int>(candidateX);
    const int newY = static_cast<int>(candidateY);

    if (!map.IsValidPosition(newX, newY))
    {
        return false;
    }

    position.SetPosition(newX, newY);

    return true;
}
