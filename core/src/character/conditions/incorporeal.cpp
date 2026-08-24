#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeIncorporealCondition()
    {
        return Condition(ConditionDefinition{
            .id = "incorporeal",
            .name = "Incorporeo",
            .stages = {
                ConditionStageDefinition{
                    .id = "incorporeal",
                    .name = "Incorporeo",
                    .effects = {}
                }
            }
        });
    }
}
