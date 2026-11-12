#include "golarion/play/session_code.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using golarion::play::SessionCode;

    void check(bool condition, const char *message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    void constructionAndMatching()
    {
        const SessionCode code("a1B2c");
        check(code.value() == "A1B2C", "code was not canonicalized");
        for (const auto candidate : {"A1B2C", "a1b2c", "A1b2C"})
        {
            check(code.matches(candidate), "matching code rejected");
        }
        check(!code.matches("A1B2D"), "different valid code accepted");
        check(SessionCode("az09m").value() == "AZ09M", "ASCII letter/digit boundaries rejected");
        check(SessionCode("00000").matches("00000"), "numeric code rejected");
        check(SessionCode("zzzzz").matches("ZZZZZ"), "alphabetic code rejected");
    }

    void malformedInput()
    {
        const SessionCode code("ABCDE");
        const std::vector<std::string> invalid{
            "", "A", "ABCD", "ABCDEF", " ABCDE", "ABCDE ", "     ",
            "A BCDE", "A BCD", "AB-CD", "AB_CD", "AB.CD", "AB/CD", "AB:CD",
            "AB\nCD", "AB\tCD", std::string("AB\0CD", 5),
            std::string("AB") + static_cast<char>(127) + "CD",
            std::string("AB") + static_cast<char>(128) + "CD",
            std::string("AB") + static_cast<char>(195) + static_cast<char>(169) + "C"
        };
        for (const auto &candidate : invalid)
        {
            bool rejected = false;
            try { const SessionCode invalidCode(candidate); }
            catch (const std::invalid_argument &) { rejected = true; }
            check(rejected, "malformed code constructed");
            check(!code.matches(candidate), "malformed code matched");
            check(code.value() == "ABCDE", "failed verification changed code");
        }
    }

    void ownsItsValue()
    {
        std::string input = "abc12";
        const SessionCode original(input);
        input.assign("ZZZZZ");
        auto copy = original;
        copy = SessionCode("99999");
        check(original.value() == "ABC12", "code aliases input or another instance");
        check(copy.value() == "99999", "value assignment failed");
        const auto moved = std::move(copy);
        check(moved.matches("99999"), "value move failed");
    }

    void generatedCandidates()
    {
        for (int iteration = 0; iteration < 64; ++iteration)
        {
            const auto code = SessionCode::generate();
            const auto value = code.value();
            check(value.size() == 5, "generated code has wrong length");
            std::string lowercase(value);
            for (std::size_t index = 0; index < value.size(); ++index)
            {
                const char character = value[index];
                check((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'), "generated code has invalid characters");
                if (character >= 'A' && character <= 'Z') lowercase[index] = static_cast<char>(character - 'A' + 'a');
            }
            check(code.matches(value) && code.matches(lowercase), "generated code cannot be verified");
        }
    }
}

int main()
{
    try
    {
        constructionAndMatching();
        malformedInput();
        ownsItsValue();
        generatedCandidates();
        std::cout << "session code: PASS\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "session code: " << error.what() << '\n';
        return 1;
    }
}
