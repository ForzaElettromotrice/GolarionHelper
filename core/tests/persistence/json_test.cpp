#include "golarion/persistence/json.hpp"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    using namespace golarion;

    CharacterSheetSaveData original{
        .formatVersion = 24,
        .identity = CharacterIdentitySaveData{
            .name = "Merisiel",
            .playerName = "Giocatrice",
            .alignment = Alignment::ChaoticNeutral,
            .deity = "Calistria",
            .homeland = "Varisia",
            .gender = "Donna",
            .age = 25,
            .heightCentimeters = 173,
            .weightGrams = 62000,
            .hair = "Neri",
            .eyes = "Verdi",
            .appearance = "Mantello scuro"
        },
        .race = RaceSaveData{
            .raceDefinitionId = "human",
            .alternateFeatureIds = {"human.dualTalent"},
            .choices = {
                RacialChoiceSelectionSaveData{
                    .elementId = "human.abilityScores",
                    .choiceId = "ability",
                    .optionIds = {"dexterity"}
                },
                RacialChoiceSelectionSaveData{
                    .elementId = "human.dualTalent",
                    .choiceId = "abilities",
                    .optionIds = {"strength", "constitution"}
                }
            }
        },
        .abilities = std::vector<AbilitySaveData>{
            AbilitySaveData{.type = AbilityType::Strength, .baseValue = 16},
            AbilitySaveData{.type = AbilityType::Dexterity, .baseValue = 12}
        },
        .hitPoints = HitPointsSaveData{
            .baseMax = 10,
            .damageTaken = 3,
            .temporary = TemporaryHitPointsSaveData{
                .pools = std::vector<TemporaryHitPointPoolSaveData>{
                    TemporaryHitPointPoolSaveData{
                        .id = "spell.aid",
                        .remaining = 3,
                        .duration = GameDuration::fromRounds(8)
                    }
                }
            },
            .nonLethal = 2,
            .dead = true
        },
        .skills = SkillsSaveData{
            .skills = std::vector<SkillSaveData>{
                SkillSaveData{
                    .type = SkillType::Acrobatics,
                    .specializationId = std::nullopt,
                    .specialization = std::nullopt,
                    .ranks = 2,
                    .custom = false
                },
                SkillSaveData{
                    .type = SkillType::Craft,
                    .specializationId = "clockwork",
                    .specialization = "Meccanismi",
                    .ranks = 3,
                    .custom = true
                }
            }
        },
        .attacks = AttacksData{
            .attacks = {
                AttackData{
                    .id = "mixedFullAttack",
                    .name = "Completo con spada",
                    .routineId = "base.mixedFullAttack",
                    .assignments = {
                        AttackAssignmentData{
                            .slotId = "weapon",
                            .strikeGrantId = "equippedLongsword"
                        }
                    }
                }
            }
        },
        .conditions = ConditionManagerSaveData{
            .manualEntries = {
                ConditionEntrySaveData{
                    .id = "spell.fear",
                    .conditionId = "fear",
                    .source = "Incantesimo",
                    .severity = 2,
                    .contributesToEscalation = true,
                    .stackingGroup = "spell.fear",
                    .parameter = std::nullopt
                }
            }
        },
        .inventory = InventorySaveData{
            .containers = {
                InventoryContainerSaveData{
                    .id = "home",
                    .name = "Casa",
                    .maximumContentsWeightGrams = 100000,
                    .maximumContentsVolumeMilliliters = std::nullopt,
                    .acceptedItems = ItemSelectorSaveData{
                        .definitionIds = {},
                        .tags = {"adventuringGear"}
                    },
                    .quantityLimits = {
                        ItemQuantityLimitSaveData{
                            .maximumQuantity = 20,
                            .selector = std::nullopt
                        }
                    },
                    .ignoresContentsWeight = false,
                    .ignoresContentsVolume = false,
                    .allowsPossessionEffects = false,
                    .contributesToCarriedWeight = true
                }
            },
            .items = {
                InventoryItemSaveData{
                    .id = "rope.saved",
                    .itemDefinitionId = "hempRope15m",
                    .quantity = 2,
                    .containerId = "home",
                    .equipped = false
                },
                InventoryItemSaveData{
                    .id = "cloak.saved",
                    .itemDefinitionId = "cloakOfResistance1",
                    .quantity = 1,
                    .containerId = "worn",
                    .equipped = true
                },
                InventoryItemSaveData{
                    .id = "belt.saved",
                    .itemDefinitionId = "beltOfPhysicalMight2",
                    .quantity = 1,
                    .choices = {
                        ItemChoiceSelectionSaveData{
                            .choiceId = "abilities",
                            .optionIds = {"strength", "constitution"}
                        }
                    },
                    .containerId = "worn",
                    .equipped = true
                }
            }
        }
    };

    const std::string json = persistence::toJson(original);
    const CharacterSheetSaveData restored = persistence::fromJson(json);

    assert(json.find("\"formatVersion\": 24") != std::string::npos);
    assert(json.find("\"raceDefinitionId\": \"human\"") != std::string::npos);
    assert(json.find("\"human.dualTalent\"") != std::string::npos);
    assert(json.find("\"alignment\": \"chaoticNeutral\"") != std::string::npos);
    assert(json.find("\"durationRounds\": 8") != std::string::npos);
    assert(json.find("\"remainingRounds\"") == std::string::npos);
    assert(json.find("\"armorClass\"") == std::string::npos);
    assert(json.find("\"initiative\"") == std::string::npos);
    assert(json.find("\"savingThrows\"") == std::string::npos);
    assert(json.find("\"modifierGroups\"") == std::string::npos);
    assert(json.find("\"contributionGroups\"") == std::string::npos);
    assert(json.find("\"movementGroups\"") == std::string::npos);
    assert(json.find("\"clockwork\"") != std::string::npos);
    assert(json.find("\"mixedFullAttack\"") != std::string::npos);
    assert(json.find("\"manualEntries\"") != std::string::npos);
    assert(json.find("\"spell.fear\"") != std::string::npos);
    assert(json.find("\"inventory\"") != std::string::npos);
    assert(restored.formatVersion == 24);
    assert(restored.identity.name == "Merisiel");
    assert(restored.identity.playerName == "Giocatrice");
    assert(restored.identity.alignment == Alignment::ChaoticNeutral);
    assert(restored.identity.deity == "Calistria");
    assert(restored.identity.homeland == "Varisia");
    assert(restored.identity.gender == "Donna");
    assert(restored.identity.age == 25);
    assert(restored.identity.heightCentimeters == 173);
    assert(restored.identity.weightGrams == 62000);
    assert(restored.identity.hair == "Neri");
    assert(restored.identity.eyes == "Verdi");
    assert(restored.identity.appearance == "Mantello scuro");
    assert(restored.race.raceDefinitionId == "human");
    assert((restored.race.alternateFeatureIds == std::vector<std::string>{"human.dualTalent"}));
    assert(restored.race.choices.size() == 2);
    assert(restored.race.choices[0].elementId == "human.abilityScores");
    assert((restored.race.choices[0].optionIds == std::vector<std::string>{"dexterity"}));
    assert(restored.race.choices[1].elementId == "human.dualTalent");
    assert((restored.race.choices[1].optionIds == std::vector<std::string>{"strength", "constitution"}));
    assert(restored.abilities.size() == 2);
    assert(restored.abilities[0].type == AbilityType::Strength);
    assert(restored.abilities[0].baseValue == 16);
    assert(restored.hitPoints.baseMax == 10);
    assert(restored.hitPoints.damageTaken == 3);
    assert(restored.hitPoints.temporary.pools.size() == 1);
    assert(restored.hitPoints.temporary.pools[0].id == "spell.aid");
    assert(restored.hitPoints.temporary.pools[0].remaining == 3);
    assert(restored.hitPoints.temporary.pools[0].duration->roundCount() == 8);
    assert(restored.hitPoints.nonLethal == 2);
    assert(restored.hitPoints.dead);
    assert(restored.skills.skills.size() == 2);
    assert(restored.skills.skills[0].type == SkillType::Acrobatics);
    assert(restored.skills.skills[0].ranks == 2);
    assert(restored.skills.skills[1].specializationId == "clockwork");
    assert(restored.skills.skills[1].specialization == "Meccanismi");
    assert(restored.skills.skills[1].custom);
    assert(restored.attacks.attacks.size() == 1);
    assert(restored.attacks.attacks[0].id == "mixedFullAttack");
    assert(restored.attacks.attacks[0].name == "Completo con spada");
    assert(restored.attacks.attacks[0].routineId == "base.mixedFullAttack");
    assert(restored.attacks.attacks[0].assignments.size() == 1);
    assert(restored.attacks.attacks[0].assignments[0].slotId == "weapon");
    assert(restored.attacks.attacks[0].assignments[0].strikeGrantId == "equippedLongsword");
    assert(restored.conditions.manualEntries.size() == 1);
    assert(restored.conditions.manualEntries[0].id == "spell.fear");
    assert(restored.conditions.manualEntries[0].conditionId == "fear");
    assert(restored.conditions.manualEntries[0].source == "Incantesimo");
    assert(restored.conditions.manualEntries[0].severity == 2);
    assert(restored.conditions.manualEntries[0].contributesToEscalation);
    assert(restored.conditions.manualEntries[0].stackingGroup == "spell.fear");
    assert(!restored.conditions.manualEntries[0].parameter.has_value());
    assert(restored.inventory.containers.size() == 1);
    assert(restored.inventory.containers[0].id == "home");
    assert(restored.inventory.containers[0].maximumContentsWeightGrams == 100000);
    assert(restored.inventory.containers[0].acceptedItems.has_value());
    assert(restored.inventory.containers[0].contributesToCarriedWeight);
    assert((restored.inventory.containers[0].acceptedItems->tags == std::vector<std::string>{"adventuringGear"}));
    assert(restored.inventory.containers[0].quantityLimits.size() == 1);
    assert(restored.inventory.items.size() == 3);
    assert(restored.inventory.items[0].id == "rope.saved");
    assert(restored.inventory.items[0].containerId == "home");
    assert(!restored.inventory.items[0].equipped);
    assert(restored.inventory.items[1].equipped);
    assert(restored.inventory.items[2].choices.size() == 1);
    assert(restored.inventory.items[2].choices[0].choiceId == "abilities");
    assert((restored.inventory.items[2].choices[0].optionIds == std::vector<std::string>{"strength", "constitution"}));

    const std::string legacyJson = R"({
        "formatVersion": 16,
        "abilities": [],
        "hitPoints": {"baseMax": 0, "damageTaken": 0, "temporary": {"pools": []}, "nonLethal": 0},
        "skills": {"skills": []},
        "attacks": {"attacks": []}
    })";
    const CharacterSheetSaveData legacy = persistence::fromJson(legacyJson);
    assert(legacy.identity.name.empty());
    assert(legacy.identity.playerName.empty());
    assert(!legacy.identity.alignment.has_value());
    assert(!legacy.race.raceDefinitionId.has_value());
    assert(!legacy.hitPoints.dead);
    assert(legacy.conditions.manualEntries.empty());
    assert(legacy.inventory.containers.empty());
    assert(legacy.inventory.items.empty());
    return 0;
}
