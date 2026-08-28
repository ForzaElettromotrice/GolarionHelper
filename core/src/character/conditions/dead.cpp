#include "condition_effects.hpp"
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
                    .effects = {
                        conditionEffects::actionInhibition("actionInhibition", "Non può compiere azioni perché è morto", "Morto", ActionSelector{}),
                        conditionEffects::reminder("resurrection", "Morto: la guarigione ordinaria non può riportarlo in vita; serve un effetto di resurrezione appropriato.")
                    }
                }
            }
        });
    }
}
