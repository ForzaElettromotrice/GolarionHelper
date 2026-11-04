#pragma once

#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    class SpecialDefenses;

    struct FastHealingDefinition
    {
        std::string id;
        std::string source;
        std::string expression;
        std::optional<std::string> applicability = std::nullopt;
        std::string stackingGroup = "fastHealing";
        std::vector<std::string> tags{};
    };

    class FastHealing final
    {
    public:
        explicit FastHealing(FastHealingDefinition definition);

    private:
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        std::string expression_;
        std::optional<std::string> applicability_;
        std::string stackingGroup_;
        std::vector<std::string> tags_;
    };
}
