#include "golarion/character/attack_routine.hpp"

#include <optional>
#include <string>
#include <utility>

namespace
{
    constexpr char BaseRulesSource[] = "Regole base";

    golarion::RoutineAttackProgression fixedAttack(std::string adjustment = "0")
    {
        return golarion::RoutineAttackProgression(golarion::RoutineAttackProgressionDefinition{
            .type = golarion::RoutineAttackProgressionType::Fixed,
            .countExpression = "1",
            .attackBonusAdjustmentExpression = std::move(adjustment)
        });
    }

    golarion::RoutineAttackProgression baseAttackBonusIteratives()
    {
        return golarion::RoutineAttackProgression(golarion::RoutineAttackProgressionDefinition{
            .type = golarion::RoutineAttackProgressionType::BaseAttackBonusIteratives,
            .countExpression = std::nullopt,
            .attackBonusAdjustmentExpression = "0"
        });
    }

    golarion::RoutineSlot naturalFullAttackSlot(std::string id)
    {
        return golarion::RoutineSlot(golarion::RoutineSlotDefinition{
            .id = std::move(id),
            .name = "Attacchi naturali",
            .selector = golarion::StrikeSelector(golarion::StrikeSelectorDefinition{
                .allowedModes = {golarion::AttackMode::Melee},
                .requiredTags = {golarion::AttackTag::Natural},
                .forbiddenTags = {},
                .allowedGrantIds = {}
            }),
            .selectionMode = golarion::RoutineSlotSelectionMode::AllMatching,
            .strikeUsage = golarion::StrikeUsage::NaturalSecondaryWhenCombined,
            .progressions = {fixedAttack()},
            .damageAbilityRuleOverride = std::nullopt,
            .baseAttackBonusAdjustmentExpression = "0",
            .handUsage = std::nullopt,
            .handRole = std::nullopt,
            .assignmentRequired = false
        });
    }
}

namespace golarion
{
    void AttackRoutines::registerCanonicalRoutines()
    {
        addRoutine(AttackRoutine(AttackRoutineDefinition{
            .id = std::string(StandardAttackRoutineId),
            .source = BaseRulesSource,
            .name = "Attacco normale",
            .actionId = "base.attack",
            .slots = {
                RoutineSlot(RoutineSlotDefinition{
                    .id = std::string(StandardAttackSlotId),
                    .name = "Strike",
                    .selector = StrikeSelector(StrikeSelectorDefinition{}),
                    .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                    .strikeUsage = StrikeUsage::Default,
                    .progressions = {fixedAttack()},
                    .damageAbilityRuleOverride = std::nullopt,
                    .baseAttackBonusAdjustmentExpression = "0",
                    .handUsage = std::nullopt,
                    .handRole = std::nullopt,
                    .assignmentRequired = true
                })
            }
        }));

        addRoutine(AttackRoutine(AttackRoutineDefinition{
            .id = std::string(FullAttackRoutineId),
            .source = BaseRulesSource,
            .name = "Attacco completo",
            .actionId = "base.fullAttack",
            .slots = {
                RoutineSlot(RoutineSlotDefinition{
                    .id = std::string(FullAttackWeaponSlotId),
                    .name = "Arma",
                    .selector = StrikeSelector(StrikeSelectorDefinition{
                        .allowedModes = {},
                        .requiredTags = {AttackTag::Weapon},
                        .forbiddenTags = {AttackTag::Natural},
                        .allowedGrantIds = {}
                    }),
                    .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                    .strikeUsage = StrikeUsage::Default,
                    .progressions = {baseAttackBonusIteratives()},
                    .damageAbilityRuleOverride = std::nullopt,
                    .baseAttackBonusAdjustmentExpression = "0",
                    .handUsage = std::nullopt,
                    .handRole = std::nullopt,
                    .assignmentRequired = false
                }),
                naturalFullAttackSlot(std::string(FullAttackNaturalSlotId))
            }
        }));

        addRoutine(AttackRoutine(AttackRoutineDefinition{
            .id = std::string(TwoWeaponFightingRoutineId),
            .source = BaseRulesSource,
            .name = "Combattere con due armi",
            .actionId = "base.fullAttack",
            .slots = {
                RoutineSlot(RoutineSlotDefinition{
                    .id = std::string(TwoWeaponFightingMainHandSlotId),
                    .name = "Arma primaria",
                    .selector = StrikeSelector(StrikeSelectorDefinition{
                        .allowedModes = {},
                        .requiredTags = {AttackTag::Weapon},
                        .forbiddenTags = {},
                        .allowedGrantIds = {}
                    }),
                    .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                    .strikeUsage = StrikeUsage::Default,
                    .progressions = {baseAttackBonusIteratives()},
                    .damageAbilityRuleOverride = std::nullopt,
                    .baseAttackBonusAdjustmentExpression = "-6",
                    .handUsage = HandUsage::OneHanded,
                    .handRole = AttackHandRole::Primary,
                    .assignmentRequired = true
                }),
                RoutineSlot(RoutineSlotDefinition{
                    .id = std::string(TwoWeaponFightingOffHandSlotId),
                    .name = "Arma secondaria",
                    .selector = StrikeSelector(StrikeSelectorDefinition{
                        .allowedModes = {},
                        .requiredTags = {AttackTag::Weapon},
                        .forbiddenTags = {},
                        .allowedGrantIds = {}
                    }),
                    .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                    .strikeUsage = StrikeUsage::Default,
                    .progressions = {fixedAttack()},
                    .damageAbilityRuleOverride = std::nullopt,
                    .baseAttackBonusAdjustmentExpression = "-10",
                    .handUsage = HandUsage::OneHanded,
                    .handRole = AttackHandRole::OffHand,
                    .assignmentRequired = true
                }),
                naturalFullAttackSlot(std::string(TwoWeaponFightingNaturalSlotId))
            }
        }));

        const auto lightOffHandRequirement = []
        {
            return RoutineAssignmentRequirement(RoutineAssignmentRequirementDefinition{
                .slotId = std::string(TwoWeaponFightingOffHandSlotId),
                .strikeGrantId = std::nullopt,
                .requiredTag = std::nullopt,
                .weaponWeightPurpose = WeaponWeightPurpose::TwoWeaponFighting,
                .weaponWeight = WeaponWeight::Light
            });
        };
        addAttackBonusAdjustment(RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition{
            .id = "base.twoWeaponFighting.lightOffHand.mainHand",
            .source = BaseRulesSource,
            .targetRoutineId = std::string(TwoWeaponFightingRoutineId),
            .targetSlotId = std::string(TwoWeaponFightingMainHandSlotId),
            .expression = "2",
            .requirements = {lightOffHandRequirement()}
        }));
        addAttackBonusAdjustment(RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition{
            .id = "base.twoWeaponFighting.lightOffHand.offHand",
            .source = BaseRulesSource,
            .targetRoutineId = std::string(TwoWeaponFightingRoutineId),
            .targetSlotId = std::string(TwoWeaponFightingOffHandSlotId),
            .expression = "2",
            .requirements = {lightOffHandRequirement()}
        }));
    }
}
