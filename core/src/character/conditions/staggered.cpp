#include "condition_effects.hpp"
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
                    .effects = {
                        conditionEffects::actionInhibition("fullRoundInhibition", "Non può compiere azioni di round completo mentre è barcollante", "Barcollante", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::FullRound})),
                        conditionEffects::reminder("roundActionLimit", "Barcollante: in ogni round può compiere una sola azione standard o una sola azione di movimento, ma non entrambe; può comunque compiere azioni gratuite, veloci e immediate.")
                    }
                }
            }
        });
    }
}
