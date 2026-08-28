#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeFlatFootedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "flatFooted",
            .name = "Impreparato",
            .stages = {
                ConditionStageDefinition{
                    .id = "flatFooted",
                    .name = "Impreparato",
                    .effects = {
                        conditionEffects::armorClassAbilitySuppression("armorClassAbilityBonusSuppression", "Perde il bonus di caratteristica alla Classe Armatura", "Impreparato"),
                        conditionEffects::combatManeuverDefenseDexteritySuppression("combatManeuverDefenseDexterityBonusSuppression", "Perde il bonus di Destrezza alla Difesa da Manovra in Combattimento", "Impreparato"),
                        conditionEffects::reminder("opportunityAttacks", "Impreparato: non può compiere Attacchi di Opportunità, salvo capacità come Riflessi in Combattimento.")
                    }
                }
            }
        });
    }
}
