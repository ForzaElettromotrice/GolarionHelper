#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeStableCondition()
    {
        return Condition(ConditionDefinition{
            .id = "stable",
            .name = "Stabilizzato",
            .stages = {
                ConditionStageDefinition{
                    .id = "stable",
                    .name = "Stabilizzato",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "unconscious"}
                    }
                }
            }
        });
    }
}
