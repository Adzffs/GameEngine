#pragma once

#include <iostream>
#include <string>

class Logger
{
public:
    static void Info(std::string message)
    {
        std::cout << "[INFO] " << message << std::endl;
    }

    static void Debug(std::string message)
    {
#ifdef DEBUG
        std::cout << "[DEBUG] " << message << std::endl;
#endif
    }
};