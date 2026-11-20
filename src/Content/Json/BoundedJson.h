#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// Shared bounded ASCII/integer JSON reader; duplicate keys and excess depth,
// values, strings and containers reject before typed callers inspect fields.
namespace BoundedJson
{
    struct Value
    {
        enum class Type { Object, Array, String, Number, Boolean, Null } type;
        std::map<std::string, Value> object;
        std::vector<Value> array;
        std::string string;
        int number = 0;
        bool boolean = false;
    };

    Value Parse(std::string_view input, std::size_t byteLimit, std::string_view context);
}
