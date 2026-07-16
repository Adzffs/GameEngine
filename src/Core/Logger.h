#pragma once

#include <iostream>
#include <string>

class Logger
{
public:
    static void Info(
        const std::string &message)
    {
        std::cout
            << "[INFO] "
            << message
            << std::endl;
    }

    static void Game(
        const std::string &message)
    {
        std::cout
            << "[GAME] "
            << message
            << std::endl;
    }

    static void Error(
        const std::string &message)
    {
        std::cerr
            << "[ERROR] "
            << message
            << std::endl;
    }

    static void Debug(
        const std::string &message)
    {
#ifdef DEBUG
        std::cout
            << "[DEBUG] "
            << message
            << std::endl;
#endif
    }
};