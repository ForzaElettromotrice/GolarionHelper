#pragma once

#include "golarion/effect/effect_definition.hpp"

namespace golarion
{
    enum class EnhancementOriginType { Custom, Spell, Item, Ability };
    struct EnhancementOrigin
    {
        EnhancementOriginType type = EnhancementOriginType::Custom;
        std::optional<std::string> definitionId;
        std::optional<std::string> sourceSheetId;
    };

    struct EnhancementGroupDefinition
    {
        std::string id;
        std::string name;
        std::vector<EffectDefinition> enhancements{};
        bool active = true;
        std::optional<std::string> description;
        std::optional<EnhancementOrigin> origin;
    };

    std::string enhancementEffectId(std::string_view groupId, std::string_view effectId);
}
