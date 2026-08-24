#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeNauseatedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "nauseated",
            .name = "Nauseato",
            .stages = {
                ConditionStageDefinition{
                    .id = "nauseated",
                    .name = "Nauseato",
                    .effects = {}
                }
            }
        });
    }
}
