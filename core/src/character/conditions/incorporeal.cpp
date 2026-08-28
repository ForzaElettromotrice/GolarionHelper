#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

namespace golarion
{
    Condition makeIncorporealCondition()
    {
        return Condition(ConditionDefinition{
            .id = "incorporeal",
            .name = "Incorporeo",
            .stages = {
                ConditionStageDefinition{
                    .id = "incorporeal",
                    .name = "Incorporeo",
                    .effects = {
                        conditionEffects::reminder("incomingEffects", "Incorporeo: è immune agli attacchi non magici; subisce normalmente gli effetti di Forza e delle fonti incorporee, ma soltanto metà danno dalle fonti magiche corporee. Gli effetti corporei non dannosi hanno il 50% di probabilità di fallire."),
                        conditionEffects::reminder("outgoingAttacks", "Incorporeo: i suoi attacchi ignorano armatura naturale, armatura e scudo, ma non bonus di deviazione ed effetti di Forza."),
                        conditionEffects::reminder("movementAndObjects", "Incorporeo: può attraversare oggetti corporei rispettando copertura, adiacenza e spazio, ma non effetti di Forza; non può Lottare, Sbilanciare o manipolare fisicamente creature ed equipaggiamento."),
                        conditionEffects::reminder("physicalProperties", "Incorporeo: non ha peso, non cade, non subisce danni da caduta e si muove silenziosamente salvo che scelga diversamente.")
                    }
                }
            }
        });
    }
}
