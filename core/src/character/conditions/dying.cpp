#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeDyingCondition()
    {
        return Condition(ConditionDefinition{
            .id = "dying",
            .name = "Morente",
            .stages = {
                ConditionStageDefinition{
                    .id = "dying",
                    .name = "Morente",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "unconscious"}
                    }
                }
            }
        });
    }
}
