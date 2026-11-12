#include "golarion/character/armor_class.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/requirement.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/armor_class_view.hpp"

#include <cassert>
#include <optional>
#include <stdexcept>
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

    int armorClassValue(golarion::ArmorClass &armorClass, golarion::ArmorClassType type, const std::optional<std::string_view> replacementId = std::nullopt)
    {
        const golarion::ArmorClassView view = armorClass.toView();
        for (const golarion::ArmorClassAbilityOptionView &option : view.abilityOptions)
        {
            const bool matches = replacementId.has_value()
                ? option.replacementId.has_value() && *option.replacementId == *replacementId
                : !option.replacementId.has_value();
            if (!matches)
            {
                continue;
            }
            for (const golarion::ArmorClassValueView &value : option.values)
            {
                if (value.type == type)
                {
                    return value.totalValue;
                }
            }
        }
        throw std::invalid_argument("armor class option or value is not available");
    }
}

int main()
{
    using namespace golarion;

    int dexterityModifier = 4;
    int charismaModifier = 5;
    int wisdomModifier = 3;
    int armorWorn = 1;

    ResourceManager manager;
    manager.registerTarget("dexMod", [&dexterityModifier]
    {
        return dexterityModifier;
    });
    manager.registerTarget("chaMod", [&charismaModifier]
    {
        return charismaModifier;
    });
    manager.registerTarget("wisMod", [&wisdomModifier]
    {
        return wisdomModifier;
    });
    manager.registerTarget("armor.worn", [&armorWorn]
    {
        return armorWorn;
    });

    ArmorClass armorClass(manager);
    manager.addModifier(ArmorClassAllResource, Modifier({.type = ModifierType::Bonus, .source = "Anello", .description = "Bonus di deviazione", .bonusType = BonusType::Deflection, .expression = "1"}));
    manager.addModifier(ArmorClassReflexiveResource, Modifier({.type = ModifierType::Bonus, .source = "Schivare", .description = "Bonus di schivare", .bonusType = BonusType::Dodge, .expression = "1"}));
    manager.addModifier(ArmorClassSolidResource, Modifier({.type = ModifierType::Bonus, .source = "Armatura", .description = "Bonus di armatura", .bonusType = BonusType::Armor, .expression = "6"}));
    manager.addModifier(ArmorClassSolidResource, Modifier({.type = ModifierType::Bonus, .source = "Scudo", .description = "Bonus di scudo", .bonusType = BonusType::Shield, .expression = "2"}));
    manager.addModifier(resourceName(ArmorClassType::Normal), Modifier({.type = ModifierType::Bonus, .source = "Difesa", .description = "Bonus solo alla CA normale", .bonusType = BonusType::Circumstance, .expression = "2"}));
    manager.addModifier(resourceName(ArmorClassType::Touch), Modifier({.type = ModifierType::Bonus, .source = "Campo", .description = "Bonus solo a contatto", .bonusType = BonusType::Insight, .expression = "3"}));
    manager.addModifier(resourceName(ArmorClassType::FlatFooted), Modifier({.type = ModifierType::Bonus, .source = "Istinto", .description = "Bonus solo da impreparato", .bonusType = BonusType::Luck, .expression = "4"}));

    assert(displayName(ArmorClassType::Normal) == "Classe Armatura");
    assert(displayName(ArmorClassType::Touch) == "Contatto");
    assert(displayName(ArmorClassType::FlatFooted) == "Impreparato");
    assert(armorClassValue(armorClass, ArmorClassType::Normal) == 26);
    assert(armorClassValue(armorClass, ArmorClassType::Touch) == 19);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted) == 23);

    manager.addToCollection(MaximumDexterityLimitsResource, MaximumDexterityLimit(MaximumDexterityLimitDefinition{
        .id = "wornArmor",
        .source = "Armatura indossata",
        .type = MaximumDexterityLimitType::Armor,
        .expression = "2"
    }));
    assert(armorClass.maximumDexterityBonus() == 2);
    assert(armorClassValue(armorClass, ArmorClassType::Normal) == 24);
    assert(armorClassValue(armorClass, ArmorClassType::Touch) == 17);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted) == 23);

    manager.addModifier(maximumDexterityResourceName(MaximumDexterityLimitType::Armor), Modifier({.type = ModifierType::Bonus, .source = "Addestramento nelle armature", .description = "Aumento del bonus massimo di Destrezza", .bonusType = BonusType::Generic, .expression = "1"}));
    assert(armorClass.maximumDexterityBonus() == 3);
    assert(armorClassValue(armorClass, ArmorClassType::Normal) == 25);
    assert(armorClassValue(armorClass, ArmorClassType::Touch) == 18);

    manager.addToCollection(MaximumDexterityLimitsResource, MaximumDexterityLimit(MaximumDexterityLimitDefinition{
        .id = "carriedLoad",
        .source = "Carico trasportato",
        .type = MaximumDexterityLimitType::Load,
        .expression = "2"
    }));
    assert(armorClass.maximumDexterityBonus() == 2);
    ArmorClassView limitedView = armorClass.toView();
    assert(limitedView.maximumDexterityBonus == 2);
    assert(limitedView.maximumDexterityLimits.size() == 2);
    assert(limitedView.maximumDexterityLimits[0].id == "carriedLoad");
    assert(limitedView.maximumDexterityLimits[0].effectiveValue == 2);
    assert(limitedView.maximumDexterityLimits[1].id == "wornArmor");
    assert(limitedView.maximumDexterityLimits[1].baseValue == 2);
    assert(limitedView.maximumDexterityLimits[1].effectiveValue == 3);
    manager.removeFromCollection(MaximumDexterityLimitsResource, "carriedLoad");
    assert(armorClass.maximumDexterityBonus() == 3);

    dexterityModifier = -2;
    assert(armorClassValue(armorClass, ArmorClassType::Normal) == 20);
    assert(armorClassValue(armorClass, ArmorClassType::Touch) == 13);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted) == 21);

    manager.addToCollection(ArmorClassAbilityReplacementsResource, ArmorClassAbilityReplacement(ArmorClassAbilityReplacementDefinition{
        .id = "scaledFist",
        .source = "Pugno Scagliato",
        .abilityType = AbilityType::Charisma
    }));
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 25);
    assert(armorClassValue(armorClass, ArmorClassType::Touch, "scaledFist") == 18);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted, "scaledFist") == 23);

    manager.removeFromCollection(MaximumDexterityLimitsResource, "wornArmor");
    assert(!armorClass.maximumDexterityBonus().has_value());
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 27);
    assert(throwsInvalidArgument([&manager]
    {
        manager.removeFromCollection(MaximumDexterityLimitsResource, "wornArmor");
    }));

    manager.addToCollection(MaximumDexterityLimitsResource, MaximumDexterityLimit(MaximumDexterityLimitDefinition{
        .id = "towerShield",
        .source = "Scudo torre",
        .type = MaximumDexterityLimitType::Shield,
        .expression = "2"
    }));
    assert(throwsInvalidArgument([&manager]
    {
        manager.addToCollection(MaximumDexterityLimitsResource, MaximumDexterityLimit(MaximumDexterityLimitDefinition{
            .id = "towerShield",
            .source = "Duplicato",
            .type = MaximumDexterityLimitType::Shield,
            .expression = "1"
        }));
    }));
    Modifier exactLimitModifier({.type = ModifierType::Bonus, .source = "Maestria con lo scudo", .description = "Aumento del limite dello scudo", .bonusType = BonusType::Generic, .expression = "1"});
    const std::string exactLimitModifierId = exactLimitModifier.id();
    manager.addModifier(maximumDexterityResourceName("towerShield"), std::move(exactLimitModifier));
    assert(armorClass.maximumDexterityBonus() == 3);
    assert(throwsInvalidArgument([&manager]
    {
        manager.removeFromCollection(MaximumDexterityLimitsResource, "towerShield");
    }));
    manager.removeModifier(maximumDexterityResourceName("towerShield"), exactLimitModifierId);
    manager.removeFromCollection(MaximumDexterityLimitsResource, "towerShield");
    assert(!armorClass.maximumDexterityBonus().has_value());

    manager.addModifier(ArmorClassAllResource, Modifier({.type = ModifierType::Bonus, .source = "Monaco", .description = "Bonus di Saggezza alla CA", .bonusType = BonusType::Generic, .expression = "@wisMod", .condition = std::nullopt, .requirements = std::vector<Requirement>{Requirement("@armor.worn == 0", "Non applicabile mentre indossi un'armatura")}}));
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 27);
    assert(armorClassValue(armorClass, ArmorClassType::Touch, "scaledFist") == 20);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted, "scaledFist") == 23);

    armorWorn = 0;
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 30);
    assert(armorClassValue(armorClass, ArmorClassType::Touch, "scaledFist") == 23);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted, "scaledFist") == 26);

    ArmorClassView view = armorClass.toView();
    assert(!view.maximumDexterityBonus.has_value());
    assert(view.maximumDexterityLimits.empty());
    assert(!view.abilityBonusSuppressed);
    assert(view.abilitySuppressions.empty());
    assert(view.abilityOptions.size() == 2);
    assert(!view.abilityOptions[0].replacementId.has_value());
    assert(view.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(view.abilityOptions[0].abilityModifier == -2);
    assert(view.abilityOptions[1].replacementId == "scaledFist");
    assert(view.abilityOptions[1].source == "Pugno Scagliato");
    assert(view.abilityOptions[1].abilityType == AbilityType::Charisma);
    assert(view.abilityOptions[1].abilityModifier == 5);
    assert(view.abilityOptions[1].values.size() == 3);
    assert(view.abilityOptions[1].values[0].type == ArmorClassType::Normal);
    assert(view.abilityOptions[1].values[0].appliedAbilityModifier == 5);
    assert(view.abilityOptions[1].values[0].totalValue == 30);
    assert(view.abilityOptions[1].values[1].type == ArmorClassType::Touch);
    assert(view.abilityOptions[1].values[1].totalValue == 23);
    assert(view.abilityOptions[1].values[2].type == ArmorClassType::FlatFooted);
    assert(view.abilityOptions[1].values[2].appliedAbilityModifier == 0);
    assert(view.abilityOptions[1].values[2].totalValue == 26);

    manager.addToCollection(MaximumDexterityLimitsResource, MaximumDexterityLimit(MaximumDexterityLimitDefinition{
        .id = "suppressionTestArmor",
        .source = "Armatura di prova",
        .type = MaximumDexterityLimitType::Armor,
        .expression = "2"
    }));
    assert(armorClass.maximumDexterityBonus() == 3);
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 28);
    manager.addToCollection(ArmorClassAbilitySuppressionsResource, ArmorClassAbilitySuppression(ArmorClassAbilitySuppressionDefinition{
        .id = " blinded ",
        .source = " Accecato "
    }));
    view = armorClass.toView();
    assert(view.abilityBonusSuppressed);
    assert(view.abilitySuppressions.size() == 1);
    assert(view.abilitySuppressions[0].id == "blinded");
    assert(view.abilitySuppressions[0].source == "Accecato");
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 25);
    assert(armorClassValue(armorClass, ArmorClassType::Touch, "scaledFist") == 18);
    assert(armorClassValue(armorClass, ArmorClassType::FlatFooted, "scaledFist") == 26);
    assert(armorClassValue(armorClass, ArmorClassType::Normal) == 23);
    assert(throwsInvalidArgument([&manager]
    {
        manager.addToCollection(ArmorClassAbilitySuppressionsResource, ArmorClassAbilitySuppression(ArmorClassAbilitySuppressionDefinition{
            .id = "blinded",
            .source = "Duplicata"
        }));
    }));
    manager.removeFromCollection(ArmorClassAbilitySuppressionsResource, "blinded");
    assert(!armorClass.toView().abilityBonusSuppressed);
    assert(armorClassValue(armorClass, ArmorClassType::Normal, "scaledFist") == 28);
    assert(throwsInvalidArgument([&manager]
    {
        manager.removeFromCollection(ArmorClassAbilitySuppressionsResource, "blinded");
    }));
    manager.removeFromCollection(MaximumDexterityLimitsResource, "suppressionTestArmor");

    assert(throwsInvalidArgument([&manager]
    {
        manager.addToCollection(ArmorClassAbilityReplacementsResource, ArmorClassAbilityReplacement(ArmorClassAbilityReplacementDefinition{
            .id = "scaledFist",
            .source = "Duplicato",
            .abilityType = AbilityType::Wisdom
        }));
    }));
    manager.removeFromCollection(ArmorClassAbilityReplacementsResource, "scaledFist");
    assert(armorClass.toView().abilityOptions.size() == 1);
    assert(throwsInvalidArgument([&manager]
    {
        manager.removeFromCollection(ArmorClassAbilityReplacementsResource, "scaledFist");
    }));
    assert(displayName(MaximumDexterityLimitType::Armor) == "Armatura");
    assert(maximumDexterityResourceName(MaximumDexterityLimitType::Load) == "armorClass.maxDex.load");

    return 0;
}
