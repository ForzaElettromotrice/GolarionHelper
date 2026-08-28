#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeParalyzedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "paralyzed",
            .name = "Paralizzato",
            .stages = {
                ConditionStageDefinition{
                    .id = "paralyzed",
                    .name = "Paralizzato",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "helpless"}
                    },
                    .effects = {
                        conditionEffects::finalAbilityReplacement("strengthFinalReplacement", "Forza considerata pari a 0", "Paralizzato", AbilityType::Strength, "0"),
                        conditionEffects::movementAdjustment("movementBlock", "Non può utilizzare alcuna modalità di movimento", "Paralizzato", MovementAdjustmentType::Block, std::nullopt),
                        conditionEffects::reminder("mentalActions", "Paralizzato: può compiere esclusivamente azioni mentali."),
                        conditionEffects::reminder("flightAndSwimming", "Paralizzato: se è alato e in volo precipita; se sta nuotando non può continuare a nuotare e potrebbe annegare."),
                        conditionEffects::reminder("occupiedSpace", "Paralizzato: altre creature possono attraversare il suo spazio, ma ogni suo quadretto conta come 2 quadretti di movimento.")
                    }
                }
            }
        });
    }
}
