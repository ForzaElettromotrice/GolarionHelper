#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"
#include "golarion/character/initiative.hpp"

#include <string>

namespace golarion
{
    Condition makeDeafenedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "deafened",
            .name = "Assordato",
            .stages = {
                ConditionStageDefinition{
                    .id = "deafened",
                    .name = "Assordato",
                    .effects = {
                        conditionEffects::penalty("initiativePenalty", "–4 alle prove di Iniziativa", "Assordato", std::string(InitiativeResource), "4"),
                        conditionEffects::reminder("soundPerception", "Assordato: fallisce automaticamente le prove di Percezione basate sul suono."),
                        conditionEffects::reminder("verbalSpellFailure", "Assordato: ha una probabilità del 20% di fallire il lancio degli incantesimi con componenti verbali.")
                    }
                }
            }
        });
    }
}
