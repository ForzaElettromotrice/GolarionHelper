#pragma once

#include <optional>
#include <string>

namespace golarion
{
    class SpecialDefenses;

    struct ImmunityDefinition
    {
        std::string id;
        std::string source;
        std::string targetId;
        std::string name;
        std::optional<std::string> applicability = std::nullopt;
    };

    class Immunity final
    {
    public:
        explicit Immunity(ImmunityDefinition definition);

    private:
        friend class SpecialDefenses;

        std::string id_;
        std::string source_;
        std::string targetId_;
        std::string name_;
        std::optional<std::string> applicability_;
    };
}
