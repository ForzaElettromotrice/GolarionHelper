#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/armor_class.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeCoweringCondition()
    {
        return Condition(ConditionDefinition{
            .id = "cowering",
            .name = "Accovacciato",
            .stages = {
                ConditionStageDefinition{
                    .id = "cowering",
                    .name = "Accovacciato",
                    .effects = {
                        conditionEffects::penalty("armorClassPenalty", "–2 alla Classe Armatura", "Accovacciato", std::string(ArmorClassAllResource), "2"),
                        conditionEffects::armorClassAbilitySuppression("armorClassAbilityBonusSuppression", "Perde il bonus di caratteristica alla Classe Armatura", "Accovacciato"),
                        conditionEffects::actionInhibition("actionInhibition", "Non può compiere azioni mentre è accovacciato", "Accovacciato", ActionSelector{})
                    }
                }
            }
        });
    }
}
