#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeDazedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "dazed",
            .name = "Frastornato",
            .stages = {
                ConditionStageDefinition{
                    .id = "dazed",
                    .name = "Frastornato",
                    .effects = {
                        conditionEffects::actionInhibition("actionInhibition", "Non può compiere azioni mentre è frastornato", "Frastornato", ActionSelector{})
                    }
                }
            }
        });
    }
}
