#include "golarion/character/feat_definition.hpp"

#include <cassert>
#include <string>
#include <variant>
#include <vector>

int main()
{
    using namespace golarion;

    const FeatDefinition powerAttack{
        .id = "powerAttack",
        .name = "Attacco Poderoso",
        .description = "Prerequisiti: Forza 13, Bonus di Attacco Base +1. Beneficio: si possono sferrare attacchi in mischia particolarmente efficaci sacrificando la precisione.",
        .types = {"combat"},
        .requirements = {
            FeatRequirementDefinition{
                .description = "Richiede Forza 13",
                .expression = "@str >= 13"
            },
            FeatRequirementDefinition{
                .description = "Richiede Bonus di Attacco Base +1",
                .expression = "@bab >= 1"
            }
        },
        .effects = {ReminderEffectDefinition{
            .id = "powerAttack",
            .message = "Puoi applicare Attacco Poderoso agli attacchi in mischia."
        }}
    };

    assert(powerAttack.id == "powerAttack");
    assert(powerAttack.types == std::vector<std::string>{"combat"});
    assert(powerAttack.requirements.size() == 2);
    assert(powerAttack.requirements[0].expression == "@str >= 13");
    assert(powerAttack.choices.empty());
    assert(std::holds_alternative<ReminderEffectDefinition>(powerAttack.effects[0]));

    const FeatDefinition weaponFocus{
        .id = "weaponFocus",
        .name = "Arma Focalizzata",
        .description = "Prerequisiti: competenza nell'arma prescelta, Bonus di Attacco Base +1. Beneficio: si sceglie un tipo di arma con cui si è particolarmente efficaci. Speciale: si può acquisire questo talento più volte scegliendo ogni volta un'arma diversa.",
        .types = {"combat"},
        .requirements = {FeatRequirementDefinition{
            .description = "Richiede Bonus di Attacco Base +1",
            .expression = "@bab >= 1"
        }},
        .choices = {FeatChoiceDefinition{
            .id = "weapon",
            .prompt = "Scegli un'arma",
            .selectionCount = 1,
            .options = {
                FeatChoiceOptionDefinition{
                    .id = "dagger",
                    .name = "Pugnale",
                    .requirements = {FeatRequirementDefinition{
                        .description = "Richiede competenza nel Pugnale",
                        .expression = "@proficiency.weapon.dagger"
                    }},
                    .effects = {ModifierEffectDefinition{
                        .id = "attackBonus",
                        .resource = "attack.weapon.dagger",
                        .description = "Bonus di Arma Focalizzata",
                        .type = ModifierType::Bonus,
                        .bonusType = BonusType::Generic,
                        .expression = "1"
                    }}
                }
            }
        }}
    };

    assert(weaponFocus.choices.size() == 1);
    assert(weaponFocus.choices[0].options.size() == 1);
    assert(weaponFocus.choices[0].options[0].requirements.size() == 1);
    return 0;
}
