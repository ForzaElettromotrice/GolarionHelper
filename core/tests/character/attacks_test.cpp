#include "golarion/character/ability.hpp"
#include "golarion/character/action.hpp"
#include "golarion/character/attack.hpp"
#include "golarion/character/attack_routine.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/resource/modifier.hpp"
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
    AttackRoutines routines(resourceManager, actionManager, strikes);
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
            .damageComponents = {
                DamageComponent(DamageComponentDefinition{
                    .id = "base",
                    .source = "Test",
                    .role = DamageComponentRole::Base,
                    .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 8}),
                    .types = {DamageType::Slashing},
                    .typeMode = DamageTypeMode::All,
                    .criticalRule = DamageCriticalRule::Multiplied,
                    .traits = {}
                }),
                DamageComponent(DamageComponentDefinition{
                    .id = "critical",
                    .source = "Test",
                    .role = DamageComponentRole::Additional,
                    .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
                    .types = {DamageType::Untyped},
                    .typeMode = DamageTypeMode::All,
                    .criticalRule = DamageCriticalRule::CriticalOnly,
                    .traits = {}
                })
            },
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
    resourceManager.addModifier(criticalConfirmationResourceName("longsword"), Modifier(ModifierType::Bonus, "Critico focalizzato", "Bonus alla conferma", BonusType::Generic, "2"));

    attacks.create("standard", "Attacco con spada", StandardAttackRoutineId);
    attacks.assignStrike("standard", "strike", "longsword");
    AttacksView canonicalView = attacks.toView();
    assert(canonicalView.attacks[0].complete);
    assert(canonicalView.attacks[0].calculatedAttacks.size() == 1);
    assert(canonicalView.attacks[0].calculatedAttacks[0].strike.attackAbilityOptions[0].attackBonus == 14);
    attacks.remove("standard");

    attacks.create("naturalFull", "Attacco completo naturale", FullAttackRoutineId);
    canonicalView = attacks.toView();
    assert(canonicalView.attacks[0].complete);
    assert(canonicalView.attacks[0].slots[0].assignmentRequired == false);
    assert(canonicalView.attacks[0].slots[1].effectiveStrikeUsage == StrikeUsage::Default);
    assert(canonicalView.attacks[0].calculatedAttacks.size() == 3);
    assert(canonicalView.attacks[0].calculatedAttacks[0].strike.usage == StrikeUsage::Default);
    assert(canonicalView.attacks[0].calculatedAttacks[0].strike.attackAbilityOptions[0].attackBonus == 14);
    attacks.remove("naturalFull");

    attacks.create("mixedCanonical", "Attacco completo misto", FullAttackRoutineId);
    attacks.assignStrike("mixedCanonical", "weapon", "longsword");
    canonicalView = attacks.toView();
    assert(canonicalView.attacks[0].slots[1].effectiveStrikeUsage == StrikeUsage::NaturalSecondary);
    assert(canonicalView.attacks[0].calculatedAttacks.size() == 5);
    assert(canonicalView.attacks[0].calculatedAttacks[3].strike.usage == StrikeUsage::NaturalSecondary);
    attacks.remove("mixedCanonical");

    resourceManager.addToCollection(AttackRoutinesResource, AttackRoutine(AttackRoutineDefinition{
        .id = "mixedFullAttack",
        .source = "Regole base",
        .name = "Attacco completo misto",
        .actionId = "base.fullAttack",
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
                .damageAbilityRuleOverride = std::nullopt,
                .handUsage = HandUsage::OneHanded,
                .handRole = AttackHandRole::Primary
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
        .actionId = "base.fullAttack",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "first",
                .name = "Primo",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt,
                .handUsage = HandUsage::OneHanded,
                .handRole = AttackHandRole::Primary
            }),
            RoutineSlot(RoutineSlotDefinition{
                .id = "second",
                .name = "Secondo",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Natural}}),
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
        .actionId = "base.fullAttack",
        .slots = {
            RoutineSlot(RoutineSlotDefinition{
                .id = "mainHand",
                .name = "Arma primaria",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt,
                .baseAttackBonusAdjustmentExpression = "-6",
                .handUsage = HandUsage::OneHanded,
                .handRole = AttackHandRole::Primary
            }),
            RoutineSlot(RoutineSlotDefinition{
                .id = "offHand",
                .name = "Arma secondaria",
                .selector = StrikeSelector(StrikeSelectorDefinition{.requiredTags = {AttackTag::Weapon}}),
                .selectionMode = RoutineSlotSelectionMode::ChooseOne,
                .strikeUsage = StrikeUsage::Default,
                .progressions = {oneAttack()},
                .damageAbilityRuleOverride = std::nullopt,
                .baseAttackBonusAdjustmentExpression = "-10",
                .handUsage = HandUsage::OneHanded,
                .handRole = AttackHandRole::OffHand
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
    assert(view.attacks[0].actionId == "base.fullAttack");
    assert(view.attacks[0].actionName == "Attacco completo");
    assert(view.attacks[0].slots[0].assignedGrantId == "longsword");
    assert((view.attacks[0].slots[0].effectiveGrantIds == std::vector<std::string>{"longsword"}));
    assert((view.attacks[0].slots[1].effectiveGrantIds == std::vector<std::string>{"leftClaw", "bite"}));
    assert(!view.attacks[0].slots[1].candidates[1].accepted);
    assert(view.attacks[0].slots[1].candidates[1].rejectionReasons.back() == "Un canale d'uso dello strike è già occupato");
    assert(view.attacks[0].calculatedAttacks.size() == 5);
    assert(view.attacks[0].calculatedAttacks[0].strikeGrantId == "longsword");
    assert(view.attacks[0].calculatedAttacks[0].totalAttackBonusAdjustment == 0);
    assert(view.attacks[0].calculatedAttacks[0].strike.attackAbilityOptions[0].attackBonus == 14);
    assert(view.attacks[0].calculatedAttacks[0].strike.attackAbilityOptions[0].criticalConfirmationBonus == 16);
    assert(view.attacks[0].calculatedAttacks[1].strike.attackAbilityOptions[0].attackBonus == 9);
    assert(view.attacks[0].calculatedAttacks[1].strike.attackAbilityOptions[0].criticalConfirmationBonus == 11);
    assert(view.attacks[0].calculatedAttacks[2].strike.attackAbilityOptions[0].attackBonus == 4);
    assert(view.attacks[0].calculatedAttacks[2].strike.attackAbilityOptions[0].criticalConfirmationBonus == 6);
    assert(view.attacks[0].calculatedAttacks[0].normalDamage.abilityOptions[0].bonus == 3);
    assert(view.attacks[0].calculatedAttacks[0].normalDamage.abilityOptions[0].abilityContribution == 3);
    assert(view.attacks[0].calculatedAttacks[0].criticalDamage.abilityOptions[0].bonus == 6);
    assert(view.attacks[0].calculatedAttacks[0].criticalDamage.abilityOptions[0].abilityContribution == 6);
    assert(view.attacks[0].calculatedAttacks[0].normalDamage.components.size() == 1);
    assert(view.attacks[0].calculatedAttacks[0].normalDamage.components[0].component.effectiveDice.expression == "1d8");
    assert(view.attacks[0].calculatedAttacks[0].normalDamage.components[0].occurrences == 1);
    assert(view.attacks[0].calculatedAttacks[0].criticalDamage.components.size() == 2);
    assert(view.attacks[0].calculatedAttacks[0].criticalDamage.components[0].occurrences == 2);
    assert(view.attacks[0].calculatedAttacks[0].criticalDamage.components[1].component.effectiveDice.expression == "1d6");
    assert(view.attacks[0].calculatedAttacks[0].criticalDamage.components[1].occurrences == 1);
    assert(view.attacks[0].calculatedAttacks[3].strikeGrantId == "leftClaw");
    assert(view.attacks[0].calculatedAttacks[3].strike.usage == StrikeUsage::NaturalSecondary);
    assert(view.attacks[0].calculatedAttacks[3].strike.attackAbilityOptions[0].attackBonus == 9);
    assert(view.attacks[0].calculatedAttacks[3].normalDamage.abilityOptions[0].bonus == 1);
    assert(view.attacks[0].calculatedAttacks[3].criticalDamage.abilityOptions[0].bonus == 2);

    resourceManager.addToCollection(ActionInhibitionsResource, ActionInhibition(ActionInhibitionDefinition{
        .id = "staggeredFullAttack",
        .source = "Barcollante",
        .selector = ActionSelector(ActionSelectorDefinition{.actionId = "base.fullAttack"}),
        .reason = "Non può compiere azioni di round completo"
    }));
    view = attacks.toView();
    assert(view.attacks[0].complete);
    assert(!view.attacks[0].usable);
    assert(view.attacks[0].failureReasons[0] == "Barcollante: Non può compiere azioni di round completo");
    resourceManager.removeFromCollection(ActionInhibitionsResource, "staggeredFullAttack");
    assert(attacks.toView().attacks[0].usable);

    const AttacksData saved = attacks.toData();
    assert(saved.attacks.size() == 1);
    assert(saved.attacks[0].routineId == "mixedFullAttack");
    assert(saved.attacks[0].assignments.size() == 1);
    assert(saved.attacks[0].assignments[0].slotId == "weapon");
    assert(saved.attacks[0].assignments[0].strikeGrantId == "longsword");

    addStrike("offhandSword", "Spada secondaria", {AttackTag::Weapon}, std::nullopt, {"hand.left"}, WeaponWeight::OneHanded);
    attacks.create("canonicalDualWield", "Due armi canonico", TwoWeaponFightingRoutineId);
    attacks.assignStrike("canonicalDualWield", "mainHand", "longsword");
    attacks.assignStrike("canonicalDualWield", "offHand", "offhandSword");
    view = attacks.toView();
    assert(view.attacks[1].slots[0].effectiveAttackBonusAdjustment == -6);
    assert(view.attacks[1].slots[1].effectiveAttackBonusAdjustment == -10);
    assert(view.attacks[1].calculatedAttacks.size() == 5);
    assert(view.attacks[1].calculatedAttacks[0].strike.attackAbilityOptions[0].attackBonus == 8);
    assert(view.attacks[1].calculatedAttacks[1].strike.attackAbilityOptions[0].attackBonus == 3);
    assert(view.attacks[1].calculatedAttacks[2].strike.attackAbilityOptions[0].attackBonus == -2);
    assert(view.attacks[1].calculatedAttacks[3].strike.attackAbilityOptions[0].attackBonus == 4);
    assert(view.attacks[1].calculatedAttacks[3].normalDamage.abilityOptions[0].bonus == 1);
    assert(view.attacks[1].calculatedAttacks[4].strikeGrantId == "bite");
    assert(view.attacks[1].calculatedAttacks[4].strike.usage == StrikeUsage::NaturalSecondary);
    attacks.remove("canonicalDualWield");

    attacks.create("dualWield", "Due armi", "twoWeaponFighting");
    attacks.assignStrike("dualWield", "mainHand", "longsword");
    attacks.assignStrike("dualWield", "offHand", "offhandSword");
    view = attacks.toView();
    assert(view.attacks[1].slots[0].baseAttackBonusAdjustment == -6);
    assert(view.attacks[1].slots[0].effectiveAttackBonusAdjustment == -4);
    assert(view.attacks[1].slots[1].baseAttackBonusAdjustment == -10);
    assert(view.attacks[1].slots[1].effectiveAttackBonusAdjustment == -4);
    const auto offHandCandidate = std::ranges::find(view.attacks[1].slots[1].candidates, "offhandSword", &RoutineStrikeCandidateView::grantId);
    assert(offHandCandidate != view.attacks[1].slots[1].candidates.end());
    assert(offHandCandidate->effectiveDamageAbilityRule == DamageAbilityRule::HalfPositiveFullPenalty);
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
