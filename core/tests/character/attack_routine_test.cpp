#include "golarion/character/ability.hpp"
#include "golarion/character/attack_routine.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/attack_routines_view.hpp"
#include "golarion/view/resource_manager_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    BaseAttackBonus baseAttackBonus(resourceManager);
    resourceManager.addContribution(BaseAttackBonusResource, Contribution("fighter", "11"));
    std::array abilities{
        AbilityScore(AbilityType::Strength, 16),
        AbilityScore(AbilityType::Dexterity, 14),
        AbilityScore(AbilityType::Constitution),
        AbilityScore(AbilityType::Intelligence),
        AbilityScore(AbilityType::Wisdom),
        AbilityScore(AbilityType::Charisma)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(resourceManager);
    }

    Strikes strikes(resourceManager);
    AttackRoutines attackRoutines(resourceManager, strikes);
    const ResourceManagerView resourceManagerView = resourceManager.toView();
    assert(std::ranges::find(resourceManagerView.collections, AttackRoutinesResource) != resourceManagerView.collections.end());
    assert(std::ranges::find(resourceManagerView.collections, AttackRoutineProgressionGrantsResource) != resourceManagerView.collections.end());
    assert(std::ranges::find(resourceManagerView.collections, AttackRoutineBonusAdjustmentsResource) != resourceManagerView.collections.end());

    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "longsword",
        .source = "Spada lunga equipaggiata",
        .name = "Spada lunga",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Weapon},
        .naturalAttackClassification = std::nullopt,
        .usageChannels = {"hand.right"},
        .damageComponents = {},
        .damageAbility = AbilityType::Strength,
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 19,
        .criticalMultiplier = 2,
        .defenseType = ArmorClassType::Normal,
        .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
        .range = std::nullopt,
        .requirements = {},
        .weaponWeight = WeaponWeight::OneHanded
    }));
    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "leftClaw",
        .source = "Forma bestiale",
        .name = "Artiglio sinistro",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Natural},
        .naturalAttackClassification = NaturalAttackClassification::Primary,
        .usageChannels = {"hand.left"},
        .damageComponents = {},
        .damageAbility = AbilityType::Strength,
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 20,
        .criticalMultiplier = 2,
        .defenseType = ArmorClassType::Normal,
        .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
        .range = std::nullopt,
        .requirements = {}
    }));
    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "bite",
        .source = "Forma bestiale",
        .name = "Morso",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Natural},
        .naturalAttackClassification = NaturalAttackClassification::Primary,
        .usageChannels = {"mouth"},
        .damageComponents = {},
        .damageAbility = AbilityType::Strength,
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 20,
        .criticalMultiplier = 2,
        .defenseType = ArmorClassType::Normal,
        .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
        .range = std::nullopt,
        .requirements = {}
    }));

    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "flurry",
        .source = "Raffica di colpi",
        .name = "Raffica di colpi",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "weapon",
                .name = "Arma della raffica",
                .selector = StrikeSelector(StrikeSelectorDefinition{
                    .allowedModes = {AttackMode::Melee},
                    .requiredTags = {AttackTag::Weapon},
                    .forbiddenTags = {AttackTag::Natural},
                    .allowedGrantIds = {"longsword"}
                }),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {
                    RoutineAttackProgression(RoutineAttackProgressionDefinition{
                        .type = RoutineAttackProgressionType::Fixed,
                        .countExpression = "2",
                        .attackBonusAdjustmentExpression = "0"
                    }),
                    RoutineAttackProgression(RoutineAttackProgressionDefinition{
                        .type = RoutineAttackProgressionType::BaseAttackBonusIteratives,
                        .countExpression = std::nullopt,
                        .attackBonusAdjustmentExpression = "0"
                    })
                },
                .damageAbilityRuleOverride = std::nullopt
            })
        }
    }));

    resourceManager.addToCollection(AttackRoutineProgressionGrantsResource, RoutineProgressionGrant(RoutineProgressionGrantDefinition{
        .id = "improvedFlurry",
        .source = "Raffica migliorata",
        .targetRoutineId = "flurry",
        .targetSlotId = "weapon",
        .progression = RoutineAttackProgression(RoutineAttackProgressionDefinition{
            .type = RoutineAttackProgressionType::Fixed,
            .countExpression = "1",
            .attackBonusAdjustmentExpression = "-5"
        })
    }));

    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "mixedFullAttack",
        .source = "Regole base",
        .name = "Attacco completo misto",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "manufacturedWeapon",
                .name = "Arma",
                .selector = StrikeSelector(StrikeSelectorDefinition{
                    .allowedModes = {},
                    .requiredTags = {},
                    .forbiddenTags = {AttackTag::Natural},
                    .allowedGrantIds = {}
                }),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {
                    RoutineAttackProgression(RoutineAttackProgressionDefinition{
                        .type = RoutineAttackProgressionType::BaseAttackBonusIteratives,
                        .countExpression = std::nullopt,
                        .attackBonusAdjustmentExpression = "0"
                    })
                },
                .damageAbilityRuleOverride = std::nullopt
            }),
            RoutineSlot(RoutineSlotDefinition{
                .id = "naturalWeapons",
                .name = "Attacchi naturali disponibili",
                .selector = StrikeSelector(StrikeSelectorDefinition{
                    .allowedModes = {AttackMode::Melee},
                    .requiredTags = {AttackTag::Natural},
                    .forbiddenTags = {},
                    .allowedGrantIds = {}
                }),
                .selectionMode = RoutineSlotSelectionMode::AllMatching,
                .strikeUsage = StrikeUsage::NaturalSecondary,
                .progressions = {
                    RoutineAttackProgression(RoutineAttackProgressionDefinition{
                        .type = RoutineAttackProgressionType::Fixed,
                        .countExpression = "1",
                        .attackBonusAdjustmentExpression = "0"
                    })
                },
                .damageAbilityRuleOverride = std::nullopt
            })
        }
    }));

    AttackRoutinesView view = attackRoutines.toView();
    assert(view.routines.size() == 2);
    assert(view.routines[0].id == "flurry");
    assert(view.routines[0].slots.size() == 1);
    assert(view.routines[0].slots[0].selectionMode == RoutineSlotSelectionMode::ChooseOne);
    assert((view.routines[0].slots[0].progressions[0].attackBonusAdjustments == std::vector<int>{0, 0}));
    assert((view.routines[0].slots[0].progressions[1].attackBonusAdjustments == std::vector<int>{0, -5, -10}));
    assert((view.routines[0].slots[0].progressions[2].attackBonusAdjustments == std::vector<int>{-5}));
    assert(view.routines[0].slots[0].progressions[2].grantId == "improvedFlurry");
    assert(view.routines[0].slots[0].progressions[2].grantSource == "Raffica migliorata");
    assert(view.routines[0].slots[0].candidates.size() == 3);
    assert(view.routines[0].slots[0].candidates[0].grantId == "longsword");
    assert(view.routines[0].slots[0].candidates[0].accepted);
    assert((view.routines[0].slots[0].candidates[0].usageChannels == std::vector<std::string>{"hand.right"}));
    assert(!view.routines[0].slots[0].candidates[1].accepted);

    const AttackRoutineView &mixedView = view.routines[1];
    assert(mixedView.slots.size() == 2);
    assert(mixedView.slots[0].candidates[0].accepted);
    assert(!mixedView.slots[0].candidates[1].accepted);
    assert(mixedView.slots[1].selectionMode == RoutineSlotSelectionMode::AllMatching);
    assert(mixedView.slots[1].strikeUsage == StrikeUsage::NaturalSecondary);
    assert(!mixedView.slots[1].candidates[0].accepted);
    assert(mixedView.slots[1].candidates[1].accepted);
    assert(mixedView.slots[1].candidates[2].accepted);

    assert(displayName(RoutineSlotSelectionMode::ChooseOne) == "Scegli uno");
    assert(displayName(RoutineAttackProgressionType::BaseAttackBonusIteratives) == "Attacchi iterativi da BAB");

    const auto fixedSlot = [](std::string id)
    {
        return RoutineSlot(RoutineSlotDefinition{
            .id = std::move(id),
            .name = "Strike",
            .selector = StrikeSelector(StrikeSelectorDefinition{}),
            .selectionMode = RoutineSlotSelectionMode::ChooseOne,
            .strikeUsage = StrikeUsage::Default,
            .progressions = {
                RoutineAttackProgression(RoutineAttackProgressionDefinition{
                    .type = RoutineAttackProgressionType::Fixed,
                    .countExpression = "1",
                    .attackBonusAdjustmentExpression = "0"
                })
            },
            .damageAbilityRuleOverride = std::nullopt
        });
    };

    assert(throwsInvalidArgument([]
    {
        StrikeSelector(StrikeSelectorDefinition{
            .allowedModes = {},
            .requiredTags = {AttackTag::Natural},
            .forbiddenTags = {AttackTag::Natural},
            .allowedGrantIds = {}
        });
    }));
    assert(throwsInvalidArgument([]
    {
        RoutineAttackProgression(RoutineAttackProgressionDefinition{
            .type = RoutineAttackProgressionType::Fixed,
            .countExpression = std::nullopt,
            .attackBonusAdjustmentExpression = "0"
        });
    }));
    assert(throwsInvalidArgument([]
    {
        RoutineSlot(RoutineSlotDefinition{
            .id = "empty",
            .name = "Slot vuoto",
            .selector = StrikeSelector(StrikeSelectorDefinition{}),
            .selectionMode = RoutineSlotSelectionMode::ChooseOne,
            .strikeUsage = StrikeUsage::Default,
            .progressions = {},
            .damageAbilityRuleOverride = std::nullopt
        });
    }));
    assert(throwsInvalidArgument([]
    {
        AttackRoutine(AttackRoutineDefinition{
            .id = "empty",
            .source = "Test",
            .name = "Routine vuota",
            .slots = {}
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        AttackRoutine(AttackRoutineDefinition{
            .id = "duplicateSlots",
            .source = "Test",
            .name = "Slot duplicati",
            .slots = {fixedSlot("strike"), fixedSlot(" strike ")}
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
            .id = "flurry",
            .source = "Test",
            .name = "Duplicata",
            .slots = {
                RoutineSlot(RoutineSlotDefinition{
                    .id = "strike",
                    .name = "Strike",
                    .selector = StrikeSelector(StrikeSelectorDefinition{}),
                    .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                    .strikeUsage = StrikeUsage::Default,
                    .progressions = {
                        RoutineAttackProgression(RoutineAttackProgressionDefinition{
                            .type = RoutineAttackProgressionType::Fixed,
                            .countExpression = "1",
                            .attackBonusAdjustmentExpression = "0"
                        })
                    },
                    .damageAbilityRuleOverride = std::nullopt
                })
            }
        }));
    }));

    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackRoutinesResource, "flurry");
    }));
    resourceManager.removeFromCollection(AttackRoutineProgressionGrantsResource, "improvedFlurry");
    resourceManager.removeFromCollection(AttackRoutinesResource, "flurry");
    resourceManager.removeFromCollection(AttackRoutinesResource, "mixedFullAttack");
    assert(attackRoutines.toView().routines.empty());
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackRoutinesResource, "missing");
    }));

    return 0;
}
