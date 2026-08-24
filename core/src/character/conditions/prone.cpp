#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/armor_class.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/strike.hpp"

#include <string>

namespace golarion
{
    Condition makeProneCondition()
    {
        return Condition(ConditionDefinition{
            .id = "prone",
            .name = "Prono",
            .stages = {
                ConditionStageDefinition{
                    .id = "prone",
                    .name = "Prono",
                    .effects = {
                        conditionEffects::penalty("meleeAttackPenalty", "–4 ai tiri per colpire in mischia", "Prono", attackResourceName(AttackMode::Melee), "4"),
                        conditionEffects::penalty("meleeArmorClassPenalty", "–4 alla Classe Armatura contro attacchi in mischia", "Prono", std::string(ArmorClassAllResource), "4", "Contro attacchi in mischia"),
                        conditionEffects::genericBonus("rangedArmorClassBonus", "+4 alla Classe Armatura contro attacchi a distanza", "Prono", std::string(ArmorClassAllResource), "4", "Contro attacchi a distanza")
                    }
                }
            }
        });
    }
}
