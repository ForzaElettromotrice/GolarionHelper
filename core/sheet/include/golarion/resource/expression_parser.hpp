#pragma once

#include <functional>
#include <string_view>

namespace golarion
{
    class ExpressionParser final
    {
    public:
        static int evaluate(std::string_view expression, const std::function<int(std::string_view)> &targetResolver);
    };
}
