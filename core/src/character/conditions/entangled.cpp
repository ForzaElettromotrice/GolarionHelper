#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeEntangledCondition()
    {
        return Condition(ConditionDefinition{
            .id = "entangled",
            .name = "Intralciato",
            .stages = {
                ConditionStageDefinition{
                    .id = "entangled",
                    .name = "Intralciato",
                    .effects = {
                        conditionEffects::penalty("attackRolls", "–2 ai tiri per colpire", "Intralciato", "attack.all", "2"),
                        conditionEffects::penalty("dexterityPenalty", "–4 a Destrezza", "Intralciato", std::string(resourceName(AbilityType::Dexterity)), "4"),
                        conditionEffects::movementAdjustment("halfSpeed", "Velocità dimezzata", "Intralciato", MovementAdjustmentType::SpeedMultiplier, "50"),
                        conditionEffects::actionInhibition("runInhibition", "Non può correre mentre è intralciato", "Intralciato", ActionSelector(ActionSelectorDefinition{.actionId = "base.run"})),
                        conditionEffects::actionInhibition("chargeInhibition", "Non può caricare mentre è intralciato", "Intralciato", ActionSelector(ActionSelectorDefinition{.actionId = "base.charge"})),
                        conditionEffects::reminder("spellcasting", "Intralciato: per lanciare un incantesimo deve superare una prova di Concentrazione con CD 15 + livello dell'incantesimo, altrimenti perde l'incantesimo.")
                    }
                }
            }
        });
    }
}
