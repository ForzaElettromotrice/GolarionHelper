#pragma once

#include "golarion/character/saving_throw.hpp"
#include "golarion/character/spell_types.hpp"
#include "golarion/effect/effect_definition.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace golarion
{
    struct SpellSavingThrowDefinition
    {
        SavingThrowType type;
        SpellSavingThrowEffect effect;
        std::string description;
    };

    // Value representation of the definition already present in the Java model.
    struct SpellDefinition
    {
        std::string id;
        std::string name;
        SpellSchool school;
        std::optional<SpellSubschool> subschool = std::nullopt;
        std::set<SpellDescriptor> descriptors{};
        std::map<CharacterClassType, int> spellLevelByClass{};
        std::map<DomainSpellListType, int> spellLevelByDomain{};
        std::map<BloodlineType, int> spellLevelByBloodline{};
        SpellCastingTimeCategory castingTimeCategory;
        std::string castingTimeDescription;
        std::set<SpellComponentType> components;
        std::optional<std::string> materialComponentDescription = std::nullopt;
        std::optional<int> materialComponentCostInGoldPieces = std::nullopt;
        std::optional<std::string> focusComponentDescription = std::nullopt;
        SpellRangeType rangeType;
        std::string rangeDescription;
        std::optional<std::string> target = std::nullopt;
        std::optional<std::string> effect = std::nullopt;
        std::optional<std::string> area = std::nullopt;
        SpellDurationType durationType;
        std::string durationDescription;
        bool dismissible = false;
        std::vector<SpellSavingThrowDefinition> savingThrows{};
        SpellResistanceType spellResistanceType;
        std::string spellResistanceDescription;
        std::string description;
        std::vector<EffectDefinition> effects{};
        std::string maxTargetsExpression = "-1";
    };

    void validateSpellDefinition(const SpellDefinition &definition);
}
