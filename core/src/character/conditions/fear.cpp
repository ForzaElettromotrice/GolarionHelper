#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeFearCondition()
    {
        return Condition(ConditionDefinition{
            .id = "fear",
            .name = "Paura",
            .stackingMode = ConditionStackingMode::Escalating,
            .stages = {
                ConditionStageDefinition{
                    .id = "shaken",
                    .name = "Scosso",
                    .effects = {
                        conditionEffects::penalty("attackRolls", "–2 ai tiri per colpire", "Paura", "attack.all", "2"),
                        conditionEffects::penalty("savingThrows", "–2 ai tiri salvezza", "Paura", "savingThrow.all", "2"),
                        conditionEffects::penalty("skillChecks", "–2 alle prove di abilità", "Paura", "skill.all", "2"),
                        conditionEffects::penalty("abilityChecks", "–2 alle prove di caratteristica", "Paura", std::string(AbilityCheckRootResource), "2")
                    }
                },
                ConditionStageDefinition{.id = "frightened", .name = "Spaventato"},
                ConditionStageDefinition{.id = "panicked", .name = "In preda al panico"}
            }
        });
    }
}
