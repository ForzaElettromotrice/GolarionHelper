#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeUnconsciousCondition()
    {
        return Condition(ConditionDefinition{
            .id = "unconscious",
            .name = "Privo di Sensi",
            .stages = {
                ConditionStageDefinition{
                    .id = "unconscious",
                    .name = "Privo di Sensi",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "helpless"}
                    }
                }
            }
        });
    }
}
