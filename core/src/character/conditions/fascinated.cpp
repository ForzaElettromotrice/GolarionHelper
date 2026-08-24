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
                        conditionEffects::penalty("reactiveSkillChecks", "–4 alle prove di abilità effettuate come reazioni", "Affascinato", "skill.all", "4", "Prove effettuate come reazioni")
                    }
                }
            }
        });
    }
}
