#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeDeadCondition()
    {
        return Condition(ConditionDefinition{
            .id = "dead",
            .name = "Morto",
            .stages = {
                ConditionStageDefinition{
                    .id = "dead",
                    .name = "Morto",
                    .effects = {}
                }
            }
        });
    }
}
