#include "golarion/character/class_definition.hpp"

#include <cassert>
#include <type_traits>
#include <variant>

int main()
{
    using namespace golarion;

    static_assert(std::is_aggregate_v<ClassDefinition>);
    static_assert(std::is_aggregate_v<ClassFeatureDefinition>);
    static_assert(std::is_enum_v<ClassHitDie>);

    // Synthetic content exercises the schema without defining a rules catalog.
    const ClassDefinition definition{
        .type = CharacterClassType::Sorcerer,
        .name = "Classe di esempio",
        .description = "Definizione usata per verificare la struttura.",
        .hitDie = ClassHitDie::D6,
        .skillRanksPerLevel = 2,
        .classSkills = {SkillType::Spellcraft, SkillType::KnowledgeArcana},
        .progression = {
            {1, 0, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 2}}},
            {2, 1, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 3}}}
        },
        .features = {{
            .id = "example.path",
            .name = "Percorso",
            .description = "Una scelta con benefici successivi.",
            .progression = {{
                .id = "example.path.selection",
                .level = 1,
                .choices = {{
                    .id = "path",
                    .prompt = "Scegli un percorso",
                    .selectionCount = 1,
                    .options = {{
                        .id = "arcane",
                        .name = "Arcano",
                        .description = "Un percorso di esempio.",
                        .progression = {
                            {
                                .id = "example.path.arcane.initial",
                                .level = 1,
                                .effects = {ContributionEffectDefinition{
                                    .id = "scaling",
                                    .resource = "example.bonus",
                                    // Stored only; class-level targets are future runtime work.
                                    .expression = "@classLevel"
                                }}
                            },
                            {
                                .id = "example.path.arcane.additional",
                                .level = 3,
                                .effects = {ReminderEffectDefinition{
                                    .id = "additional",
                                    .message = "Ottieni una capacità aggiuntiva."
                                }}
                            },
                            {
                                .id = "example.path.arcane.improved",
                                .level = 9,
                                .effects = {ReminderEffectDefinition{
                                    .id = "improved",
                                    .message = "La capacità iniziale è sostituita."
                                }},
                                .replaces = {"example.path.arcane.initial"}
                            }
                        }
                    }}
                }}
            }}
        }}
    };

    assert(definition.type == CharacterClassType::Sorcerer);
    assert(static_cast<int>(definition.hitDie) == 6);
    assert(definition.skillRanksPerLevel == 2);
    assert(definition.classSkills.contains(SkillType::Spellcraft));
    assert(definition.progression.size() == 2);
    assert(definition.progression[1].baseAttackBonus == 1);
    assert(definition.progression[1].baseSavingThrows.at(SavingThrowType::Will) == 3);

    const auto &selection = definition.features[0].progression[0].choices[0];
    assert(selection.selectionCount == 1);
    const auto &stages = selection.options[0].progression;
    assert(stages.size() == 3);
    assert(stages[0].level == 1);
    assert(stages[1].level == 3);
    assert(stages[1].replaces.empty());
    assert(stages[2].level == 9);
    assert(stages[2].replaces[0] == stages[0].id);
    assert(std::get<ContributionEffectDefinition>(stages[0].effects[0]).expression == "@classLevel");

    // Repeated acquisitions own distinct choices, even with identical options.
    ClassFeatureDefinition repeated{
        .id = "example.talents",
        .name = "Doti",
        .description = "Scelte indipendenti a livelli diversi.",
        .progression = {
            {.id = "example.talents.2", .level = 2, .choices = {selection}},
            {.id = "example.talents.4", .level = 4, .choices = {selection}}
        }
    };
    assert(repeated.progression[0].id != repeated.progression[1].id);
    repeated.progression[0].choices[0].options.clear();
    assert(repeated.progression[1].choices[0].options.size() == 1);

    // Definitions own their nested data by value.
    auto copy = definition;
    copy.features[0].progression[0].choices[0].options[0].progression.clear();
    assert(stages.size() == 3);
    return 0;
}
