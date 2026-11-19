#pragma once

enum class PlayerInitializationMode
{
    DEVELOPMENT_DEFAULTS,
    EMPTY,
    // Real starting progression; EMPTY remains reserved for save reconstruction.
    FRESH_START
};
