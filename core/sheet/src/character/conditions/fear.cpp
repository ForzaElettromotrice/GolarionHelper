#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/ability.hpp"
#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeFearCondition()
    {
        return Condition(ConditionDefinition{
            .id = "fear",
            .name = "Paura",
            .stackingMode = ConditionStackingMode::Escalating,
            .stages = {
                ConditionStageDefinition{
                    .id = "shaken",
                    .name = "Scosso",
                    .effects = {
                        conditionEffects::penalty("attackRolls", "–2 ai tiri per colpire", "Paura", "attack.all", "2"),
                        conditionEffects::penalty("savingThrows", "–2 ai tiri salvezza", "Paura", "savingThrow.all", "2"),
                        conditionEffects::penalty("skillChecks", "–2 alle prove di abilità", "Paura", "skill.all", "2"),
                        conditionEffects::penalty("abilityChecks", "–2 alle prove di caratteristica", "Paura", std::string(AbilityCheckRootResource), "2")
                    }
                },
                ConditionStageDefinition{
                    .id = "frightened",
                    .name = "Spaventato",
                    .effects = {
                        conditionEffects::reminder("flee", "Spaventato: deve fuggire il più velocemente possibile dalla fonte della paura, usando anche capacità speciali se sono l'unica via; se non può fuggire può combattere.")
                    }
                },
                ConditionStageDefinition{
                    .id = "panicked",
                    .name = "In preda al panico",
                    .effects = {
                        conditionEffects::reminder("panicBehavior", "In preda al panico: lascia cadere ciò che impugna e fugge alla massima velocità in direzione casuale da ogni pericolo; non può compiere altre azioni e, se non può fuggire, diventa Accovacciato.")
                    }
                }
            }
        });
    }
}
