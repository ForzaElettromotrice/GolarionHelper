#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/armor_class.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeStunnedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "stunned",
            .name = "Stordito",
            .stages = {
                ConditionStageDefinition{
                    .id = "stunned",
                    .name = "Stordito",
                    .effects = {
                        conditionEffects::penalty("armorClassPenalty", "–2 alla Classe Armatura", "Stordito", std::string(ArmorClassAllResource), "2"),
                        conditionEffects::armorClassAbilitySuppression("armorClassAbilityBonusSuppression", "Perde il bonus di caratteristica alla Classe Armatura", "Stordito"),
                        conditionEffects::actionInhibition("actionInhibition", "Non può compiere azioni mentre è stordito", "Stordito", ActionSelector{}),
                        conditionEffects::reminder("dropHeldItems", "Stordito: lascia cadere a terra tutto ciò che impugna.")
                    }
                }
            }
        });
    }
}
