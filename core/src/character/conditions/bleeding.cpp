#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeBleedingCondition()
    {
        return Condition(ConditionDefinition{
            .id = "bleeding",
            .name = "Sanguinante",
            .stackingMode = ConditionStackingMode::PerEntry,
            .stages = {
                ConditionStageDefinition{
                    .id = "bleeding",
                    .name = "Sanguinante",
                    .effects = {
                        conditionEffects::contextualReminder("damagePerTurn", "Subisce il danno da sanguinamento indicato all'inizio del proprio turno", [](const ConditionEffectContext &context)
                        {
                            return "Sanguinante (" + context.source.value() + "): subisce " + context.parameter.value() + " all'inizio del proprio turno. Il sanguinamento può essere fermato con Guarire CD 15 o con una cura magica; gli effetti dello stesso tipo non si cumulano e si applica il peggiore.";
                        })
                    }
                }
            },
            .entryParameterName = "Danno per turno"
        });
    }
}
