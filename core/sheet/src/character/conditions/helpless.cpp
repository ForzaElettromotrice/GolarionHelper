#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeHelplessCondition()
    {
        return Condition(ConditionDefinition{
            .id = "helpless",
            .name = "Indifeso",
            .stages = {
                ConditionStageDefinition{
                    .id = "helpless",
                    .name = "Indifeso",
                    .effects = {
                        conditionEffects::finalAbilityReplacement("dexterityFinalReplacement", "Destrezza considerata pari a 0", "Indifeso", AbilityType::Dexterity, "0"),
                        conditionEffects::reminder("meleeAttacks", "Indifeso: gli attacchi in mischia contro di lui ottengono +4; gli attacchi a distanza non ricevono questo bonus."),
                        conditionEffects::reminder("sneakAttacks", "Indifeso: può subire Attacchi Furtivi."),
                        conditionEffects::reminder("coupDeGrace", "Indifeso: un nemico adiacente può infliggere un colpo di grazia come azione di round completo; colpisce automaticamente, infligge un critico e impone Tempra CD 10 + danni per evitare la morte. Il colpo di grazia provoca Attacchi di Opportunità.")
                    }
                }
            }
        });
    }
}
