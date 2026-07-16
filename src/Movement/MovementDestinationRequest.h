#pragma once

class MovementDestinationRequest
{
public:
    MovementDestinationRequest(
        int entityID,
        int destinationX,
        int destinationY);

    int GetEntityID() const;
    int GetDestinationX() const;
    int GetDestinationY() const;

private:
    int entityID;
    int destinationX;
    int destinationY;
};