#include "golarion/persistence/json.hpp"

#include <cassert>
#include <string>
#include <vector>

int main()
{
    using namespace golarion;

    CharacterSheetSaveData original{
        .formatVersion = 10,
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
        .initiative = InitiativeSaveData{.abilityType = AbilityType::Charisma},
        .savingThrows = SavingThrowsSaveData{
            .savingThrows = std::vector<SavingThrowSaveData>{
                SavingThrowSaveData{.type = SavingThrowType::Fortitude, .baseValue = 2, .abilityType = AbilityType::Constitution},
                SavingThrowSaveData{.type = SavingThrowType::Reflex, .baseValue = 1, .abilityType = AbilityType::Charisma},
                SavingThrowSaveData{.type = SavingThrowType::Will, .baseValue = 3, .abilityType = AbilityType::Wisdom}
            }
        },
        .skills = SkillsSaveData{
            .skills = std::vector<SkillSaveData>{
                SkillSaveData{
                    .type = SkillType::Acrobatics,
                    .specializationId = std::nullopt,
                    .specialization = std::nullopt,
                    .abilityType = AbilityType::Dexterity,
                    .ranks = 2,
                    .classSkill = true,
                    .custom = false
                },
                SkillSaveData{
                    .type = SkillType::Craft,
                    .specializationId = "clockwork",
                    .specialization = "Meccanismi",
                    .abilityType = AbilityType::Intelligence,
                    .ranks = 3,
                    .classSkill = true,
                    .custom = true
                }
            }
        },
        .movementGroups = MovementGroupManagerSaveData{
            .groups = std::vector<MovementGroupManagerSaveData::GroupSaveData>{
                MovementGroupManagerSaveData::GroupSaveData{
                    .id = "user.flight",
                    .enabled = true,
                    .group = MovementGroupSaveData{
                        .grants = std::vector<MovementGrantSaveData>{
                            MovementGrantSaveData{
                                .id = "spellFly",
                                .source = "Volare",
                                .type = MovementType::Fly,
                                .baseSpeedExpression = "12",
                                .maneuverability = Maneuverability::Good,
                                .affectedByArmor = true,
                                .affectedByLoad = true
                            }
                        },
                        .adjustments = std::vector<MovementAdjustmentSaveData>{
                            MovementAdjustmentSaveData{
                                .id = "slow",
                                .source = "Lentezza",
                                .description = "Velocità dimezzata",
                                .type = MovementAdjustmentType::SpeedMultiplier,
                                .selector = MovementSelector{.type = std::nullopt, .grantId = std::nullopt},
                                .expression = "50",
                                .condition = std::nullopt
                            }
                        }
                    }
                }
            }
        },
        .modifierGroups = ModifierGroupManagerSaveData{
            .groups = std::vector<ModifierGroupManagerSaveData::GroupSaveData>{
                ModifierGroupManagerSaveData::GroupSaveData{
                    .id = "manual.strength",
                    .enabled = true,
                    .group = ModifierGroupSaveData{
                        .modifiers = std::vector<ModifierGroupSaveData::TargetedModifierSaveData>{
                            ModifierGroupSaveData::TargetedModifierSaveData{
                                .resourceName = "str",
                                .modifier = ModifierSaveData{
                                    .id = "modifier-id",
                                    .type = ModifierType::Bonus,
                                    .source = "Cintura",
                                    .description = "Bonus alla Forza",
                                    .bonusType = BonusType::Enhancement,
                                    .expression = "2 + @level",
                                    .condition = "Contro il veleno"
                                }
                            }
                        }
                    }
                }
            }
        },
        .contributionGroups = ContributionGroupManagerSaveData{
            .groups = std::vector<ContributionGroupManagerSaveData::GroupSaveData>{
                ContributionGroupManagerSaveData::GroupSaveData{
                    .id = "user.hitPoints",
                    .enabled = false,
                    .group = ContributionGroupSaveData{
                        .contributions = std::vector<ContributionGroupSaveData::TargetedContributionSaveData>{
                            ContributionGroupSaveData::TargetedContributionSaveData{
                                .resourceName = "hp.max",
                                .contribution = ContributionSaveData{
                                    .id = "user.toughness",
                                    .expression = "3"
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    const std::string json = persistence::toJson(original);
    const CharacterSheetSaveData restored = persistence::fromJson(json);

    assert(json.find("\"formatVersion\": 10") != std::string::npos);
    assert(json.find("\"enhancement\"") != std::string::npos);
    assert(json.find("\"clockwork\"") != std::string::npos);
    assert(restored.formatVersion == 10);
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
    assert(restored.initiative.abilityType == AbilityType::Charisma);
    assert(restored.savingThrows.savingThrows.size() == 3);
    assert(restored.savingThrows.savingThrows[1].type == SavingThrowType::Reflex);
    assert(restored.savingThrows.savingThrows[1].abilityType == AbilityType::Charisma);
    assert(restored.skills.skills.size() == 2);
    assert(restored.skills.skills[0].type == SkillType::Acrobatics);
    assert(restored.skills.skills[0].ranks == 2);
    assert(restored.skills.skills[1].specializationId == "clockwork");
    assert(restored.skills.skills[1].specialization == "Meccanismi");
    assert(restored.skills.skills[1].custom);
    assert(restored.movementGroups.groups.size() == 1);
    assert(restored.movementGroups.groups[0].id == "user.flight");
    assert(restored.movementGroups.groups[0].enabled);
    assert(restored.movementGroups.groups[0].group.grants[0].type == MovementType::Fly);
    assert(restored.movementGroups.groups[0].group.grants[0].maneuverability == Maneuverability::Good);
    assert(restored.movementGroups.groups[0].group.adjustments[0].type == MovementAdjustmentType::SpeedMultiplier);
    assert(restored.modifierGroups.groups.size() == 1);
    assert(restored.modifierGroups.groups[0].id == "manual.strength");
    assert(restored.modifierGroups.groups[0].enabled);
    assert(restored.modifierGroups.groups[0].group.modifiers[0].resourceName == "str");
    assert(restored.modifierGroups.groups[0].group.modifiers[0].modifier.id == "modifier-id");
    assert(restored.modifierGroups.groups[0].group.modifiers[0].modifier.bonusType == BonusType::Enhancement);
    assert(restored.modifierGroups.groups[0].group.modifiers[0].modifier.condition == "Contro il veleno");
    assert(restored.contributionGroups.groups.size() == 1);
    assert(restored.contributionGroups.groups[0].id == "user.hitPoints");
    assert(!restored.contributionGroups.groups[0].enabled);
    assert(restored.contributionGroups.groups[0].group.contributions[0].resourceName == "hp.max");
    assert(restored.contributionGroups.groups[0].group.contributions[0].contribution.id == "user.toughness");

    return 0;
}
