#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeDyingCondition()
    {
        return Condition(ConditionDefinition{
            .id = "dying",
            .name = "Morente",
            .stages = {
                ConditionStageDefinition{
                    .id = "dying",
                    .name = "Morente",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "unconscious"}
                    },
                    .effects = {
                        conditionEffects::reminder("stabilization", "Morente: a ogni turno effettua una prova di Costituzione CD 10, con penalità pari ai Punti Ferita negativi; un 20 naturale riesce automaticamente, un fallimento fa perdere 1 Punto Ferita."),
                        conditionEffects::reminder("deathThreshold", "Morente: muore quando i Punti Ferita negativi raggiungono il punteggio di Costituzione.")
                    }
                }
            }
        });
    }
}
