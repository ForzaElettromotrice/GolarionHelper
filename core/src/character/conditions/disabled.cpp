#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeDisabledCondition()
    {
        return Condition(ConditionDefinition{
            .id = "disabled",
            .name = "Inabile",
            .stages = {
                ConditionStageDefinition{
                    .id = "disabled",
                    .name = "Inabile",
                    .effects = {
                        conditionEffects::movementAdjustment("halfSpeed", "Velocità dimezzata", "Inabile", MovementAdjustmentType::SpeedMultiplier, "50"),
                        conditionEffects::actionInhibition("fullRoundInhibition", "Non può compiere azioni di round completo mentre è inabile", "Inabile", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::FullRound})),
                        conditionEffects::reminder("roundActionLimit", "Inabile: in ogni round può compiere una sola azione standard o una sola azione di movimento, ma non entrambe."),
                        conditionEffects::reminder("strenuousActionDamage", "Inabile: dopo un'azione standard o un'altra azione faticosa perde 1 Punto Ferita, salvo che l'azione aumenti i suoi Punti Ferita.")
                    }
                }
            }
        });
    }
}
