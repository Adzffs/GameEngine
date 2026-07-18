#pragma once

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

class TestContext
{
public:
    void Expect(
        bool condition,
        const std::string &message)
    {
        assertions++;

        if (!condition)
        {
            failures++;
            std::cerr << "[FAIL] " << message << std::endl;
        }
    }

    template <typename Left, typename Right>
    void ExpectEqual(
        const Left &left,
        const Right &right,
        const std::string &message)
    {
        std::ostringstream stream;
        stream << message << " | expected: " << right << " actual: " << left;
        Expect(left == right, stream.str());
    }

    void ExpectNear(
        float actual,
        float expected,
        float tolerance,
        const std::string &message)
    {
        std::ostringstream stream;
        stream << message << " | expected: " << expected << " actual: " << actual;
        Expect(std::fabs(actual - expected) <= tolerance, stream.str());
    }

    int Finish() const
    {
        if (failures == 0)
        {
            std::cout << "[PASS] " << assertions << " assertions" << std::endl;
            return 0;
        }

        std::cerr << "[FAIL] " << failures << " of " << assertions << " assertions failed" << std::endl;
        return 1;
    }

private:
    int assertions = 0;
    int failures = 0;
};