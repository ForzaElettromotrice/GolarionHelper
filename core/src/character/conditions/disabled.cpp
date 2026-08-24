#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeDisabledCondition()
    {
        return Condition(ConditionDefinition{
            .id = "disabled",
            .name = "Inabile",
            .stages = {
                ConditionStageDefinition{
                    .id = "disabled",
                    .name = "Inabile",
                    .effects = {
                        conditionEffects::movementAdjustment("halfSpeed", "Velocità dimezzata", "Inabile", MovementAdjustmentType::SpeedMultiplier, "50")
                    }
                }
            }
        });
    }
}
