#pragma once

#include <cstdint>

using BankSessionId = std::uint64_t;
inline constexpr BankSessionId InvalidBankSessionId = 0;

struct ActiveBankSession
{
    BankSessionId sessionId = InvalidBankSessionId;
    int actorEntityID = -1;
    int stationObjectID = -1;
};
