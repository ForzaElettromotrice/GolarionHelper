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
                        conditionEffects::movementAdjustment("halfSpeed", "Velocità dimezzata", "Intralciato", MovementAdjustmentType::SpeedMultiplier, "50")
                    }
                }
            }
        });
    }
}
