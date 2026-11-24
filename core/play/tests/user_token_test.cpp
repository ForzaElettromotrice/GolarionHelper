#include "golarion/play/user_token.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using golarion::play::UserToken;

    void check(bool condition, const char *message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    void knownValue()
    {
        const UserToken token("0001020304050607");
        check(token.value() == "0001020304050607", "token value changed");
        std::string text = "0123456789abcdef";
        const UserToken copied(text);
        text.assign(16, '0');
        check(copied.value() == "0123456789abcdef", "token aliases input storage");
    }

    void rejectsMalformedValues()
    {
        const std::vector<std::string> invalid{
            "", "0123456789abcde", "0123456789abcdef0", "0123456789ABCDEF",
            " 123456789abcdef", "0123456789abcde ", "0123456789abcdeg",
            std::string("01234567\0abcdef0", 16), std::string(16, static_cast<char>(128))
        };
        for (const auto &value : invalid)
        {
            bool rejected = false;
            try { const UserToken token(value); }
            catch (const std::invalid_argument &) { rejected = true; }
            check(rejected, "malformed or normalized token accepted");
        }
    }

    void generatedValues()
    {
        for (int iteration = 0; iteration < 64; ++iteration)
        {
            const auto token = UserToken::generate();
            const auto value = token.value();
            check(value.size() == 16, "generated token has wrong length");
            for (const char character : value)
            {
                check((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f'), "generated token is not canonical hexadecimal");
            }
            check(UserToken(value).value() == value, "generated token cannot be reconstructed");
        }
    }
}

int main()
{
    try
    {
        knownValue();
        rejectsMalformedValues();
        generatedValues();
        std::cout << "user token: PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "user token: " << error.what() << '\n';
        return 1;
    }
}
