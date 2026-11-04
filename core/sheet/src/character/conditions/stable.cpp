#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeStableCondition()
    {
        return Condition(ConditionDefinition{
            .id = "stable",
            .name = "Stabilizzato",
            .stages = {
                ConditionStageDefinition{
                    .id = "stable",
                    .name = "Stabilizzato",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "unconscious"}
                    },
                    .effects = {
                        conditionEffects::reminder("recovery", "Stabilizzato: ogni ora può effettuare una prova di Costituzione CD 10, con penalità pari ai Punti Ferita negativi, per riprendere i sensi e diventare Inabile."),
                        conditionEffects::reminder("unassistedRisk", "Stabilizzato: se si è stabilizzato da solo e non ha ricevuto aiuto, ogni prova oraria fallita gli fa perdere 1 Punto Ferita.")
                    }
                }
            }
        });
    }
}
