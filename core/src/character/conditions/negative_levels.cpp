#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/hit_points.hpp"

#include <string>

namespace golarion
{
    Condition makeNegativeLevelsCondition()
    {
        return Condition(ConditionDefinition{
            .id = "negativeLevels",
            .name = "Livelli Negativi",
            .stackingMode = ConditionStackingMode::PerEntry,
            .stages = {
                ConditionStageDefinition{
                    .id = "negativeLevel",
                    .name = "Livello Negativo",
                    .effects = {
                        conditionEffects::perEntryPenalty("attackRolls", "–1 ai tiri per colpire e alle prove di manovra in combattimento", "Livello Negativo", "attack.all", "1"),
                        conditionEffects::perEntryPenalty("savingThrows", "–1 ai tiri salvezza", "Livello Negativo", "savingThrow.all", "1"),
                        conditionEffects::perEntryPenalty("skillChecks", "–1 alle prove di abilità", "Livello Negativo", "skill.all", "1"),
                        conditionEffects::perEntryPenalty("abilityChecks", "–1 alle prove di caratteristica", "Livello Negativo", std::string(AbilityCheckRootResource), "1"),
                        conditionEffects::perEntryPenalty("combatManeuverDefense", "–1 alla Difesa da Manovra in Combattimento", "Livello Negativo", std::string(CombatManeuverDefenseAllResource), "1"),
                        conditionEffects::contribution("hitPoints", "–5 ai punti ferita massimi e attuali", std::string(MaxHitPointsResource), "-5")
                    }
                }
            }
        });
    }
}
