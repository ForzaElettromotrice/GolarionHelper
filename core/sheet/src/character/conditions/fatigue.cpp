#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeFatigueCondition()
    {
        return Condition(ConditionDefinition{
            .id = "fatigue",
            .name = "Affaticamento",
            .stackingMode = ConditionStackingMode::Escalating,
            .stages = {
                ConditionStageDefinition{
                    .id = "fatigued",
                    .name = "Affaticato",
                    .effects = {
                        conditionEffects::penalty("strengthPenalty", "–2 a Forza", "Affaticato", std::string(resourceName(AbilityType::Strength)), "2"),
                        conditionEffects::penalty("dexterityPenalty", "–2 a Destrezza", "Affaticato", std::string(resourceName(AbilityType::Dexterity)), "2"),
                        conditionEffects::actionInhibition("runInhibition", "Non può correre mentre è affaticato", "Affaticato", ActionSelector(ActionSelectorDefinition{.actionId = "base.run"})),
                        conditionEffects::actionInhibition("chargeInhibition", "Non può caricare mentre è affaticato", "Affaticato", ActionSelector(ActionSelectorDefinition{.actionId = "base.charge"})),
                        conditionEffects::reminder("furtherFatigue", "Affaticato: un'attività che normalmente causerebbe Affaticamento lo rende Esausto; 8 ore di riposo totale rimuovono l'Affaticamento.")
                    }
                },
                ConditionStageDefinition{
                    .id = "exhausted",
                    .name = "Esausto",
                    .effects = {
                        conditionEffects::penalty("additionalStrengthPenalty", "Ulteriore penalità –4 a Forza", "Esausto", std::string(resourceName(AbilityType::Strength)), "4"),
                        conditionEffects::penalty("additionalDexterityPenalty", "Ulteriore penalità –4 a Destrezza", "Esausto", std::string(resourceName(AbilityType::Dexterity)), "4"),
                        conditionEffects::movementAdjustment("halfSpeed", "Velocità dimezzata", "Esausto", MovementAdjustmentType::SpeedMultiplier, "50"),
                        conditionEffects::reminder("recovery", "Esausto: 1 ora di riposo totale riduce la condizione ad Affaticato.")
                    }
                }
            }
        });
    }
}
