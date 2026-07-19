#pragma once

#include "PlayerSaveData.h"
#include "PlayerSaveValidation.h"

#include <memory>

class Player;

class PlayerSaveState
{
public:
    static PlayerSaveData Capture(const Player &player);

    static PlayerSaveValidationReport Validate(
        const PlayerSaveData &saveData);

    static std::unique_ptr<Player> TryCreatePlayer(
        int runtimeEntityID,
        const PlayerSaveData &saveData,
        PlayerSaveValidationReport &validationReport);

private:
    static PlayerSaveValidationReport ValidateInternal(
        const PlayerSaveData &saveData,
        bool validateEquipmentRequirements);
};
