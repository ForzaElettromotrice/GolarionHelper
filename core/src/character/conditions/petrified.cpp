#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makePetrifiedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "petrified",
            .name = "Pietrificato",
            .stages = {
                ConditionStageDefinition{
                    .id = "petrified",
                    .name = "Pietrificato",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "unconscious"}
                    }
                }
            }
        });
    }
}
