#include "golarion/character/race_definition.hpp"

#include <cassert>
#include <optional>
#include <variant>
#include <vector>

int main()
{
    using namespace golarion;

    const std::vector<RacialChoiceOptionDefinition> abilities{
        {
            .id = "strength",
            .name = "Forza",
            .effects = {ModifierEffectDefinition{
                .id = "abilityBonus",
                .resource = "str",
                .description = "Bonus razziale di Umano",
                .type = ModifierType::Bonus,
                .bonusType = BonusType::Racial,
                .expression = "2"
            }}
        },
        {
            .id = "dexterity",
            .name = "Destrezza",
            .effects = {ModifierEffectDefinition{
                .id = "abilityBonus",
                .resource = "dex",
                .description = "Bonus razziale di Umano",
                .type = ModifierType::Bonus,
                .bonusType = BonusType::Racial,
                .expression = "2"
            }}
        },
        {
            .id = "constitution",
            .name = "Costituzione",
            .effects = {ModifierEffectDefinition{
                .id = "abilityBonus",
                .resource = "con",
                .description = "Bonus razziale di Umano",
                .type = ModifierType::Bonus,
                .bonusType = BonusType::Racial,
                .expression = "2"
            }}
        },
        {
            .id = "intelligence",
            .name = "Intelligenza",
            .effects = {ModifierEffectDefinition{
                .id = "abilityBonus",
                .resource = "int",
                .description = "Bonus razziale di Umano",
                .type = ModifierType::Bonus,
                .bonusType = BonusType::Racial,
                .expression = "2"
            }}
        },
        {
            .id = "wisdom",
            .name = "Saggezza",
            .effects = {ModifierEffectDefinition{
                .id = "abilityBonus",
                .resource = "wis",
                .description = "Bonus razziale di Umano",
                .type = ModifierType::Bonus,
                .bonusType = BonusType::Racial,
                .expression = "2"
            }}
        },
        {
            .id = "charisma",
            .name = "Carisma",
            .effects = {ModifierEffectDefinition{
                .id = "abilityBonus",
                .resource = "cha",
                .description = "Bonus razziale di Umano",
                .type = ModifierType::Bonus,
                .bonusType = BonusType::Racial,
                .expression = "2"
            }}
        }
    };
    const RaceDefinition human{
        .id = "human",
        .name = "Umano",
        .qualities = {
            {
                .id = "human.type",
                .name = "Tipo",
                .description = "Gli Umani sono Umanoidi con il sottotipo Umano.",
                .effects = {
                    CarryingBodyTypeEffectDefinition{
                        .id = "bodyType",
                        .type = CarryingBodyType::Biped
                    },
                    ReminderEffectDefinition{
                        .id = "creatureType",
                        .message = "Il personaggio è un Umanoide con il sottotipo Umano."
                    }
                }
            },
            {
                .id = "human.size",
                .name = "Taglia Media",
                .description = "Gli Umani sono creature Medie.",
                .effects = {SizeBaseEffectDefinition{
                    .id = "size",
                    .category = SizeCategory::Medium
                }}
            },
            {
                .id = "human.baseSpeed",
                .name = "Velocità Normale",
                .description = "Gli Umani hanno una velocità base sul terreno di 9 metri.",
                .effects = {MovementGrantEffectDefinition{
                    .id = "landSpeed",
                    .type = MovementType::Land,
                    .baseSpeedExpression = "6",
                    .affectedByArmor = true,
                    .affectedByLoad = true,
                    .supportsRunning = true
                }}
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
                .description = "Gli Umani iniziano il gioco parlando Comune.",
                .effects = {ReminderEffectDefinition{
                    .id = "languages",
                    .message = "Il personaggio parla Comune e può scegliere linguaggi bonus."
                }}
            }
        },
        .standardFeatures = {
            {
                .id = "human.skilled",
                .name = "Esperto",
                .description = "Gli Umani ottengono un grado di abilità aggiuntivo a ogni livello.",
                .effects = {ReminderEffectDefinition{
                    .id = "skilled",
                    .message = "Ottieni un grado di abilità aggiuntivo a ogni livello."
                }}
            },
            {
                .id = "human.bonusFeat",
                .name = "Talento Bonus",
                .description = "Gli Umani scelgono un talento aggiuntivo al 1° livello.",
                .effects = {ReminderEffectDefinition{
                    .id = "bonusFeat",
                    .message = "Scegli un talento aggiuntivo al 1° livello."
                }}
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
    assert(human.qualities[3].choices[0].options[0].effects.size() == 1);
    const ModifierEffectDefinition &strengthBonus = std::get<ModifierEffectDefinition>(human.qualities[3].choices[0].options[0].effects[0]);
    assert(strengthBonus.resource == "str");
    assert(strengthBonus.type == ModifierType::Bonus);
    assert(strengthBonus.bonusType == std::optional<BonusType>{BonusType::Racial});
    assert(strengthBonus.expression == "2");
    assert(std::holds_alternative<SizeBaseEffectDefinition>(human.qualities[1].effects[0]));
    assert(std::holds_alternative<MovementGrantEffectDefinition>(human.qualities[2].effects[0]));
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
