#include "golarion/character/archetype_definition.hpp"

#include <cassert>
#include <type_traits>
#include <variant>

int main()
{
    using namespace golarion;
    static_assert(std::is_aggregate_v<ArchetypeDefinition>);
    static_assert(std::is_enum_v<ArchetypeFeatureChangeType>);

    // Synthetic definitions: no official archetype catalog is introduced here.
    const ArchetypeDefinition archetype{
        .id = "example.specialist",
        .classType = CharacterClassType::Fighter,
        .name = "Specialista di prova",
        .description = "Modifica una progressione e sostituisce un privilegio.",
        .affectedFeatures = {
            {"fighter.bonusFeats", ArchetypeFeatureChangeType::Alter},
            {"fighter.bravery", ArchetypeFeatureChangeType::Replace}
        },
        .removedFeatureIds = {"fighter.bravery"},
        .removedAcquisitionIds = {"fighter.bonusFeats.2"},
        .addedFeatures = {{
            .id = "example.specialist.training",
            .name = "Addestramento speciale",
            .description = "Benefici ottenuti a livelli successivi.",
            .progression = {
                {
                    .id = "example.specialist.training.2", .level = 2,
                    .effects = {ReminderEffectDefinition{"training", "Ottieni un addestramento speciale."}},
                    .choices = {{
                        .id = "path", .prompt = "Scegli un percorso", .selectionCount = 1,
                        .options = {{
                            .id = "first", .name = "Primo", .description = "Un percorso di esempio.",
                            .progression = {{
                                .id = "example.specialist.path.first.6", .level = 6,
                                .effects = {ContributionEffectDefinition{"scaling", "example.bonus", "@classLevel"}}
                            }}
                        }}
                    }}
                },
                {
                    .id = "example.specialist.training.10", .level = 10,
                    .effects = {ReminderEffectDefinition{"improved", "Ottieni un beneficio aggiuntivo."}}
                }
            }
        }}
    };

    assert(archetype.classType == CharacterClassType::Fighter);
    assert(archetype.affectedFeatures[0].type == ArchetypeFeatureChangeType::Alter);
    assert(archetype.affectedFeatures[1].type == ArchetypeFeatureChangeType::Replace);
    assert(archetype.removedFeatureIds[0] == "fighter.bravery");
    assert(archetype.removedAcquisitionIds[0] == "fighter.bonusFeats.2");
    const auto &progression = archetype.addedFeatures[0].progression;
    assert(progression.size() == 2 && progression[0].level == 2 && progression[1].level == 10);
    const auto &option = progression[0].choices[0].options[0];
    assert(option.progression[0].level == 6);
    assert(std::get<ContributionEffectDefinition>(option.progression[0].effects[0]).expression == "@classLevel");

    // Different acquisition edits can still affect the same original feature.
    const ArchetypeDefinition other{
        .id = "example.other", .classType = CharacterClassType::Fighter,
        .name = "Altro archetipo", .description = "Modifica un'acquisizione successiva.",
        .affectedFeatures = {{"fighter.bonusFeats", ArchetypeFeatureChangeType::Alter}},
        .removedAcquisitionIds = {"fighter.bonusFeats.6"}
    };
    assert(other.removedAcquisitionIds != archetype.removedAcquisitionIds);
    assert(other.affectedFeatures[0].featureId == archetype.affectedFeatures[0].featureId);

    auto copy = archetype;
    copy.addedFeatures[0].progression[0].choices.clear();
    copy.affectedFeatures.clear();
    assert(archetype.affectedFeatures.size() == 2);
    assert(archetype.addedFeatures[0].progression[0].choices.size() == 1);
    return 0;
}
