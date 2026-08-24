#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/armor_class.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makePinnedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "pinned",
            .name = "Immobilizzato",
            .stages = {
                ConditionStageDefinition{
                    .id = "pinned",
                    .name = "Immobilizzato",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "flatFooted"}
                    },
                    .effects = {
                        conditionEffects::penalty("armorClassPenalty", "–4 alla Classe Armatura", "Immobilizzato", std::string(ArmorClassAllResource), "4"),
                        conditionEffects::armorClassAbilitySuppression("armorClassAbilityBonusSuppression", "Perde il bonus di caratteristica alla Classe Armatura anche con Schivare Prodigioso", "Immobilizzato")
                    }
                }
            }
        });
    }
}
