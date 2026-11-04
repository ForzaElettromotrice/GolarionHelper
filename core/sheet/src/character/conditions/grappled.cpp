#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeGrappledCondition()
    {
        return Condition(ConditionDefinition{
            .id = "grappled",
            .name = "In Lotta",
            .stages = {
                ConditionStageDefinition{
                    .id = "grappled",
                    .name = "In Lotta",
                    .effects = {
                        conditionEffects::penalty("dexterityPenalty", "–4 a Destrezza", "In Lotta", std::string(resourceName(AbilityType::Dexterity)), "4"),
                        conditionEffects::penalty("attackAndCombatManeuverPenalty", "–2 ai tiri per colpire e alle prove di manovra in combattimento", "In Lotta", "attack.all", "2"),
                        conditionEffects::genericBonus("grappleCombatManeuverException", "La penalità –2 non si applica alle prove di Lottare o liberarsi da una lotta", "In Lotta", combatManeuverBonusResourceName(CombatManeuverType::Grapple), "2"),
                        conditionEffects::actionInhibition("movementInhibition", "Non può spostarsi mentre è in lotta", "In Lotta", ActionSelector(ActionSelectorDefinition{.requiredTags = {"movement"}})),
                        conditionEffects::reminder("restrictedActions", "In Lotta: non può compiere Attacchi di Opportunità né azioni che richiedono due mani."),
                        conditionEffects::reminder("spellcasting", "In Lotta: per lanciare un incantesimo o usare una capacità magica deve rispettare i componenti disponibili e superare la prova di Concentrazione prevista.")
                    }
                }
            }
        });
    }
}
