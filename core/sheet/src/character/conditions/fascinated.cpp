#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeFascinatedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "fascinated",
            .name = "Affascinato",
            .stages = {
                ConditionStageDefinition{
                    .id = "fascinated",
                    .name = "Affascinato",
                    .effects = {
                        conditionEffects::penalty("reactiveSkillChecks", "–4 alle prove di abilità effettuate come reazioni", "Affascinato", "skill.all", "4", "Prove effettuate come reazioni"),
                        conditionEffects::actionInhibition("actionInhibition", "Non può compiere azioni mentre è affascinato", "Affascinato", ActionSelector{}),
                        conditionEffects::reminder("potentialThreat", "Affascinato: una potenziale minaccia concede un nuovo Tiro Salvezza contro l'effetto."),
                        conditionEffects::reminder("obviousThreat", "Affascinato: una minaccia palese interrompe automaticamente l'effetto."),
                        conditionEffects::reminder("allyAssistance", "Affascinato: un alleato può scuoterlo e liberarlo dall'effetto con un'azione standard.")
                    }
                }
            }
        });
    }
}
