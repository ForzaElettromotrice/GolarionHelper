#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makePetrifiedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "petrified",
            .name = "Pietrificato",
            .stages = {
                ConditionStageDefinition{
                    .id = "petrified",
                    .name = "Pietrificato",
                    .derivedConditions = {
                        DerivedConditionDefinition{.conditionId = "unconscious"}
                    },
                    .effects = {
                        conditionEffects::reminder("bodyIntegrity", "Pietrificato: se il corpo di pietra è incompleto quando torna di carne, rimane incompleto e può subire perdite permanenti di Punti Ferita o altre menomazioni; i pezzi ricongiunti prima della trasformazione evitano queste conseguenze.")
                    }
                }
            }
        });
    }
}
