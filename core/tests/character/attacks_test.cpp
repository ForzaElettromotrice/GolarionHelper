#include "golarion/character/ability.hpp"
#include "golarion/character/attack.hpp"
#include "golarion/character/attack_routine.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/attacks_view.hpp"

#include <array>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
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

    golarion::RoutineAttackProgression oneAttack()
    {
        return golarion::RoutineAttackProgression(golarion::RoutineAttackProgressionDefinition{
            .type = golarion::RoutineAttackProgressionType::Fixed,
            .countExpression = "1",
            .attackBonusAdjustmentExpression = "0"
        });
    }
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    BaseAttackBonus baseAttackBonus(resourceManager);
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
    AttackRoutines routines(resourceManager, strikes);
    Attacks attacks(routines, strikes);

    const auto addStrike = [&resourceManager](std::string id, std::string name, std::vector<AttackTag> tags, std::optional<NaturalAttackClassification> classification, std::vector<std::string> channels, std::optional<WeaponWeight> weaponWeight = std::nullopt)
    {
        resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
            .id = std::move(id),
            .source = "Test",
            .name = std::move(name),
            .mode = AttackMode::Melee,
            .tags = std::move(tags),
            .naturalAttackClassification = classification,
            .usageChannels = std::move(channels),
            .damageComponents = {},
            .damageAbility = AbilityType::Strength,
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2,
            .defenseType = ArmorClassType::Normal,
            .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
            .range = std::nullopt,
            .requirements = {},
            .weaponWeight = weaponWeight
        }));
    };
    addStrike("longsword", "Spada lunga", {AttackTag::Weapon}, std::nullopt, {"hand.right"}, WeaponWeight::OneHanded);
    addStrike("rightClaw", "Artiglio destro", {AttackTag::Natural}, NaturalAttackClassification::Primary, {"hand.right"});
    addStrike("leftClaw", "Artiglio sinistro", {AttackTag::Natural}, NaturalAttackClassification::Primary, {"hand.left"});
    addStrike("bite", "Morso", {AttackTag::Natural}, NaturalAttackClassification::Primary, {"mouth"});

    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "mixedFullAttack",
        .source = "Regole base",
        .name = "Attacco completo misto",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "weapon",
                .name = "Arma",
                .selector = StrikeSelector(StrikeSelectorDefinition{
                    .allowedModes = {},
                    .requiredTags = {AttackTag::Weapon},
                    .forbiddenTags = {AttackTag::Natural},
                    .allowedGrantIds = {}
                }),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {RoutineAttackProgression(RoutineAttackProgressionDefinition{
                    .type = RoutineAttackProgressionType::BaseAttackBonusIteratives,
                    .countExpression = std::nullopt,
                    .attackBonusAdjustmentExpression = "0"
                })},
                .damageAbilityRuleOverride = std::nullopt
            }),
            RoutineSlot(RoutineSlotDefinition{
                .id = "natural",
                .name = "Attacchi naturali",
                .selector = StrikeSelector(StrikeSelectorDefinition{
                    .allowedModes = {AttackMode::Melee},
                    .requiredTags = {AttackTag::Natural},
                    .forbiddenTags = {},
                    .allowedGrantIds = {}
                }),
                .selectionMode = RoutineSlotSelectionMode::AllMatching,
                .strikeUsage = StrikeUsage::NaturalSecondary,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt
            })
        }
    }));
    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "twoChosenStrikes",
        .source = "Test",
        .name = "Due strike scelti",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "first",
                .name = "Primo",
                .selector = StrikeSelector(StrikeSelectorDefinition{}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt
            }),
            RoutineSlot(RoutineSlotDefinition{
                .id = "second",
                .name = "Secondo",
                .selector = StrikeSelector(StrikeSelectorDefinition{}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt
            })
        }
    }));

    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "twoWeaponFighting",
        .source = "Regole base",
        .name = "Combattere con due armi",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "mainHand",
                .name = "Arma primaria",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt,
                .baseAttackBonusAdjustmentExpression = "-6"
            }),
            RoutineSlot(RoutineSlotDefinition{
                .id = "offHand",
                .name = "Arma secondaria",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt,
                .baseAttackBonusAdjustmentExpression = "-10"
            })
        }
    }));
    resourceManager.addToCollection(AttackRoutineBonusAdjustmentsResource, RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition{
        .id = "lightOffHand",
        .source = "Regole base",
        .targetRoutineId = "twoWeaponFighting",
        .targetSlotId = std::nullopt,
        .expression = "2",
        .requirements = {RoutineAssignmentRequirement(RoutineAssignmentRequirementDefinition{
            .slotId = "offHand",
            .weaponWeightPurpose = WeaponWeightPurpose::TwoWeaponFighting,
            .weaponWeight = WeaponWeight::Light
        })}
    }));
    resourceManager.addToCollection(AttackRoutineBonusAdjustmentsResource, RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition{
        .id = "twoWeaponFightingMain",
        .source = "Combattere con due armi",
        .targetRoutineId = "twoWeaponFighting",
        .targetSlotId = "mainHand",
        .expression = "2",
        .requirements = {}
    }));
    resourceManager.addToCollection(AttackRoutineBonusAdjustmentsResource, RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition{
        .id = "twoWeaponFightingOff",
        .source = "Combattere con due armi",
        .targetRoutineId = "twoWeaponFighting",
        .targetSlotId = "offHand",
        .expression = "6",
        .requirements = {}
    }));

    attacks.create("mixed", "Completo con spada", "mixedFullAttack");
    assert(throwsInvalidArgument([&]
    {
        attacks.create("mixed", "Duplicato", "mixedFullAttack");
    }));
    assert(throwsInvalidArgument([&]
    {
        attacks.create("missingRoutine", "Routine assente", "missing");
    }));

    AttacksView view = attacks.toView();
    assert(view.attacks.size() == 1);
    assert(!view.attacks[0].complete);
    assert(!view.attacks[0].usable);
    assert(view.attacks[0].slots.size() == 2);
    assert(!view.attacks[0].slots[0].assignedGrantId.has_value());
    assert((view.attacks[0].slots[1].effectiveGrantIds == std::vector<std::string>{"rightClaw", "leftClaw", "bite"}));

    assert(throwsInvalidArgument([&]
    {
        attacks.assignStrike("mixed", "natural", "rightClaw");
    }));
    assert(throwsInvalidArgument([&]
    {
        attacks.assignStrike("mixed", "weapon", "rightClaw");
    }));
    attacks.assignStrike("mixed", "weapon", "longsword");
    view = attacks.toView();
    assert(view.attacks[0].complete);
    assert(view.attacks[0].usable);
    assert(view.attacks[0].slots[0].assignedGrantId == "longsword");
    assert((view.attacks[0].slots[0].effectiveGrantIds == std::vector<std::string>{"longsword"}));
    assert((view.attacks[0].slots[1].effectiveGrantIds == std::vector<std::string>{"leftClaw", "bite"}));
    assert(!view.attacks[0].slots[1].candidates[1].accepted);
    assert(view.attacks[0].slots[1].candidates[1].rejectionReasons.back() == "Un canale d'uso dello strike è già occupato");

    const AttacksData saved = attacks.toData();
    assert(saved.attacks.size() == 1);
    assert(saved.attacks[0].routineId == "mixedFullAttack");
    assert(saved.attacks[0].assignments.size() == 1);
    assert(saved.attacks[0].assignments[0].slotId == "weapon");
    assert(saved.attacks[0].assignments[0].strikeGrantId == "longsword");

    addStrike("offhandSword", "Spada secondaria", {AttackTag::Weapon}, std::nullopt, {"hand.left"}, WeaponWeight::OneHanded);
    attacks.create("dualWield", "Due armi", "twoWeaponFighting");
    attacks.assignStrike("dualWield", "mainHand", "longsword");
    attacks.assignStrike("dualWield", "offHand", "offhandSword");
    view = attacks.toView();
    assert(view.attacks[1].slots[0].baseAttackBonusAdjustment == -6);
    assert(view.attacks[1].slots[0].effectiveAttackBonusAdjustment == -4);
    assert(view.attacks[1].slots[1].baseAttackBonusAdjustment == -10);
    assert(view.attacks[1].slots[1].effectiveAttackBonusAdjustment == -4);
    resourceManager.addToCollection(StrikeWeaponWeightAdjustmentsResource, StrikeWeaponWeightAdjustment(StrikeWeaponWeightAdjustmentDefinition{
        .id = "oversizedTwoWeaponFighting",
        .source = "Addestramento con armi sovradimensionate",
        .targetResourceName = "attack.strike.offhandSword",
        .purpose = WeaponWeightPurpose::TwoWeaponFighting,
        .weaponWeight = WeaponWeight::Light
    }));
    resourceManager.addToCollection(AttackRoutineProgressionGrantsResource, RoutineProgressionGrant(RoutineProgressionGrantDefinition{
        .id = "improvedTwoWeaponFighting",
        .source = "Combattere con due armi migliorato",
        .targetRoutineId = "twoWeaponFighting",
        .targetSlotId = "offHand",
        .progression = RoutineAttackProgression(RoutineAttackProgressionDefinition{
            .type = RoutineAttackProgressionType::Fixed,
            .countExpression = "1",
            .attackBonusAdjustmentExpression = "-5"
        })
    }));
    view = attacks.toView();
    assert(view.attacks[1].slots[0].effectiveAttackBonusAdjustment == -2);
    assert(view.attacks[1].slots[1].effectiveAttackBonusAdjustment == -2);
    assert(view.attacks[1].slots[0].appliedAttackBonusAdjustments.size() == 2);
    assert(view.attacks[1].slots[1].appliedAttackBonusAdjustments.size() == 2);
    assert(view.attacks[1].slots[1].progressions.size() == 2);
    assert((view.attacks[1].slots[1].progressions[1].attackBonusAdjustments == std::vector<int>{-5}));
    attacks.remove("dualWield");
    resourceManager.removeFromCollection(AttackRoutineProgressionGrantsResource, "improvedTwoWeaponFighting");
    resourceManager.removeFromCollection(StrikeWeaponWeightAdjustmentsResource, "oversizedTwoWeaponFighting");

    attacks.create("dual", "Due strike", "twoChosenStrikes");
    attacks.assignStrike("dual", "first", "longsword");
    assert(throwsInvalidArgument([&]
    {
        attacks.assignStrike("dual", "second", "rightClaw");
    }));
    attacks.assignStrike("dual", "second", "leftClaw");
    assert(attacks.toView().attacks[1].complete);
    attacks.remove("dual");

    attacks.unassignStrike("mixed", "weapon");
    view = attacks.toView();
    assert((view.attacks[0].slots[1].effectiveGrantIds == std::vector<std::string>{"rightClaw", "leftClaw", "bite"}));
    assert(throwsInvalidArgument([&]
    {
        attacks.unassignStrike("mixed", "weapon");
    }));

    Attacks loadedAttacks(routines, strikes);
    loadedAttacks.load(saved);
    const AttacksView loadedView = loadedAttacks.toView();
    assert(loadedView.attacks.size() == 1);
    assert(loadedView.attacks[0].slots[0].assignedGrantId == "longsword");
    assert((loadedView.attacks[0].slots[1].effectiveGrantIds == std::vector<std::string>{"leftClaw", "bite"}));
    assert(throwsInvalidArgument([&]
    {
        loadedAttacks.load(saved);
    }));

    resourceManager.removeFromCollection(AttackRoutinesResource, "mixedFullAttack");
    const AttacksView missingRoutineView = loadedAttacks.toView();
    assert(!missingRoutineView.attacks[0].complete);
    assert(!missingRoutineView.attacks[0].usable);
    assert(!missingRoutineView.attacks[0].routineName.has_value());
    assert(loadedAttacks.toData().attacks[0].routineId == "mixedFullAttack");

    attacks.remove("mixed");
    assert(attacks.toView().attacks.empty());
    assert(throwsInvalidArgument([&]
    {
        attacks.remove("mixed");
    }));

    resourceManager.removeFromCollection(AttackRoutineBonusAdjustmentsResource, "lightOffHand");
    resourceManager.removeFromCollection(AttackRoutineBonusAdjustmentsResource, "twoWeaponFightingMain");
    resourceManager.removeFromCollection(AttackRoutineBonusAdjustmentsResource, "twoWeaponFightingOff");
    resourceManager.removeFromCollection(AttackRoutinesResource, "twoWeaponFighting");

    return 0;
}
