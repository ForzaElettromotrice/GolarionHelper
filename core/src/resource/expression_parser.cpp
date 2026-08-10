#include "golarion/resource/expression_parser.hpp"

#include "golarion/util/string_utils.hpp"

#include <charconv>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
    int checkedArithmetic(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("integer arithmetic result is out of range");
        }
        return static_cast<int>(value);
    }

    class Parser final
    {
    public:
        Parser(std::string_view expression, const std::function<int(std::string_view)> &targetResolver)
            : expression_(golarion::normalize(expression)),
              targetResolver_(targetResolver)
        {
            if (!targetResolver_)
            {
                throw std::invalid_argument("target resolver must not be empty");
            }
        }

        int evaluate()
        {
            int value = parseExpression();
            skipWhitespace();
            if (!isAtEnd())
            {
                throw std::invalid_argument("unexpected token at position " + std::to_string(position_));
            }

            return value;
        }

    private:
        int parseExpression()
        {
            int value = parseTerm();

            while (true)
            {
                skipWhitespace();
                if (match('+'))
                {
                    value = checkedArithmetic(static_cast<long long>(value) + parseTerm());
                }
                else if (match('-'))
                {
                    value = checkedArithmetic(static_cast<long long>(value) - parseTerm());
                }
                else
                {
                    return value;
                }
            }
        }

        int parseTerm()
        {
            int value = parseFactor();

            while (true)
            {
                skipWhitespace();
                if (match('*'))
                {
                    value = checkedArithmetic(static_cast<long long>(value) * parseFactor());
                }
                else if (match('/'))
                {
                    const int divisor = parseFactor();
                    if (divisor == 0)
                    {
                        throw std::invalid_argument("division by zero is not allowed");
                    }
                    value = checkedArithmetic(static_cast<long long>(value) / divisor);
                }
                else
                {
                    return value;
                }
            }
        }

        int parseFactor()
        {
            skipWhitespace();

            if (match('+'))
            {
                return parseFactor();
            }
            if (match('-'))
            {
                return checkedArithmetic(-static_cast<long long>(parseFactor()));
            }
            if (match('('))
            {
                const int value = parseExpression();
                skipWhitespace();
                if (!match(')'))
                {
                    throw std::invalid_argument("missing closing ')' at position " + std::to_string(position_));
                }
                return value;
            }
            if (match('@'))
            {
                return parseTarget();
            }
            if (isDigit(peek()))
            {
                return parseNumber();
            }

            throw std::invalid_argument("unexpected token at position " + std::to_string(position_));
        }

        int parseNumber()
        {
            const std::size_t start = position_;
            while (isDigit(peek()))
            {
                ++position_;
            }

            int value = 0;
            const char *begin = expression_.data() + start;
            const char *end = expression_.data() + position_;
            const auto result = std::from_chars(begin, end, value);
            if (result.ec == std::errc::result_out_of_range)
            {
                throw std::invalid_argument("integer literal is out of range at position " + std::to_string(start));
            }
            if (result.ec != std::errc{} || result.ptr != end)
            {
                throw std::invalid_argument("invalid integer at position " + std::to_string(start));
            }

            return value;
        }

        int parseTarget()
        {
            const std::size_t start = position_;
            while (isTargetCharacter(peek()))
            {
                ++position_;
            }

            if (start == position_)
            {
                throw std::invalid_argument("missing target name after '@' at position " + std::to_string(start));
            }

            return targetResolver_(std::string_view(expression_).substr(start, position_ - start));
        }

        void skipWhitespace()
        {
            while (std::isspace(static_cast<unsigned char>(peek())) != 0)
            {
                ++position_;
            }
        }

        bool match(char expected)
        {
            if (peek() != expected)
            {
                return false;
            }

            ++position_;
            return true;
        }

        char peek() const
        {
            return isAtEnd() ? '\0' : expression_[position_];
        }

        bool isAtEnd() const
        {
            return position_ >= expression_.size();
        }

        static bool isDigit(char value)
        {
            return value >= '0' && value <= '9';
        }

        static bool isTargetCharacter(char value)
        {
            const auto character = static_cast<unsigned char>(value);
            return std::isalnum(character) != 0 || value == '_' || value == '.';
        }

        std::string expression_;
        std::function<int(std::string_view)> targetResolver_;
        std::size_t position_ = 0;
    };
}

namespace golarion
{
    int ExpressionParser::evaluate(std::string_view expression, const std::function<int(std::string_view)> &targetResolver)
    {
        return Parser(expression, targetResolver).evaluate();
    }
}
