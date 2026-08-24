#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeConfusedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "confused",
            .name = "Confuso",
            .stages = {
                ConditionStageDefinition{
                    .id = "confused",
                    .name = "Confuso",
                    .effects = {}
                }
            }
        });
    }
}
