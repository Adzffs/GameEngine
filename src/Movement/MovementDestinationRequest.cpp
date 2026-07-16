#include "MovementDestinationRequest.h"

MovementDestinationRequest::MovementDestinationRequest(
    int entityID,
    int destinationX,
    int destinationY)
    : entityID(entityID),
      destinationX(destinationX),
      destinationY(destinationY)
{
}

int MovementDestinationRequest::GetEntityID() const
{
    return entityID;
}

int MovementDestinationRequest::GetDestinationX() const
{
    return destinationX;
}

int MovementDestinationRequest::GetDestinationY() const
{
    return destinationY;
}