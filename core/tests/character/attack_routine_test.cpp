#include "golarion/character/ability.hpp"
#include "golarion/character/action.hpp"
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
#include <string_view>
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

    const golarion::AttackRoutineView &routineView(const golarion::AttackRoutinesView &view, std::string_view id)
    {
        const auto routine = std::ranges::find(view.routines, id, &golarion::AttackRoutineView::id);
        assert(routine != view.routines.end());
        return *routine;
    }
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    ActionManager actionManager(resourceManager);
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
    AttackRoutines attackRoutines(resourceManager, actionManager, strikes);
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
        .actionId = "base.fullAttack",
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
                .damageAbilityRuleOverride = std::nullopt,
                .handUsage = HandUsage::OneHanded,
                .handRole = AttackHandRole::Primary
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
        .actionId = "base.fullAttack",
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
                .damageAbilityRuleOverride = std::nullopt,
                .handUsage = HandUsage::OneHanded,
                .handRole = AttackHandRole::Primary
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

    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "twoHandedAttack",
        .source = "Regole base",
        .name = "Attacco a due mani",
        .actionId = "base.attack",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "weapon",
                .name = "Arma a due mani",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {
                    RoutineAttackProgression(RoutineAttackProgressionDefinition{
                        .type = RoutineAttackProgressionType::Fixed,
                        .countExpression = "1",
                        .attackBonusAdjustmentExpression = "0"
                    })
                },
                .damageAbilityRuleOverride = std::nullopt,
                .handUsage = HandUsage::TwoHanded,
                .handRole = AttackHandRole::Primary
            })
        }
    }));

    AttackRoutinesView view = attackRoutines.toView();
    assert(view.routines.size() == 6);
    const AttackRoutineView &standardAttackView = routineView(view, StandardAttackRoutineId);
    assert(standardAttackView.name == "Attacco normale");
    assert(standardAttackView.actionId == "base.attack");
    assert(standardAttackView.slots.size() == 1);
    assert(standardAttackView.slots[0].assignmentRequired);
    assert((standardAttackView.slots[0].progressions[0].attackBonusAdjustments == std::vector<int>{0}));
    assert(standardAttackView.slots[0].candidates[0].accepted);
    assert(standardAttackView.slots[0].candidates[1].accepted);

    const AttackRoutineView &fullAttackView = routineView(view, FullAttackRoutineId);
    assert(fullAttackView.name == "Attacco completo");
    assert(fullAttackView.actionId == "base.fullAttack");
    assert(fullAttackView.slots.size() == 2);
    assert(!fullAttackView.slots[0].assignmentRequired);
    assert((fullAttackView.slots[0].progressions[0].attackBonusAdjustments == std::vector<int>{0, -5, -10}));
    assert(fullAttackView.slots[1].selectionMode == RoutineSlotSelectionMode::AllMatching);
    assert(fullAttackView.slots[1].strikeUsage == StrikeUsage::NaturalSecondaryWhenCombined);

    const AttackRoutineView &twoWeaponFightingView = routineView(view, TwoWeaponFightingRoutineId);
    assert(twoWeaponFightingView.name == "Combattere con due armi");
    assert(twoWeaponFightingView.actionId == "base.fullAttack");
    assert(twoWeaponFightingView.slots.size() == 3);
    assert((twoWeaponFightingView.slots[0].progressions[0].attackBonusAdjustments == std::vector<int>{0, -5, -10}));
    assert((twoWeaponFightingView.slots[1].progressions[0].attackBonusAdjustments == std::vector<int>{0}));
    const AttackRoutineView &flurryView = routineView(view, "flurry");
    assert(flurryView.actionId == "base.fullAttack");
    assert(flurryView.actionName == "Attacco completo");
    assert(flurryView.usable);
    assert(flurryView.slots.size() == 1);
    assert(flurryView.slots[0].selectionMode == RoutineSlotSelectionMode::ChooseOne);
    assert((flurryView.slots[0].progressions[0].attackBonusAdjustments == std::vector<int>{0, 0}));
    assert((flurryView.slots[0].progressions[1].attackBonusAdjustments == std::vector<int>{0, -5, -10}));
    assert((flurryView.slots[0].progressions[2].attackBonusAdjustments == std::vector<int>{-5}));
    assert(flurryView.slots[0].progressions[2].grantId == "improvedFlurry");
    assert(flurryView.slots[0].progressions[2].grantSource == "Raffica migliorata");
    assert(flurryView.slots[0].candidates.size() == 3);
    assert(flurryView.slots[0].candidates[0].grantId == "longsword");
    assert(flurryView.slots[0].candidates[0].accepted);
    assert((flurryView.slots[0].candidates[0].usageChannels == std::vector<std::string>{"hand.right"}));
    assert(!flurryView.slots[0].candidates[1].accepted);

    const AttackRoutineView &mixedView = routineView(view, "mixedFullAttack");
    assert(mixedView.slots.size() == 2);
    assert(mixedView.slots[0].candidates[0].accepted);
    assert(!mixedView.slots[0].candidates[1].accepted);
    assert(mixedView.slots[1].selectionMode == RoutineSlotSelectionMode::AllMatching);
    assert(mixedView.slots[1].strikeUsage == StrikeUsage::NaturalSecondary);
    assert(!mixedView.slots[1].candidates[0].accepted);
    assert(mixedView.slots[1].candidates[1].accepted);
    assert(mixedView.slots[1].candidates[2].accepted);

    const AttackRoutineView &twoHandedView = routineView(view, "twoHandedAttack");
    assert(twoHandedView.slots[0].handUsage == HandUsage::TwoHanded);
    assert(twoHandedView.slots[0].handRole == AttackHandRole::Primary);
    assert(twoHandedView.slots[0].candidates[0].accepted);
    assert(twoHandedView.slots[0].candidates[0].effectiveDamageAbilityRule == DamageAbilityRule::OneAndHalfPositiveFullPenalty);

    resourceManager.removeFromCollection(ActionGrantsResource, "base.attack");
    view = attackRoutines.toView();
    assert(!routineView(view, "twoHandedAttack").usable);
    assert(routineView(view, "twoHandedAttack").failureReasons == std::vector<std::string>{"L'azione associata non è registrata"});
    resourceManager.addToCollection(ActionGrantsResource, Action(ActionDefinition{
        .id = "base.attack",
        .source = "Regole base",
        .categoryId = "attacks",
        .name = "Attaccare",
        .description = "Effettua un attacco",
        .cost = ActionCost::Standard,
        .tags = {"attack"}
    }));

    resourceManager.addToCollection(ActionInhibitionsResource, ActionInhibition(ActionInhibitionDefinition{
        .id = "staggeredFullAttack",
        .source = "Barcollante",
        .selector = ActionSelector(ActionSelectorDefinition{.actionId = "base.fullAttack"}),
        .reason = "Non può compiere azioni di round completo"
    }));
    view = attackRoutines.toView();
    assert(!routineView(view, "flurry").usable);
    assert(routineView(view, "flurry").failureReasons == std::vector<std::string>{"Barcollante: Non può compiere azioni di round completo"});
    assert(!routineView(view, "mixedFullAttack").usable);
    assert(routineView(view, "twoHandedAttack").usable);
    resourceManager.removeFromCollection(ActionInhibitionsResource, "staggeredFullAttack");

    assert(displayName(RoutineSlotSelectionMode::ChooseOne) == "Scegli uno");
    assert(displayName(RoutineAttackProgressionType::BaseAttackBonusIteratives) == "Attacchi iterativi da BAB");
    assert(displayName(HandUsage::TwoHanded) == "Due mani");
    assert(displayName(AttackHandRole::OffHand) == "Secondaria");

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
            .actionId = "base.attack",
            .slots = {}
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        AttackRoutine(AttackRoutineDefinition{
            .id = "duplicateSlots",
            .source = "Test",
            .name = "Slot duplicati",
            .actionId = "base.attack",
            .slots = {fixedSlot("strike"), fixedSlot(" strike ")}
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
            .id = "flurry",
            .source = "Test",
            .name = "Duplicata",
            .actionId = "base.fullAttack",
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
    assert(throwsInvalidArgument([]
    {
        RoutineSlot(RoutineSlotDefinition{
            .id = "invalidOffHand",
            .name = "Secondaria non valida",
            .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
            .selectionMode = RoutineSlotSelectionMode::ChooseOne,
            .strikeUsage = StrikeUsage::Default,
            .progressions = {
                RoutineAttackProgression(RoutineAttackProgressionDefinition{
                    .type = RoutineAttackProgressionType::Fixed,
                    .countExpression = "1",
                    .attackBonusAdjustmentExpression = "0"
                })
            },
            .damageAbilityRuleOverride = std::nullopt,
            .handUsage = HandUsage::TwoHanded,
            .handRole = AttackHandRole::OffHand
        });
    }));
    resourceManager.removeFromCollection(AttackRoutineProgressionGrantsResource, "improvedFlurry");
    resourceManager.removeFromCollection(AttackRoutinesResource, "flurry");
    resourceManager.removeFromCollection(AttackRoutinesResource, "mixedFullAttack");
    resourceManager.removeFromCollection(AttackRoutinesResource, "twoHandedAttack");
    assert(attackRoutines.toView().routines.size() == 3);
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackRoutinesResource, "missing");
    }));

    return 0;
}
