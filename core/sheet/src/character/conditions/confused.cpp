#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeConfusedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "confused",
            .name = "Confuso",
            .stages = {
                ConditionStageDefinition{
                    .id = "confused",
                    .name = "Confuso",
                    .effects = {
                        conditionEffects::reminder("turnBehavior", "Confuso: all'inizio di ogni turno tira d%: 01–25 agisce normalmente; 26–50 balbetta senza agire; 51–75 si infligge 1d8 + modificatore di Forza; 76–100 attacca la creatura più vicina."),
                        conditionEffects::reminder("targetsAndRetaliation", "Confuso: considera tutti nemici; gli effetti benefici a contatto richiedono un attacco di contatto riuscito e, se viene attaccato, nel turno seguente attacca l'ultimo aggressore finché è disponibile."),
                        conditionEffects::reminder("opportunityAttacks", "Confuso: non compie Attacchi di Opportunità contro creature con cui non è già impegnato in combattimento.")
                    }
                }
            }
        });
    }
}
