#include "golarion/persistence/json.hpp"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    using namespace golarion;

    CharacterSheetSaveData original{
        .formatVersion = 14,
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
                        .remainingDuration = GameDuration::fromRounds(8)
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
        }
    };

    const std::string json = persistence::toJson(original);
    const CharacterSheetSaveData restored = persistence::fromJson(json);

    assert(json.find("\"formatVersion\": 14") != std::string::npos);
    assert(json.find("\"armorClass\"") == std::string::npos);
    assert(json.find("\"initiative\"") == std::string::npos);
    assert(json.find("\"savingThrows\"") == std::string::npos);
    assert(json.find("\"modifierGroups\"") == std::string::npos);
    assert(json.find("\"contributionGroups\"") == std::string::npos);
    assert(json.find("\"movementGroups\"") == std::string::npos);
    assert(json.find("\"clockwork\"") != std::string::npos);
    assert(restored.formatVersion == 14);
    assert(restored.abilities.size() == 2);
    assert(restored.abilities[0].type == AbilityType::Strength);
    assert(restored.abilities[0].baseValue == 16);
    assert(restored.hitPoints.baseMax == 10);
    assert(restored.hitPoints.damageTaken == 3);
    assert(restored.hitPoints.temporary.pools.size() == 1);
    assert(restored.hitPoints.temporary.pools[0].id == "spell.aid");
    assert(restored.hitPoints.temporary.pools[0].remaining == 3);
    assert(restored.hitPoints.temporary.pools[0].remainingDuration->roundCount() == 8);
    assert(restored.hitPoints.nonLethal == 2);
    assert(restored.skills.skills.size() == 2);
    assert(restored.skills.skills[0].type == SkillType::Acrobatics);
    assert(restored.skills.skills[0].ranks == 2);
    assert(restored.skills.skills[1].specializationId == "clockwork");
    assert(restored.skills.skills[1].specialization == "Meccanismi");
    assert(restored.skills.skills[1].custom);
    return 0;
}
