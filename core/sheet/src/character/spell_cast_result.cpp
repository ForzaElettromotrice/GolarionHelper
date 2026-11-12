#include "golarion/character/spell_cast_result.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <stdexcept>
#include <utility>

namespace
{
    void freeze(std::string &expression, golarion::ResourceManager &caster)
    {
        expression = std::to_string(caster.evaluateExpression(expression));
    }

    void freeze(std::optional<std::string> &expression, golarion::ResourceManager &caster)
    {
        if (expression) freeze(*expression, caster);
    }

    template<class Effect> void freezeEffect(Effect &value, golarion::ResourceManager &caster)
    {
        if constexpr (requires { value.expression; }) freeze(value.expression, caster);
        if constexpr (requires { value.baseSpeedExpression; }) freeze(value.baseSpeedExpression, caster);
        if constexpr (requires { value.amountExpression; }) freeze(value.amountExpression, caster);
        if constexpr (requires { value.attackPenaltyExpression; }) freeze(value.attackPenaltyExpression, caster);
        if constexpr (requires { value.countExpression; }) freeze(value.countExpression, caster);
        if constexpr (requires { value.attackBonusAdjustmentExpression; }) freeze(value.attackBonusAdjustmentExpression, caster);
        if constexpr (requires { value.baseAttackBonusAdjustmentExpression; }) freeze(value.baseAttackBonusAdjustmentExpression, caster);
        if constexpr (requires { value.progression; }) freezeEffect(value.progression, caster);
        if constexpr (requires { value.progressions; }) for (auto &entry : value.progressions) freezeEffect(entry, caster);
        if constexpr (requires { value.slots; }) for (auto &entry : value.slots) freezeEffect(entry, caster);
        const auto freezeRequirement = [&](golarion::Requirement &requirement)
        {
            auto expression = requirement.expression();
            freeze(expression, caster);
            requirement = golarion::Requirement(std::move(expression), requirement.failureReason());
        };
        if constexpr (requires { value.requirement; }) freezeRequirement(value.requirement);
        if constexpr (requires { value.requirements; }) for (auto &requirement : value.requirements)
            if constexpr (requires { requirement.expression(); }) freezeRequirement(requirement);
        if constexpr (requires { value.entry; }) std::visit([&](auto &entry) { freezeEffect(entry, caster); }, value.entry);
        if constexpr (requires { value.effects; }) for (auto &entry : value.effects)
            std::visit([&](auto &effect) { freezeEffect(effect, caster); }, entry);
        // Conditions are contextual labels, not numeric expressions. Structural
        // selectors, stacking and relative rules remain recipient-side behavior.
    }
}

namespace golarion
{
    SpellCastResult exportSpellCast(const SpellDefinition &spell, ResourceManager &caster, std::optional<MetamagicEvaluation> metamagic)
    {
        SpellCastResult result{caster.evaluateExpression(spell.maxTargetsExpression), spell.effects, std::move(metamagic)};
        if (result.maxTargets != -1 && result.maxTargets <= 0)
            throw std::invalid_argument("spell maximum targets must be positive or -1");
        for (auto &effect : result.effects)
            std::visit([&](auto &value) { freezeEffect(value, caster); }, effect);
        return result;
    }
}
