#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeHelplessCondition()
    {
        return Condition(ConditionDefinition{
            .id = "helpless",
            .name = "Indifeso",
            .stages = {
                ConditionStageDefinition{
                    .id = "helpless",
                    .name = "Indifeso",
                    .effects = {
                        conditionEffects::finalAbilityReplacement("dexterityFinalReplacement", "Destrezza considerata pari a 0", "Indifeso", AbilityType::Dexterity, "0")
                    }
                }
            }
        });
    }
}
