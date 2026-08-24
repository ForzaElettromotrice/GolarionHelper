#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeBleedingCondition()
    {
        return Condition(ConditionDefinition{
            .id = "bleeding",
            .name = "Sanguinante",
            .stages = {
                ConditionStageDefinition{
                    .id = "bleeding",
                    .name = "Sanguinante"
                }
            },
            .entryParameterName = "Danno per turno"
        });
    }
}
