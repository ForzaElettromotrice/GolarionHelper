#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    class SpecialDefenses;

    struct RegenerationDefinition
    {
        std::string id;
        std::string source;
        std::string expression;
        std::optional<std::string> interruption = std::nullopt;
        std::optional<std::string> applicability = std::nullopt;
        std::vector<std::string> tags{};
    };

    class Regeneration final
    {
    public:
        explicit Regeneration(RegenerationDefinition definition);

    private:
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        std::string expression_;
        std::optional<std::string> interruption_;
        std::optional<std::string> applicability_;
        std::vector<std::string> tags_;
    };
}
