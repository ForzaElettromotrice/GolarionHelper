#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeStaggeredCondition()
    {
        return Condition(ConditionDefinition{
            .id = "staggered",
            .name = "Barcollante",
            .stages = {
                ConditionStageDefinition{
                    .id = "staggered",
                    .name = "Barcollante",
                    .effects = {}
                }
            }
        });
    }
}
