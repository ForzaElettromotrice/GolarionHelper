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
                        conditionEffects::armorClassAbilitySuppression("armorClassAbilityBonusSuppression", "Perde il bonus di caratteristica alla Classe Armatura anche con Schivare Prodigioso", "Immobilizzato"),
                        conditionEffects::actionInhibition("movementInhibition", "Non può muoversi mentre è immobilizzato", "Immobilizzato", ActionSelector(ActionSelectorDefinition{.requiredTags = {"movement"}})),
                        conditionEffects::actionInhibition("attackInhibition", "Non può attaccare mentre è immobilizzato", "Immobilizzato", ActionSelector(ActionSelectorDefinition{.requiredTags = {"attack"}})),
                        conditionEffects::actionInhibition("itemInhibition", "Non può manipolare oggetti mentre è immobilizzato", "Immobilizzato", ActionSelector(ActionSelectorDefinition{.categoryId = "items"})),
                        conditionEffects::reminder("spellcasting", "Immobilizzato: può lanciare soltanto gli incantesimi consentiti dai componenti disponibili e deve superare la prova di Concentrazione prevista dalla lotta.")
                    }
                }
            }
        });
    }
}
