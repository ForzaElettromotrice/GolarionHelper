#include "golarion/character/race_definition.hpp"

#include <cassert>
#include <vector>

int main()
{
    using namespace golarion;

    const std::vector<RacialChoiceOptionDefinition> abilities{
        {.id = "strength", .name = "Forza"},
        {.id = "dexterity", .name = "Destrezza"},
        {.id = "constitution", .name = "Costituzione"},
        {.id = "intelligence", .name = "Intelligenza"},
        {.id = "wisdom", .name = "Saggezza"},
        {.id = "charisma", .name = "Carisma"}
    };
    const RaceDefinition human{
        .id = "human",
        .name = "Umano",
        .qualities = {
            {
                .id = "human.type",
                .name = "Tipo",
                .description = "Gli Umani sono Umanoidi con il sottotipo Umano."
            },
            {
                .id = "human.size",
                .name = "Taglia Media",
                .description = "Gli Umani sono creature Medie."
            },
            {
                .id = "human.baseSpeed",
                .name = "Velocità Normale",
                .description = "Gli Umani hanno una velocità base sul terreno di 9 metri."
            },
            {
                .id = "human.abilityScores",
                .name = "Caratteristiche",
                .description = "Gli Umani ottengono bonus +2 a una caratteristica scelta alla creazione del personaggio.",
                .choices = {{
                    .id = "ability",
                    .prompt = "Scegli una caratteristica",
                    .selectionCount = 1,
                    .options = abilities
                }}
            },
            {
                .id = "human.languages",
                .name = "Linguaggi",
                .description = "Gli Umani iniziano il gioco parlando Comune."
            }
        },
        .standardFeatures = {
            {
                .id = "human.skilled",
                .name = "Esperto",
                .description = "Gli Umani ottengono un grado di abilità aggiuntivo a ogni livello."
            },
            {
                .id = "human.bonusFeat",
                .name = "Talento Bonus",
                .description = "Gli Umani scelgono un talento aggiuntivo al 1° livello."
            }
        },
        .alternateFeatures = {{
            .id = "human.dualTalent",
            .name = "Doppia Dote",
            .description = "Gli Umani ottengono bonus +2 a due caratteristiche diverse.",
            .choices = {{
                .id = "abilities",
                .prompt = "Scegli due caratteristiche diverse",
                .selectionCount = 2,
                .options = abilities
            }},
            .replaces = {"human.abilityScores", "human.skilled", "human.bonusFeat"}
        }}
    };

    assert(human.id == "human");
    assert(human.name == "Umano");
    assert(human.qualities.size() == 5);
    assert(human.qualities[3].choices.size() == 1);
    assert(human.qualities[3].choices[0].selectionCount == 1);
    assert(human.qualities[3].choices[0].options.size() == 6);
    assert(human.standardFeatures.size() == 2);
    assert(human.alternateFeatures.size() == 1);
    assert(human.alternateFeatures[0].choices[0].selectionCount == 2);
    assert((human.alternateFeatures[0].replaces == std::vector<std::string>{
        "human.abilityScores",
        "human.skilled",
        "human.bonusFeat"
    }));

    return 0;
}
