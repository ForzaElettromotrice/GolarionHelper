#include "golarion/character/attack.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/resource_manager_view.hpp"
#include "golarion/view/attacks_view.hpp"

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
    Attacks attacks(resourceManager);
    resourceManager.addContribution(BaseAttackBonusResource, Contribution("fighter", "11"));
    resourceManager.addToCollection(AttackGrantsResource, AttackGrant(AttackGrantDefinition{
        .id = "equippedDaggerThrown",
        .source = "Pugnale equipaggiato",
        .name = "Pugnale (lancio)",
        .mode = AttackMode::Ranged,
        .tags = {AttackTag::Weapon, AttackTag::Thrown},
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
        .requirements = {Requirement("@daggerEquipped", "Il pugnale non è equipaggiato")}
    }));

    const ResourceManagerView managerView = resourceManager.toView();
    assert(std::ranges::find(managerView.collections, AttackGrantsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, DamageComponentGrantsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, CriticalAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackAbilityReplacementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, DamageAbilityReplacementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, DamageDiceAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackDistanceAdjustmentsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackRequirementsResource) != managerView.collections.end());
    assert(std::ranges::find(managerView.collections, AttackDefenseReplacementsResource) != managerView.collections.end());
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.grant.equippedDaggerThrown", "attack.all"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.grant.equippedDaggerThrown", "attack.ranged"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.grant.equippedDaggerThrown", "attack.weapon"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attack.grant.equippedDaggerThrown", "attack.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.grant.equippedDaggerThrown", "damage.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("criticalConfirmation.grant.equippedDaggerThrown", "attack.grant.equippedDaggerThrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("criticalConfirmation.grant.equippedDaggerThrown", "criticalConfirmation.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("attackRange.increment.grant.equippedDaggerThrown", "attackRange.increment.thrown"));

    resourceManager.addModifier("attack.all", Modifier(ModifierType::Bonus, "Test", "Bonus globale", BonusType::Generic, "1"));
    resourceManager.addModifier("attack.ranged", Modifier(ModifierType::Bonus, "Test", "Bonus a distanza", BonusType::Generic, "2"));
    resourceManager.addModifier("attack.thrown", Modifier(ModifierType::Bonus, "Test", "Bonus al lancio", BonusType::Generic, "3"));
    Modifier exactModifier(ModifierType::Bonus, "Test", "Bonus al pugnale", BonusType::Generic, "4");
    const std::string exactModifierId = exactModifier.id();
    resourceManager.addModifier("attack.grant.equippedDaggerThrown", std::move(exactModifier));
    assert(resourceManager.modifierTotal("attack.grant.equippedDaggerThrown") == 10);
    resourceManager.addModifier("criticalConfirmation.weapon", Modifier(ModifierType::Bonus, "Critico Focalizzato", "Bonus alla conferma", BonusType::Circumstance, "4"));
    assert(resourceManager.modifierTotal("criticalConfirmation.grant.equippedDaggerThrown") == 14);
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
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("damage.grant.equippedDaggerThrown", "damage.thrown"));
    assert(resourceManager.enhanceableResourceIsOrInheritsFrom("criticalConfirmation.grant.equippedDaggerThrown", "criticalConfirmation.thrown"));
    resourceManager.removeModifier("attack.grant.equippedDaggerThrown", exactModifierId);
    Modifier exactConfirmationModifier(ModifierType::Bonus, "Test", "Bonus specifico alla conferma", BonusType::Generic, "2");
    const std::string exactConfirmationModifierId = exactConfirmationModifier.id();
    resourceManager.addModifier("criticalConfirmation.grant.equippedDaggerThrown", std::move(exactConfirmationModifier));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeModifier("criticalConfirmation.grant.equippedDaggerThrown", exactConfirmationModifierId);

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
        .targetResourceName = "attack.grant.equippedDaggerThrown",
        .abilityType = AbilityType::Charisma
    }));
    resourceManager.addToCollection(DamageAbilityReplacementsResource, DamageAbilityReplacement(DamageAbilityReplacementDefinition{
        .id = "finesseDamage",
        .source = "Grazia tagliente",
        .targetResourceName = "damage.grant.equippedDaggerThrown",
        .abilityType = AbilityType::Dexterity
    }));
    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "largerWeapons",
        .source = "Dadi delle armi aumentati",
        .targetResourceName = "damage.weapon",
        .targetRole = DamageComponentRole::Base,
        .targetComponentId = std::nullopt,
        .type = DamageDiceAdjustmentType::ProgressionSteps,
        .expression = "1",
        .setDice = std::nullopt,
        .condition = std::nullopt
    }));
    resourceManager.addToCollection(DamageDiceAdjustmentsResource, DamageDiceAdjustment(DamageDiceAdjustmentDefinition{
        .id = "giantTarget",
        .source = "Anatema dei giganti",
        .targetResourceName = "damage.grant.equippedDaggerThrown",
        .targetRole = DamageComponentRole::Base,
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
        .targetResourceName = "attackRange.increment.grant.equippedDaggerThrown",
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
        .targetResourceName = "attack.grant.equippedDaggerThrown",
        .requirement = Requirement("@canThrowUnderwater", "Non può essere usato sott'acqua"),
        .condition = "Sott'acqua"
    }));
    resourceManager.addToCollection(AttackDefenseReplacementsResource, AttackDefenseReplacement(AttackDefenseReplacementDefinition{
        .id = "closeRangeTouch",
        .source = "Penetrazione a distanza ravvicinata",
        .targetResourceName = "attack.grant.equippedDaggerThrown",
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
        .targetResourceName = "attack.grant.equippedDaggerThrown",
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

    AttacksView attacksView = attacks.toView();
    assert(attacksView.attacks.size() == 1);
    const AttackView &attackView = attacksView.attacks[0];
    assert(attackView.id == "equippedDaggerThrown");
    assert(attackView.defenseOptions.size() == 2);
    assert(!attackView.defenseOptions[0].replacementId.has_value());
    assert(attackView.defenseOptions[0].defenseType == ArmorClassType::Normal);
    assert(attackView.defenseOptions[1].replacementId == "closeRangeTouch");
    assert(attackView.defenseOptions[1].defenseType == ArmorClassType::Touch);
    assert(attackView.defenseOptions[1].condition == "Entro il primo incremento di gittata");
    assert(attackView.attackAbilityOptions.size() == 2);
    assert(!attackView.attackAbilityOptions[0].replacementId.has_value());
    assert(attackView.attackAbilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(attackView.attackAbilityOptions[0].abilityModifier == 2);
    assert((attackView.attackAbilityOptions[0].attackBonuses == std::vector<int>{19, 14, 9}));
    assert(attackView.attackAbilityOptions[0].criticalConfirmationBonus == 23);
    assert(attackView.attackAbilityOptions[1].replacementId == "charismaticAttack");
    assert(attackView.attackAbilityOptions[1].abilityType == AbilityType::Charisma);
    assert((attackView.attackAbilityOptions[1].attackBonuses == std::vector<int>{21, 16, 11}));
    assert(attackView.attackAbilityOptions[1].criticalConfirmationBonus == 25);
    assert(attackView.criticalProfile.threatMinimum == 17);
    assert(attackView.criticalProfile.multiplier == 3);
    assert(attackView.conditionalCriticalProfiles.size() == 1);
    assert(attackView.conditionalCriticalProfiles[0].condition == "Contro il nemico prescelto");
    assert(attackView.conditionalCriticalProfiles[0].profile.threatMinimum == 15);
    assert(attackView.conditionalCriticalProfiles[0].profile.multiplier == 3);
    assert(attackView.damageAbilityOptions.size() == 2);
    assert(!attackView.damageAbilityOptions[0].replacementId.has_value());
    assert(attackView.damageAbilityOptions[0].abilityType == AbilityType::Strength);
    assert(attackView.damageAbilityOptions[0].abilityContribution == 3);
    assert(attackView.damageAbilityOptions[0].damageBonus == 3);
    assert(attackView.damageAbilityOptions[0].criticalDamageBonus == 9);
    assert(attackView.damageAbilityOptions[1].replacementId == "finesseDamage");
    assert(attackView.damageAbilityOptions[1].abilityType == AbilityType::Dexterity);
    assert(attackView.damageAbilityOptions[1].abilityContribution == 2);
    assert(attackView.damageAbilityOptions[1].damageBonus == 2);
    assert(attackView.damageAbilityOptions[1].criticalDamageBonus == 6);
    assert(attackView.damageComponents.size() == 2);
    assert(!attackView.damageComponents[0].grantId.has_value());
    assert(attackView.damageComponents[0].role == DamageComponentRole::Base);
    assert(attackView.damageComponents[0].baseDice.expression == "1d4");
    assert(attackView.damageComponents[0].effectiveDice.expression == "1d6");
    assert(attackView.damageComponents[0].conditionalDice.size() == 1);
    assert(attackView.damageComponents[0].conditionalDice[0].condition == "Contro giganti");
    assert(attackView.damageComponents[0].conditionalDice[0].dice.expression == "1d8");
    assert(attackView.damageComponents[0].diceAdjustments.size() == 2);
    assert(attackView.damageComponents[0].criticalOccurrences == 3);
    assert(attackView.damageComponents[1].grantId == "flamingWeapons");
    assert(attackView.damageComponents[1].role == DamageComponentRole::Additional);
    assert(attackView.damageComponents[1].baseDice.expression == "1d6");
    assert(attackView.damageComponents[1].effectiveDice.expression == "1d6");
    assert(attackView.damageComponents[1].diceAdjustments.empty());
    assert(attackView.damageComponents[1].criticalOccurrences == 1);
    assert(!attackView.reach.has_value());
    assert(attackView.range.has_value());
    assert(attackView.range->incrementUnits.baseValue == 2);
    assert(attackView.range->incrementUnits.effectiveValue == 8);
    assert(attackView.range->incrementUnits.conditionalValues.size() == 1);
    assert(attackView.range->incrementUnits.conditionalValues[0].condition == "Sott'acqua");
    assert(attackView.range->incrementUnits.conditionalValues[0].value == 6);
    assert(attackView.range->maximumIncrements.effectiveValue == 5);
    assert(attackView.range->penaltyPerAdditionalIncrement.effectiveValue == -1);
    assert(!attackView.usable);
    assert(attackView.failureReasons.size() == 1);
    assert(attackView.failureReasons[0] == "Non ci sono munizioni disponibili");
    assert(attackView.conditionalUsability.size() == 1);
    assert(attackView.conditionalUsability[0].condition == "Sott'acqua");
    assert(!attackView.conditionalUsability[0].usable);
    assert(attackView.conditionalUsability[0].failureReasons.size() == 2);
    assert(attackView.requirements.size() == 3);
    ammunitionAvailable = 1;
    attacksView = attacks.toView();
    assert(attacksView.attacks[0].usable);
    assert(attacksView.attacks[0].failureReasons.empty());
    assert(!attacksView.attacks[0].conditionalUsability[0].usable);
    assert(attacksView.attacks[0].conditionalUsability[0].failureReasons.size() == 1);
    daggerEquipped = 0;
    attacksView = attacks.toView();
    assert(!attacksView.attacks[0].usable);
    assert(attacksView.attacks[0].failureReasons.size() == 1);
    assert(attacksView.attacks[0].failureReasons[0] == "Il pugnale non è equipaggiato");
    daggerEquipped = 1;
    resourceManager.addToCollection(DamageComponentGrantsResource, DamageComponentGrant(DamageComponentGrantDefinition{
        .id = "flamingDagger",
        .source = "Arma fiammeggiante",
        .targetResourceName = "damage.grant.equippedDaggerThrown",
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
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageComponentGrantsResource, "flamingDagger");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "mythicCritical");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackAbilityReplacementsResource, "charismaticAttack");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageAbilityReplacementsResource, "finesseDamage");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "giantTarget");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "underwaterRangeLimit");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackRequirementsResource, "underwaterThrowing");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(AttackDefenseReplacementsResource, "closeRangeTouch");
    resourceManager.removeFromCollection(AttackGrantsResource, "equippedDaggerThrown");
    assert(throwsInvalidArgument([&]
    {
        resourceManager.modifierTotal("attack.grant.equippedDaggerThrown");
    }));
    resourceManager.removeFromCollection(DamageComponentGrantsResource, "flamingWeapons");
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "improvedCritical");
    resourceManager.removeFromCollection(CriticalAdjustmentsResource, "favoredEnemyCritical");
    resourceManager.removeFromCollection(DamageDiceAdjustmentsResource, "largerWeapons");
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "distanceWeapon");
    resourceManager.removeFromCollection(AttackRequirementsResource, "thrownAmmunition");

    resourceManager.addToCollection(AttackGrantsResource, AttackGrant(AttackGrantDefinition{
        .id = "longspear",
        .source = "Lancia lunga equipaggiata",
        .name = "Lancia lunga",
        .mode = AttackMode::Melee,
        .tags = {AttackTag::Weapon},
        .damageComponents = {},
        .damageAbilityRule = DamageAbilityRule::Full,
        .criticalThreatMinimum = 20,
        .criticalMultiplier = 3,
        .defenseType = ArmorClassType::Normal,
        .reach = AttackReachDefinition{
            .minimumUnits = 2,
            .maximumUnits = 2
        },
        .range = std::nullopt,
        .requirements = {}
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
    attacksView = attacks.toView();
    assert(attacksView.attacks.size() == 1);
    assert(attacksView.attacks[0].reach.has_value());
    assert(attacksView.attacks[0].reach->minimumUnits.effectiveValue == 2);
    assert(attacksView.attacks[0].reach->maximumUnits.effectiveValue == 6);
    assert(!attacksView.attacks[0].range.has_value());
    resourceManager.removeFromCollection(AttackGrantsResource, "longspear");
    resourceManager.removeFromCollection(AttackDistanceAdjustmentsResource, "doubledReach");

    assert(displayName(AttackMode::Melee) == "Mischia");
    assert(displayName(AttackTag::Thrown) == "Lancio");
    assert(displayName(DamageAbilityRule::PenaltyOnly) == "Solo penalità");
    assert(displayName(CriticalAdjustmentType::ThreatRangeMultiplier) == "Moltiplicatore dell'intervallo di minaccia");
    assert(displayName(CriticalAdjustmentType::ThreatMinimum) == "Soglia minima di minaccia");
    assert(displayName(CriticalAdjustmentType::MultiplierIncrease) == "Aumento del moltiplicatore del critico");
    assert(displayName(CriticalAdjustmentType::MultiplierSet) == "Moltiplicatore del critico impostato");
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
        AttackGrant(AttackGrantDefinition{
            .id = "invalidThrown",
            .source = "Test",
            .name = "Lancio in mischia",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Thrown},
            .damageComponents = {},
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2
        });
    }));
    assert(throwsInvalidArgument([]
    {
        AttackGrant(AttackGrantDefinition{
            .id = "invalidNatural",
            .source = "Test",
            .name = "Morso",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Natural, AttackTag::Primary, AttackTag::Secondary},
            .damageComponents = {},
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 20,
            .criticalMultiplier = 2
        });
    }));
    assert(throwsInvalidArgument([]
    {
        AttackGrant(AttackGrantDefinition{
            .id = "invalidCritical",
            .source = "Test",
            .name = "Critico impossibile",
            .mode = AttackMode::Melee,
            .tags = {AttackTag::Weapon},
            .damageComponents = {},
            .damageAbilityRule = DamageAbilityRule::Full,
            .criticalThreatMinimum = 0,
            .criticalMultiplier = 2
        });
    }));

    return 0;
}
