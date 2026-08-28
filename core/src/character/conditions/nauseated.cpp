#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeNauseatedCondition()
    {
        return Condition(ConditionDefinition{
            .id = "nauseated",
            .name = "Nauseato",
            .stages = {
                ConditionStageDefinition{
                    .id = "nauseated",
                    .name = "Nauseato",
                    .effects = {
                        conditionEffects::actionInhibition("standardActionInhibition", "Può compiere soltanto un'azione di movimento mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::Standard})),
                        conditionEffects::actionInhibition("fullRoundActionInhibition", "Può compiere soltanto un'azione di movimento mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::FullRound})),
                        conditionEffects::actionInhibition("swiftActionInhibition", "Può compiere soltanto un'azione di movimento mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::Swift})),
                        conditionEffects::actionInhibition("immediateActionInhibition", "Può compiere soltanto un'azione di movimento mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::Immediate})),
                        conditionEffects::actionInhibition("freeActionInhibition", "Può compiere soltanto un'azione di movimento mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::Free})),
                        conditionEffects::actionInhibition("replacementAttackInhibition", "Non può attaccare mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.cost = ActionCost::ReplacesAttack})),
                        conditionEffects::actionInhibition("attackInhibition", "Non può attaccare mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.requiredTags = {"attack"}})),
                        conditionEffects::actionInhibition("spellInhibition", "Non può lanciare incantesimi mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.requiredTags = {"spell"}})),
                        conditionEffects::actionInhibition("concentrationInhibition", "Non può concentrarsi mentre è nauseato", "Nauseato", ActionSelector(ActionSelectorDefinition{.requiredTags = {"concentration"}})),
                        conditionEffects::reminder("roundActionLimit", "Nauseato: in ogni turno può compiere una sola azione di movimento; il gestore delle azioni mostra i divieti, ma non conta le azioni già effettuate nel turno.")
                    }
                }
            }
        });
    }
}
