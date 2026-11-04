#include "condition_effects.hpp"
#include "condition_factories.hpp"

#include "golarion/character/condition.hpp"

#include <string>

namespace golarion
{
    Condition makeInvisibleCondition()
    {
        const std::string applicability = "Contro avversari che non percepiscono l'attaccante";
        return Condition(ConditionDefinition{
            .id = "invisible",
            .name = "Invisibile",
            .stages = {
                ConditionStageDefinition{
                    .id = "invisible",
                    .name = "Invisibile",
                    .effects = {
                        conditionEffects::genericBonus("attackRollBonus", "+2 ai tiri per colpire contro avversari che non percepiscono l'attaccante", "Invisibile", "attack.all", "2", applicability),
                        conditionEffects::attackDefenseReplacement("flatFootedDefense", "Gli attacchi possono usare la CA da Impreparato contro avversari che non percepiscono l'attaccante", "Invisibile", "attack.all", ArmorClassType::FlatFooted, applicability),
                        conditionEffects::reminder("detection", "Invisibile: gestire manualmente l'individuazione della presenza, la localizzazione del quadretto e i sensi che possono rivelarlo."),
                        conditionEffects::reminder("concealment", "Invisibile: contro chi non lo percepisce dispone di Occultamento Totale e della relativa probabilità di essere mancato.")
                    }
                }
            }
        });
    }
}
