#include "golarion/character/strike.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/resource_manager_view.hpp"
#include "golarion/view/strikes_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>

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
    std::array abilities{
        AbilityScore(AbilityType::Strength, 16),
        AbilityScore(AbilityType::Dexterity, 14),
        AbilityScore(AbilityType::Constitution),
        AbilityScore(AbilityType::Intelligence),
        AbilityScore(AbilityType::Wisdom),
        AbilityScore(AbilityType::Charisma, 18)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(resourceManager);
    }
    int daggerEquipped = 1;
    int ammunitionAvailable = 0;
    int canThrowUnderwater = 0;
    resourceManager.registerTarget("daggerEquipped", [&daggerEquipped]
    {
        return daggerEquipped;
    });
    resourceManager.registerTarget("ammunitionAvailable", [&ammunitionAvailable]
    {
        return ammunitionAvailable;
    });
    resourceManager.registerTarget("canThrowUnderwater", [&canThrowUnderwater]
    {
        return canThrowUnderwater;
    });
    Strikes strikes(resourceManager);
    resourceManager.addContribution(BaseAttackBonusResource, Contribution("fighter", "11"));
    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "equippedDaggerThrown",
        .source = "Pugnale equipaggiato",
        .name = "Pugnale (lancio)",
        .mode = AttackMode::Ranged,
        .tags = {AttackTag::Weapon, AttackTag::Thrown},
        .naturalAttackClassification = std::nullopt,
        .usageChannels = {" hand.right "},
        .damageComponents = {
            DamageComponent(DamageComponentDefinition{
                .id = "weapon",
                .source = "Pugnale",
                .role = DamageComponentRole::Base,
                .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 4}),
                .types = {DamageType::Piercing},
                .typeMode = DamageTypeMode::All,
                .criticalRule = DamageCriticalRule::Multiplied,
                .traits = {}
            })
        },
        .damageAbility = AbilityType::Strength,
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 19,
        .criticalMultiplier = 2,
        .defenseType = ArmorClassType::Normal,
        .reach = std::nullopt,
        .range = AttackRangeDefinition{
            .incrementUnits = 2,
            .maximumIncrements = 5,
            .penaltyPerAdditionalIncrement = -2
        },
        .requirements = {Requirement("@daggerEquipped", "Il pugnale non è equipaggiato")},
        .weaponWeight = WeaponWeight::Light
    }));

    const ResourceManagerView managerView = resourceManager.toView();
    assert(std::ranges::find(managerView.collections, StrikeGrantsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, DamageComponentGrantsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, CriticalAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackAbilityReplacementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, DamageAbilityReplacementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, DamageDiceAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackDistanceAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackRequirementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackDefenseReplacementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, StrikeWeaponWeightAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, StrikeUsageAdjustmentsResource) != managerView.collections.end());
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.strike.equippedDaggerThrown", "attack.all"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.strike.equippedDaggerThrown", "attack.ranged"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.strike.equippedDaggerThrown", "attack.weapon"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.strike.equippedDaggerThrown", "attack.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.strike.equippedDaggerThrown", "damage.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.weapon", WeaponDamageRollResource));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.natural", WeaponDamageRollResource));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.unarmed", WeaponDamageRollResource));
    assert(!resourceManager.enhanceableResourceIsOrInheritsFrom("damage.thrown", WeaponDamageRollResource));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.strike.equippedDaggerThrown", WeaponDamageRollResource));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("criticalConfirmation.strike.equippedDaggerThrown", "attack.strike.equippedDaggerThrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("criticalConfirmation.strike.equippedDaggerThrown", "criticalConfirmation.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attackRange.increment.strike.equippedDaggerThrown", "attackRange.increment.thrown"));

    resourceManager.addModifier("attack.all", Modifier(ModifierType::Bonus, "Test", "Bonus globale", BonusType::Generic, "1"));
    resourceManager.addModifier("attack.ranged", Modifier(ModifierType::Bonus, "Test", "Bonus a distanza", BonusType::Generic, "2"));
    resourceManager.addModifier("attack.thrown", Modifier(ModifierType::Bonus, "Test", "Bonus al lancio", BonusType::Generic, "3"));
    Modifier exactModifier(ModifierType::Bonus, "Test", "Bonus al pugnale", BonusType::Generic, "4");
    const std::string exactModifierId = exactModifier.id();
    resourceManager.addModifier("attack.strike.equippedDaggerThrown", std::move(exactModifier));
    assert(resourceManager.modifierTotal("attack.strike.equippedDaggerThrown") == 10);
    resourceManager.addModifier("criticalConfirmation.weapon", Modifier(ModifierType::Bonus, "Critico Focalizzato", "Bonus alla conferma", BonusType::Circumstance, "4"));
    assert(resourceManager.modifierTotal("criticalConfirmation.strike.equippedDaggerThrown") == 14);
    resourceManager.addModifier("attackRange.increment.thrown", Modifier(ModifierType::Bonus, "Tiro Lungo", "Incremento di gittata aumentato", BonusType::Generic, "2"));
    resourceManager.addModifier("attackRange.penaltyPerAdditionalIncrement.ranged", Modifier(ModifierType::Bonus, "Tiro Lontano", "Penalità di gittata ridotta", BonusType::Generic, "1"));

    resourceManager.addToCollection(DamageComponentGrantsResource, DamageComponentGrant(DamageComponentGrantDefinition{
        .id = "flamingWeapons",
        .source = "Aura infuocata",
        .targetResourceName = "damage.weapon",
        .component = DamageComponent(DamageComponentDefinition{
            .id = "fire",
            .source = "Aura infuocata",
            .role = DamageComponentRole::Additional,
            .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
            .types = {DamageType::Fire},
            .typeMode = DamageTypeMode::All,
            .criticalRule = DamageCriticalRule::NotMultiplied,
            .traits = {}
        })
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(DamageComponentGrantsResource, DamageComponentGrant(DamageComponentGrantDefinition{
            .id = "invalidTarget",
            .source = "Test",
            .targetResourceName = "attack.all",
            .component = DamageComponent(DamageComponentDefinition{
                .id = "fire",
                .source = "Test",
                .role = DamageComponentRole::Additional,
                .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
                .types = {DamageType::Fire},
                .typeMode = DamageTypeMode::All,
                .criticalRule = DamageCriticalRule::NotMultiplied,
                .traits = {}
            })
        }));
    }));

    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.strike.equippedDaggerThrown", "damage.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("criticalConfirmation.strike.equippedDaggerThrown", "criticalConfirmation.thrown"));
    resourceManager.removeModifier("attack.strike.equippedDaggerThrown", exactModifierId);
    Modifier exactConfirmationModifier(ModifierType::Bonus, "Test", "Bonus specifico alla conferma", BonusType::Generic, "2");
    const std::string exactConfirmationModifierId = exactConfirmationModifier.id();
    resourceManager.addModifier("criticalConfirmation.strike.equippedDaggerThrown", std::move(exactConfirmationModifier));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeModifier("criticalConfirmation.strike.equippedDaggerThrown", exactConfirmationModifierId);

    resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
        .id = "improvedCritical",
        .source = "Critico Migliorato",
        .targetResourceName = "attack.weapon",
        .type = CriticalAdjustmentType::ThreatRangeMultiplier,
        .expression = "2",
        .maximumMultiplier = std::nullopt,
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(AttackAbilityReplacementsResource, AttackAbilityReplacement(AttackAbilityReplacementDefinition{
        .id = "charismaticAttack",
        .source = "Attacco carismatico",
        .targetResourceName = "attack.strike.equippedDaggerThrown",
        .abilityType = AbilityType::Charisma
    }));
    resourceManager.addToCollection(DamageAbilityReplacementsResource, DamageAbilityReplacement(DamageAbilityReplacementDefinition{
        .id = "finesseDamage",
        .source = "Grazia tagliente",
        .targetResourceName = "damage.strike.equippedDaggerThrown",
        .abilityType = AbilityType::Dexterity
    }));
    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "largerWeapons",
        .source = "Dadi delle armi aumentati",
        .targetResourceName = "damage.weapon",
        .targetRole = DamageComponentRole::Base,
        .targetOrigin = DamageComponentOriginFilter::Any,
        .targetComponentGrantId = std::nullopt,
        .targetComponentId = std::nullopt,
        .type = DamageDiceAdjustmentType::ProgressionSteps,
        .expression = "1",
        .setDice = std::nullopt,
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "giantTarget",
        .source = "Anatema dei giganti",
        .targetResourceName = "damage.strike.equippedDaggerThrown",
        .targetRole = DamageComponentRole::Base,
        .targetOrigin = DamageComponentOriginFilter::Intrinsic,
        .targetComponentGrantId = std::nullopt,
        .targetComponentId = "weapon",
        .type = DamageDiceAdjustmentType::ProgressionSteps,
        .expression = "1",
        .setDice = std::nullopt,
        .condition = "Contro giganti"
    }));
    resourceManager.addToCollection(AttackDistanceAdjustmentsResource, AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition{
        .id = "distanceWeapon",
        .source = "Distanza",
        .targetResourceName = "attackRange.increment.thrown",
        .type = AttackDistanceAdjustmentType::Multiplier,
        .expression = "2",
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(AttackDistanceAdjustmentsResource, AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition{
        .id = "underwaterRangeLimit",
        .source = "Combattimento subacqueo",
        .targetResourceName = "attackRange.increment.strike.equippedDaggerThrown",
        .type = AttackDistanceAdjustmentType::Maximum,
        .expression = "6",
        .condition = "Sott'acqua"
    }));
    resourceManager.addToCollection(AttackRequirementsResource, AttackRequirement(AttackRequirementDefinition{
        .id = "thrownAmmunition",
        .source = "Munizioni",
        .targetResourceName = "attack.thrown",
        .requirement = Requirement("@ammunitionAvailable", "Non ci sono munizioni disponibili"),
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(AttackRequirementsResource, AttackRequirement(AttackRequirementDefinition{
        .id = "underwaterThrowing",
        .source = "Combattimento subacqueo",
        .targetResourceName = "attack.strike.equippedDaggerThrown",
        .requirement = Requirement("@canThrowUnderwater", "Non può essere usato sott'acqua"),
        .condition = "Sott'acqua"
    }));
    resourceManager.addToCollection(AttackDefenseReplacementsResource, AttackDefenseReplacement(AttackDefenseReplacementDefinition{
        .id = "closeRangeTouch",
        .source = "Penetrazione a distanza ravvicinata",
        .targetResourceName = "attack.strike.equippedDaggerThrown",
        .defenseType = ArmorClassType::Touch,
        .condition = "Entro il primo incremento di gittata"
    }));
    resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
        .id = "favoredEnemyCritical",
        .source = "Nemico prescelto",
        .targetResourceName = "attack.thrown",
        .type = CriticalAdjustmentType::ThreatMinimum,
        .expression = "15",
        .maximumMultiplier = std::nullopt,
        .condition = "Contro il nemico prescelto"
    }));
    resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
        .id = "mythicCritical",
        .source = "Critico Migliorato Mitico",
        .targetResourceName = "attack.strike.equippedDaggerThrown",
        .type = CriticalAdjustmentType::MultiplierIncrease,
        .expression = "1",
        .maximumMultiplier = 6,
        .condition = std::nullopt
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
            .id = "invalidCriticalTarget",
            .source = "Test",
            .targetResourceName = "damage.all",
            .type = CriticalAdjustmentType::ThreatMinimum,
            .expression = "18",
            .maximumMultiplier = std::nullopt,
            .condition = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(AttackRequirementsResource, AttackRequirement(AttackRequirementDefinition{
            .id = "invalidRequirementTarget",
            .source = "Test",
            .targetResourceName = "damage.all",
            .requirement = Requirement("1", "Non disponibile"),
            .condition = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(AttackDefenseReplacementsResource, AttackDefenseReplacement(AttackDefenseReplacementDefinition{
            .id = "invalidDefenseTarget",
            .source = "Test",
            .targetResourceName = "damage.all",
            .defenseType = ArmorClassType::Touch,
            .condition = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
            .id = "invalidDiceTarget",
            .source = "Test",
            .targetResourceName = "attack.all",
            .targetRole = DamageComponentRole::Base,
            .targetOrigin = DamageComponentOriginFilter::Any,
            .targetComponentGrantId = std::nullopt,
            .targetComponentId = std::nullopt,
            .type = DamageDiceAdjustmentType::ProgressionSteps,
            .expression = "1",
            .setDice = std::nullopt,
            .condition = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(AttackDistanceAdjustmentsResource, AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition{
            .id = "invalidDistanceTarget",
            .source = "Test",
            .targetResourceName = "attack.all",
            .type = AttackDistanceAdjustmentType::Multiplier,
            .expression = "2",
            .condition = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(AttackAbilityReplacementsResource, AttackAbilityReplacement(AttackAbilityReplacementDefinition{
            .id = "invalidAttackAbility",
            .source = "Test",
            .targetResourceName = "damage.all",
            .abilityType = AbilityType::Wisdom
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(DamageAbilityReplacementsResource, DamageAbilityReplacement(DamageAbilityReplacementDefinition{
            .id = "invalidDamageAbility",
            .source = "Test",
            .targetResourceName = "attack.all",
            .abilityType = AbilityType::Wisdom
        }));
    }));

    StrikesView strikesView = strikes.toView();
    assert(strikesView.strikes.size() == 1);
    const StrikeView &strikeView = strikesView.strikes[0];
    assert(strikeView.grantId == "equippedDaggerThrown");
    assert(!strikeView.naturalAttackClassification.has_value());
    assert((strikeView.usageChannels == std::vector<std::string>{"hand.right"}));
    assert(strikeView.defenseOptions.size() == 2);
    assert(!strikeView.defenseOptions[0].replacementId.has_value());
    assert(strikeView.defenseOptions[0].defenseType == ArmorClassType::Normal);
    assert(strikeView.defenseOptions[1].replacementId == "closeRangeTouch");
    assert(strikeView.defenseOptions[1].defenseType == ArmorClassType::Touch);
    assert(strikeView.defenseOptions[1].condition == "Entro il primo incremento di gittata");
    assert(strikeView.attackAbilityOptions.size() == 2);
    assert(!strikeView.attackAbilityOptions[0].replacementId.has_value());
    assert(strikeView.attackAbilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(strikeView.attackAbilityOptions[0].abilityModifier == 2);
    assert(strikeView.attackAbilityOptions[0].attackBonus == 19);
    assert(strikeView.attackAbilityOptions[0].criticalConfirmationBonus == 23);
    assert(strikeView.attackAbilityOptions[1].replacementId == "charismaticAttack");
    assert(strikeView.attackAbilityOptions[1].abilityType == AbilityType::Charisma);
    assert(strikeView.attackAbilityOptions[1].attackBonus == 21);
    assert(strikeView.attackAbilityOptions[1].criticalConfirmationBonus == 25);
    assert(strikeView.criticalProfile.threatMinimum == 17);
    assert(strikeView.criticalProfile.multiplier == 3);
    assert(strikeView.conditionalCriticalProfiles.size() == 1);
    assert(strikeView.conditionalCriticalProfiles[0].condition == "Contro il nemico prescelto");
    assert(strikeView.conditionalCriticalProfiles[0].profile.threatMinimum == 15);
    assert(strikeView.conditionalCriticalProfiles[0].profile.multiplier == 3);
    assert(strikeView.damageAbilityOptions.size() == 2);
    assert(!strikeView.damageAbilityOptions[0].replacementId.has_value());
    assert(strikeView.damageAbilityOptions[0].abilityType == AbilityType::Strength);
    assert(strikeView.damageAbility == AbilityType::Strength);
    assert(strikeView.damageAbilityOptions[0].abilityContribution == 3);
    assert(strikeView.damageAbilityOptions[0].damageBonus == 3);
    assert(strikeView.damageAbilityOptions[0].criticalDamageBonus == 9);
    assert(strikeView.damageAbilityOptions[1].replacementId == "finesseDamage");
    assert(strikeView.damageAbilityOptions[1].abilityType == AbilityType::Dexterity);
    assert(strikeView.damageAbilityOptions[1].abilityContribution == 2);
    assert(strikeView.damageAbilityOptions[1].damageBonus == 2);
    assert(strikeView.damageAbilityOptions[1].criticalDamageBonus == 6);
    assert(strikeView.damageComponents.size() == 2);
    assert(!strikeView.damageComponents[0].grantId.has_value());
    assert(strikeView.damageComponents[0].role == DamageComponentRole::Base);
    assert(strikeView.damageComponents[0].baseDice.expression == "1d4");
    assert(strikeView.damageComponents[0].effectiveDice.expression == "1d6");
    assert(strikeView.damageComponents[0].conditionalDice.size() == 1);
    assert(strikeView.damageComponents[0].conditionalDice[0].condition == "Contro giganti");
    assert(strikeView.damageComponents[0].conditionalDice[0].dice.expression == "1d8");
    assert(strikeView.damageComponents[0].diceAdjustments.size() == 2);
    assert(strikeView.damageComponents[0].criticalOccurrences == 3);
    assert(strikeView.damageComponents[1].grantId == "flamingWeapons");
    assert(strikeView.damageComponents[1].role == DamageComponentRole::Additional);
    assert(strikeView.damageComponents[1].baseDice.expression == "1d6");
    assert(strikeView.damageComponents[1].effectiveDice.expression == "1d6");
    assert(strikeView.damageComponents[1].diceAdjustments.empty());
    assert(strikeView.damageComponents[1].criticalOccurrences == 1);
    assert(!strikeView.reach.has_value());
    assert(strikeView.range.has_value());
    assert(strikeView.range->incrementUnits.baseValue == 2);
    assert(strikeView.range->incrementUnits.effectiveValue == 8);
    assert(strikeView.range->incrementUnits.conditionalValues.size() == 1);
    assert(strikeView.range->incrementUnits.conditionalValues[0].condition == "Sott'acqua");
    assert(strikeView.range->incrementUnits.conditionalValues[0].value == 6);
    assert(strikeView.range->maximumIncrements.effectiveValue == 5);
    assert(strikeView.range->penaltyPerAdditionalIncrement.effectiveValue == -1);
    assert(!strikeView.usable);
    assert(strikeView.failureReasons.size() == 1);
    assert(strikeView.failureReasons[0] == "Non ci sono munizioni disponibili");
    assert(strikeView.conditionalUsability.size() == 1);
    assert(strikeView.conditionalUsability[0].condition == "Sott'acqua");
    assert(!strikeView.conditionalUsability[0].usable);
    assert(strikeView.conditionalUsability[0].failureReasons.size() == 2);
    assert(strikeView.requirements.size() == 3);
    resourceManager.addModifier("attack.thrown", Modifier(ModifierType::Bonus, "Nemico prescelto", "Bonus situazionale", BonusType::Generic, "2", "Contro giganti"));
    resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
        .id = "giantCriticalMultiplier",
        .source = "Anatema dei giganti",
        .targetResourceName = "attack.thrown",
        .type = CriticalAdjustmentType::MultiplierIncrease,
        .expression = "1",
        .maximumMultiplier = std::nullopt,
        .condition = "Contro giganti"
    }));
    const StrikesView combinedConditionsView = strikes.toView(StrikeCalculationContext{
        .activeConditions = {"Contro giganti", "Contro il nemico prescelto"}
    });
    const StrikeView &combinedStrike = combinedConditionsView.strikes[0];
    assert(combinedStrike.attackAbilityOptions[0].attackBonus == 21);
    assert(combinedStrike.criticalProfile.threatMinimum == 15);
    assert(combinedStrike.criticalProfile.multiplier == 4);
    assert(combinedStrike.damageComponents[0].effectiveDice.expression == "1d8");
    assert(combinedStrike.damageComponents[0].criticalOccurrences == 4);
    assert(combinedStrike.damageAbilityOptions[0].criticalDamageBonus == 12);
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "giantCriticalMultiplier");
    ammunitionAvailable = 1;
    strikesView = strikes.toView();
    assert(strikesView.strikes[0].usable);
    assert(strikesView.strikes[0].failureReasons.empty());
    assert(!strikesView.strikes[0].conditionalUsability[0].usable);
    assert(strikesView.strikes[0].conditionalUsability[0].failureReasons.size() == 1);
    const StrikesView underwaterView = strikes.toView(StrikeCalculationContext{.activeConditions = {"Sott'acqua"}});
    assert(!underwaterView.strikes[0].usable);
    assert(underwaterView.strikes[0].failureReasons.size() == 1);
    assert(underwaterView.strikes[0].failureReasons[0] == "Non può essere usato sott'acqua");
    daggerEquipped = 0;
    strikesView = strikes.toView();
    assert(!strikesView.strikes[0].usable);
    assert(strikesView.strikes[0].failureReasons.size() == 1);
    assert(strikesView.strikes[0].failureReasons[0] == "Il pugnale non è equipaggiato");
    daggerEquipped = 1;
    resourceManager.addToCollection(DamageComponentGrantsResource, DamageComponentGrant(DamageComponentGrantDefinition{
        .id = "flamingDagger",
        .source = "Arma fiammeggiante",
        .targetResourceName = "damage.strike.equippedDaggerThrown",
        .component = DamageComponent(DamageComponentDefinition{
            .id = "fire",
            .source = "Arma fiammeggiante",
            .role = DamageComponentRole::Additional,
            .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
            .types = {DamageType::Fire},
            .typeMode = DamageTypeMode::All,
            .criticalRule = DamageCriticalRule::NotMultiplied,
            .traits = {}
        })
    }));
    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "specificFlamingGrant",
        .source = "Fiamme intensificate",
        .targetResourceName = "damage.weapon",
        .targetRole = DamageComponentRole::Additional,
        .targetOrigin = DamageComponentOriginFilter::ExternalGrant,
        .targetComponentGrantId = "flamingWeapons",
        .targetComponentId = "fire",
        .type = DamageDiceAdjustmentType::ProgressionSteps,
        .expression = "1",
        .setDice = std::nullopt,
        .condition = std::nullopt
    }));
    strikesView = strikes.toView();
    const auto globalFire = std::ranges::find(strikesView.strikes[0].damageComponents, std::optional<std::string>("flamingWeapons"), &DamageComponentView::grantId);
    const auto exactFire = std::ranges::find(strikesView.strikes[0].damageComponents, std::optional<std::string>("flamingDagger"), &DamageComponentView::grantId);
    assert(globalFire != strikesView.strikes[0].damageComponents.end());
    assert(exactFire != strikesView.strikes[0].damageComponents.end());
    assert(globalFire->effectiveDice.expression == "1d8");
    assert(exactFire->effectiveDice.expression == "1d6");
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "specificFlamingGrant");

    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "firstConflictingReplacement",
        .source = "Prima sostituzione",
        .targetResourceName = "damage.strike.equippedDaggerThrown",
        .targetRole = DamageComponentRole::Base,
        .targetOrigin = DamageComponentOriginFilter::Intrinsic,
        .targetComponentGrantId = std::nullopt,
        .targetComponentId = "weapon",
        .type = DamageDiceAdjustmentType::Set,
        .expression = std::nullopt,
        .setDice = DamageDice(DamageDiceDefinition{.diceCount = 2, .dieSize = 6}),
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "secondConflictingReplacement",
        .source = "Seconda sostituzione",
        .targetResourceName = "damage.strike.equippedDaggerThrown",
        .targetRole = DamageComponentRole::Base,
        .targetOrigin = DamageComponentOriginFilter::Intrinsic,
        .targetComponentGrantId = std::nullopt,
        .targetComponentId = "weapon",
        .type = DamageDiceAdjustmentType::Set,
        .expression = std::nullopt,
        .setDice = DamageDice(DamageDiceDefinition{.diceCount = 3, .dieSize = 6}),
        .condition = std::nullopt
    }));
    assert(throwsInvalidArgument([&]
    {
        strikes.toView();
    }));
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "firstConflictingReplacement");
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "secondConflictingReplacement");

    resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
        .id = "firstCriticalReplacement",
        .source = "Prima sostituzione del critico",
        .targetResourceName = "attack.strike.equippedDaggerThrown",
        .type = CriticalAdjustmentType::MultiplierSet,
        .expression = "2",
        .maximumMultiplier = std::nullopt,
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(CriticalAdjustmentsResource, CriticalAdjustment(CriticalAdjustmentDefinition{
        .id = "secondCriticalReplacement",
        .source = "Seconda sostituzione del critico",
        .targetResourceName = "attack.strike.equippedDaggerThrown",
        .type = CriticalAdjustmentType::MultiplierSet,
        .expression = "4",
        .maximumMultiplier = std::nullopt,
        .condition = std::nullopt
    }));
    assert(throwsInvalidArgument([&]
    {
        strikes.toView();
    }));
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "firstCriticalReplacement");
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "secondCriticalReplacement");

    resourceManager.addToCollection(AttackDistanceAdjustmentsResource, AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition{
        .id = "invalidConditionalRange",
        .source = "Gittata impossibile",
        .targetResourceName = "attackRange.increment.strike.equippedDaggerThrown",
        .type = AttackDistanceAdjustmentType::Maximum,
        .expression = "0",
        .condition = "In uno spazio ristretto"
    }));
    assert(throwsInvalidArgument([&]
    {
        strikes.toView();
    }));
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "invalidConditionalRange");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageComponentGrantsResource, "flamingDagger");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "mythicCritical");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackAbilityReplacementsResource, "charismaticAttack");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageAbilityReplacementsResource, "finesseDamage");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "giantTarget");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "underwaterRangeLimit");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackRequirementsResource, "underwaterThrowing");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackDefenseReplacementsResource, "closeRangeTouch");
    resourceManager.removeFromCollection(StrikeGrantsResource, "equippedDaggerThrown");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.modifierTotal("attack.strike.equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageComponentGrantsResource, "flamingWeapons");
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "improvedCritical");
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "favoredEnemyCritical");
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "largerWeapons");
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "distanceWeapon");
    resourceManager.removeFromCollection(AttackRequirementsResource, "thrownAmmunition");

    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "claws",
        .source = "Forma bestiale",
        .name = "Artigli",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Natural},
        .naturalAttackClassification = NaturalAttackClassification::Primary,
        .damageComponents = {},
        .damageAbility = AbilityType::Dexterity,
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 20,
        .criticalMultiplier = 2,
        .defenseType = ArmorClassType::Normal,
        .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
        .range = std::nullopt,
        .requirements = {}
    }));
    strikesView = strikes.toView();
    assert(strikesView.strikes.size() == 1);
    assert(strikesView.strikes[0].usage == StrikeUsage::Default);
    assert(strikesView.strikes[0].attackAbilityOptions[0].attackBonus == 15);
    assert(strikesView.strikes[0].naturalAttackClassification == NaturalAttackClassification::Primary);
    assert(strikesView.strikes[0].damageAbilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(strikesView.strikes[0].damageAbilityOptions[0].abilityContribution == 2);

    strikesView = strikes.toView(StrikeCalculationContext{
        .activeConditions = {},
        .usageOverrides = {
            StrikeUsageOverride{.grantId = "claws", .usage = StrikeUsage::NaturalSecondary}
        }
    });
    assert(strikesView.strikes[0].usage == StrikeUsage::NaturalSecondary);
    assert(strikesView.strikes[0].naturalAttackClassification == NaturalAttackClassification::Secondary);
    assert(strikesView.strikes[0].attackAbilityOptions[0].attackBonus == 10);
    assert(strikesView.strikes[0].damageAbilityRule == DamageAbilityRule::HalfPositiveFullPenalty);
    assert(strikesView.strikes[0].damageAbilityOptions[0].abilityContribution == 1);

    strikesView = strikes.toView(StrikeCalculationContext{
        .activeConditions = {},
        .usageOverrides = {
            StrikeUsageOverride{.grantId = "claws", .usage = StrikeUsage::SingleNatural}
        }
    });
    assert(strikesView.strikes[0].usage == StrikeUsage::SingleNatural);
    assert(strikesView.strikes[0].naturalAttackClassification == NaturalAttackClassification::Primary);
    assert(strikesView.strikes[0].attackAbilityOptions[0].attackBonus == 15);
    assert(strikesView.strikes[0].damageAbilityRule == DamageAbilityRule::OneAndHalfPositiveFullPenalty);
    assert(strikesView.strikes[0].damageAbilityOptions[0].abilityContribution == 3);
    resourceManager.removeFromCollection(StrikeGrantsResource, "claws");

    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "bite",
        .source = "Forma bestiale",
        .name = "Morso",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Natural},
        .naturalAttackClassification = NaturalAttackClassification::Secondary,
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
    resourceManager.addToCollection(StrikeUsageAdjustmentsResource, StrikeUsageAdjustment(StrikeUsageAdjustmentDefinition{
        .id = "multiattack",
        .source = "Multiattacco",
        .targetResourceName = "attack.natural",
        .usage = StrikeUsage::NaturalSecondary,
        .attackPenaltyExpression = "-2",
        .damageAbilityRule = std::nullopt
    }));
    strikesView = strikes.toView();
    assert(strikesView.strikes[0].attackAbilityOptions[0].attackBonus == 13);
    assert(strikesView.strikes[0].naturalAttackClassification == NaturalAttackClassification::Secondary);
    assert(strikesView.strikes[0].usageAttackPenalty == -2);
    assert(strikesView.strikes[0].usageAdjustments.size() == 1);
    resourceManager.removeFromCollection(StrikeUsageAdjustmentsResource, "multiattack");
    resourceManager.removeFromCollection(StrikeGrantsResource, "bite");

    resourceManager.addToCollection(StrikeGrantsResource, StrikeGrant(StrikeGrantDefinition{
        .id = "longspear",
        .source = "Lancia lunga equipaggiata",
        .name = "Lancia lunga",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Weapon},
        .naturalAttackClassification = std::nullopt,
        .damageComponents = {},
        .damageAbility = AbilityType::Strength,
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 20,
        .criticalMultiplier = 3,
        .defenseType = ArmorClassType::Normal,
        .reach = AttackReachDefinition{
            .minimumUnits = 2,
            .maximumUnits = 2
        },
        .range = std::nullopt,
        .requirements = {},
        .weaponWeight = WeaponWeight::TwoHanded
    }));
    resourceManager.addToCollection(StrikeWeaponWeightAdjustmentsResource, StrikeWeaponWeightAdjustment(StrikeWeaponWeightAdjustmentDefinition{
        .id = "oversizedTwoWeaponTraining",
        .source = "Addestramento con due armi",
        .targetResourceName = "attack.strike.longspear",
        .purpose = WeaponWeightPurpose::TwoWeaponFighting,
        .weaponWeight = WeaponWeight::Light
    }));
    resourceManager.addModifier("attackReach.maximum.weapon", Modifier(ModifierType::Bonus, "Affondo", "Portata aumentata", BonusType::Generic, "1"));
    resourceManager.addToCollection(AttackDistanceAdjustmentsResource, AttackDistanceAdjustment(AttackDistanceAdjustmentDefinition{
        .id = "doubledReach",
        .source = "Portata raddoppiata",
        .targetResourceName = "attackReach.maximum.melee",
        .type = AttackDistanceAdjustmentType::Multiplier,
        .expression = "2",
        .condition = std::nullopt
    }));
    strikesView = strikes.toView();
    assert(strikesView.strikes.size() == 1);
    assert(strikesView.strikes[0].reach.has_value());
    assert(strikesView.strikes[0].reach->minimumUnits.effectiveValue == 2);
    assert(strikesView.strikes[0].reach->maximumUnits.effectiveValue == 6);
    assert(!strikesView.strikes[0].range.has_value());
    assert(strikesView.strikes[0].baseWeaponWeight == WeaponWeight::TwoHanded);
    assert(strikesView.strikes[0].effectiveWeaponWeight == WeaponWeight::TwoHanded);
    assert(strikesView.strikes[0].twoWeaponFightingWeaponWeight == WeaponWeight::Light);
    assert(strikesView.strikes[0].weaponWeightAdjustments.size() == 1);
    assert(throwsInvalidArgument([&]
    {
        strikes.toView(StrikeCalculationContext{
            .activeConditions = {},
            .usageOverrides = {
                StrikeUsageOverride{.grantId = "longspear", .usage = StrikeUsage::NaturalSecondary}
            }
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        strikes.toView(StrikeCalculationContext{
            .activeConditions = {},
            .usageOverrides = {
                StrikeUsageOverride{.grantId = "missing", .usage = StrikeUsage::Default}
            }
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        strikes.toView(StrikeCalculationContext{
            .activeConditions = {},
            .usageOverrides = {
                StrikeUsageOverride{.grantId = "longspear", .usage = StrikeUsage::Default},
                StrikeUsageOverride{.grantId = " longspear ", .usage = StrikeUsage::Default}
            }
        });
    }));
    resourceManager.removeFromCollection(StrikeWeaponWeightAdjustmentsResource, "oversizedTwoWeaponTraining");
    resourceManager.removeFromCollection(StrikeGrantsResource, "longspear");
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "doubledReach");

    assert(displayName(AttackMode::Melee) == "Mischia");
    assert(displayName(AttackTag::Thrown) == "Lancio");
    assert(displayName(NaturalAttackClassification::Primary) == "Primario");
    assert(displayName(StrikeUsage::NaturalSecondary) == "Attacco naturale secondario");
    assert(displayName(WeaponWeight::Light) == "Leggera");
    assert(displayName(WeaponWeightPurpose::TwoWeaponFighting) == "Combattere con due armi");
    assert(displayName(DamageAbilityRule::PenaltyOnly) == "Solo penalità");
    assert(displayName(CriticalAdjustmentType::ThreatRangeMultiplier) == "Moltiplicatore dell'intervallo di minaccia");
    assert(displayName(CriticalAdjustmentType::ThreatMinimum) == "Soglia minima di minaccia");
    assert(displayName(CriticalAdjustmentType::MultiplierIncrease) == "Aumento del moltiplicatore del critico");
    assert(displayName(CriticalAdjustmentType::MultiplierSet) == "Moltiplicatore del critico impostato");
    assert(displayName(DamageComponentOriginFilter::Intrinsic) == "Componente intrinseco");
    assert(displayName(AttackDistanceProperty::MaximumReach) == "Portata massima");
    assert(displayName(AttackDistanceAdjustmentType::Multiplier) == "Moltiplicatore");
    assert(defaultAttackAbility(AttackMode::Melee) == AbilityType::Strength);
    assert(defaultAttackAbility(AttackMode::Ranged) == AbilityType::Dexterity);

    assert(throwsInvalidArgument([]
    {
        CriticalAdjustment(CriticalAdjustmentDefinition{
            .id = "invalidMaximum",
            .source = "Test",
            .targetResourceName = "attack.all",
            .type = CriticalAdjustmentType::ThreatRangeMultiplier,
            .expression = "2",
            .maximumMultiplier = 4,
            .condition = std::nullopt
        });
    }));
    assert(throwsInvalidArgument([]
    {
        StrikeGrant(StrikeGrantDefinition{
            .id = "invalidThrown",
            .source = "Test",
            .name = "Lancio in mischia",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Thrown},
            .naturalAttackClassification = std::nullopt,
            .damageComponents = {},
            .damageAbility = AbilityType::Strength,
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2
        });
    }));
    assert(throwsInvalidArgument([]
    {
        StrikeGrant(StrikeGrantDefinition{
            .id = "invalidNatural",
            .source = "Test",
            .name = "Morso senza classificazione",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Natural},
            .naturalAttackClassification = std::nullopt,
            .damageComponents = {},
            .damageAbility = AbilityType::Strength,
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2
        });
    }));
    assert(throwsInvalidArgument([]
    {
        StrikeGrant(StrikeGrantDefinition{
            .id = "invalidCritical",
            .source = "Test",
            .name = "Critico impossibile",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Weapon},
            .naturalAttackClassification = std::nullopt,
            .damageComponents = {},
            .damageAbility = AbilityType::Strength,
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 0,
            .criticalMultiplier = 2
        });
    }));
    assert(throwsInvalidArgument([]
    {
        StrikeGrant(StrikeGrantDefinition{
            .id = "invalidNaturalClassification",
            .source = "Test",
            .name = "Arma classificata come naturale",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Weapon},
            .naturalAttackClassification = NaturalAttackClassification::Primary,
            .damageComponents = {},
            .damageAbility = AbilityType::Strength,
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2,
            .defenseType = ArmorClassType::Normal,
            .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
            .range = std::nullopt,
            .requirements = {}
        });
    }));
    assert(throwsInvalidArgument([]
    {
        StrikeGrant(StrikeGrantDefinition{
            .id = "duplicateUsageChannel",
            .source = "Test",
            .name = "Canale duplicato",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Weapon},
            .naturalAttackClassification = std::nullopt,
            .usageChannels = {"hand.right", " hand.right "},
            .damageComponents = {},
            .damageAbility = AbilityType::Strength,
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2,
            .defenseType = ArmorClassType::Normal,
            .reach = AttackReachDefinition{.minimumUnits = 1, .maximumUnits = 1},
            .range = std::nullopt,
            .requirements = {}
        });
    }));

    return 0;
}
