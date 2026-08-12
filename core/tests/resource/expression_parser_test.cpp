#include "golarion/resource/expression_parser.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using golarion::ExpressionParser;

    const std::unordered_map<std::string, int> targets{
        {"strength", 18},
        {"ability.strength", 18},
        {"level", 5}
    };
    const auto resolveTarget = [&targets](std::string_view name)
    {
        auto target = targets.find(std::string(name));
        if (target == targets.end())
        {
            throw std::invalid_argument("unknown target: " + std::string(name));
        }
        return target->second;
    };

    assert(ExpressionParser::evaluate("2 + 3 * 4", resolveTarget) == 14);
    assert(ExpressionParser::evaluate("-(2 + 3) * +2", resolveTarget) == -10);
    assert(ExpressionParser::evaluate("@ability.strength / 2 + 2", resolveTarget) == 11);
    assert(ExpressionParser::evaluate("@level / 2", resolveTarget) == 2);
    assert(ExpressionParser::evaluate("@strength >= 18", resolveTarget) == 1);
    assert(ExpressionParser::evaluate("@level < 5", resolveTarget) == 0);
    assert(ExpressionParser::evaluate("@level == 5 && @strength != 10", resolveTarget) == 1);
    assert(ExpressionParser::evaluate("@level > 10 || !0", resolveTarget) == 1);
    assert(ExpressionParser::evaluate("1 + 2 * 3 == 7 && (0 || 4)", resolveTarget) == 1);

    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("4 / 0", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("(2 + 3", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("@ + 2", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("2 value", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate(" ", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("999999999999999999999", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("2147483647 + 1", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("50000 * 50000", resolveTarget);
    }));
    assert(throwsInvalidArgument([&]
    {
        ExpressionParser::evaluate("1 &&", resolveTarget);
    }));

    return 0;
}
