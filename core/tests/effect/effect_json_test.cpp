#include "golarion/effect/effect_json.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>
#include <variant>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;
    using Json = nlohmann::json;

    const ModifierEffectDefinition modifier = std::get<ModifierEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "savingThrows"},
        {"type", "modifier"},
        {"resource", "savingThrow.all"},
        {"description", "+1 ai tiri salvezza"},
        {"modifierType", "bonus"},
        {"bonusType", "resistance"},
        {"expression", "1"}
    }));
    assert(modifier.id == "savingThrows");
    assert(modifier.resource == "savingThrow.all");
    assert(modifier.bonusType == BonusType::Resistance);

    const ContributionEffectDefinition contribution = std::get<ContributionEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "hitPoints"},
        {"type", "contribution"},
        {"resource", "hitPoints.max"},
        {"expression", "@conMod * @level"}
    }));
    assert(contribution.expression == "@conMod * @level");

    const SizeBaseEffectDefinition size = std::get<SizeBaseEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "baseSize"},
        {"type", "sizeBase"},
        {"category", "small"}
    }));
    assert(size.category == SizeCategory::Small);

    const MovementGrantEffectDefinition movement = std::get<MovementGrantEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "flight"},
        {"type", "movementGrant"},
        {"movementType", "fly"},
        {"baseSpeedExpression", "12"},
        {"maneuverability", "good"},
        {"affectedByArmor", false},
        {"affectedByLoad", false},
        {"supportsRunning", true}
    }));
    assert(movement.type == MovementType::Fly);
    assert(movement.maneuverability == Maneuverability::Good);

    const CarryingBodyTypeEffectDefinition bodyType = std::get<CarryingBodyTypeEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "bodyType"},
        {"type", "carryingBodyType"},
        {"bodyType", "quadruped"}
    }));
    assert(bodyType.type == CarryingBodyType::Quadruped);

    const ReminderEffectDefinition reminder = std::get<ReminderEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "remember"},
        {"type", "reminder"},
        {"message", "Ricorda questa regola."}
    }));
    assert(reminder.message == "Ricorda questa regola.");

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(effectDefinitionFromJson(Json{
            {"id", "invalid"},
            {"type", "modifier"},
            {"resource", "savingThrow.all"},
            {"description", "Bonus non tipizzato"},
            {"modifierType", "bonus"},
            {"expression", "1"}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(effectDefinitionFromJson(Json{
            {"id", "invalid"},
            {"type", "movementGrant"},
            {"movementType", "land"},
            {"baseSpeedExpression", "6"},
            {"maneuverability", "good"},
            {"affectedByArmor", true},
            {"affectedByLoad", true}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(effectDefinitionFromJson(Json{{"id", "invalid"}, {"type", "unknown"}}));
    }));

    return 0;
}
