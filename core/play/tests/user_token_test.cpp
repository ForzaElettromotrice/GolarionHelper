#include "golarion/play/user_token.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
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

    void knownValueAndDigest()
    {
        const UserToken token("0001020304050607");
        check(token.value() == "0001020304050607", "token value changed");
        std::ostringstream hash;
        for (const auto byte : token.digest()) hash << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(byte);
        check(hash.str() == "8a851ff82ee7048ad09ec3847f1ddf44944104d2cbd17ef4e3db22c6785a0d45", "SHA-256 does not hash the original eight bytes");
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
            check(UserToken(value).digest() == token.digest(), "generated token cannot be reconstructed");
        }
    }
}

int main()
{
    try
    {
        knownValueAndDigest();
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
