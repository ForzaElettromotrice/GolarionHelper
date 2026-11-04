#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/strike.hpp"

#include <string>

namespace golarion
{
    Condition makeSickenedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "sickened",
            .name = "Infermo",
            .stages = {
                ConditionStageDefinition{
                    .id = "sickened",
                    .name = "Infermo",
                    .effects = {
                        conditionEffects::penalty("attackRolls", "–2 ai tiri per colpire", "Infermo", "attack.all", "2"),
                        conditionEffects::penalty("weaponDamageRolls", "–2 ai tiri per i danni delle armi", "Infermo", std::string(WeaponDamageRollResource), "2"),
                        conditionEffects::penalty("savingThrows", "–2 ai tiri salvezza", "Infermo", "savingThrow.all", "2"),
                        conditionEffects::penalty("skillChecks", "–2 alle prove di abilità", "Infermo", "skill.all", "2"),
                        conditionEffects::penalty("abilityChecks", "–2 alle prove di caratteristica", "Infermo", std::string(AbilityCheckRootResource), "2")
                    }
                }
            }
        });
    }
}
