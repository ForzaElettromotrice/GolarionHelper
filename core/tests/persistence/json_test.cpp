#include "golarion/persistence/json.hpp"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    using namespace golarion;

    CharacterSheetSaveData original{
        .formatVersion = 18,
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
            .nonLethal = 2
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
        }
    };

    const std::string json = persistence::toJson(original);
    const CharacterSheetSaveData restored = persistence::fromJson(json);

    assert(json.find("\"formatVersion\": 18") != std::string::npos);
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
    assert(restored.formatVersion == 18);
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

    const std::string legacyJson = R"({
        "formatVersion": 16,
        "abilities": [],
        "hitPoints": {"baseMax": 0, "damageTaken": 0, "temporary": {"pools": []}, "nonLethal": 0},
        "skills": {"skills": []},
        "attacks": {"attacks": []}
    })";
    assert(persistence::fromJson(legacyJson).conditions.manualEntries.empty());
    return 0;
}
