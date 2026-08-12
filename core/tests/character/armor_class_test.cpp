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
    manager.addModifier(ArmorClassAllResource, Modifier(ModifierType::Bonus, "Anello", "Bonus di deviazione", BonusType::Deflection, "1"));
    manager.addModifier(ArmorClassReflexiveResource, Modifier(ModifierType::Bonus, "Schivare", "Bonus di schivare", BonusType::Dodge, "1"));
    manager.addModifier(ArmorClassSolidResource, Modifier(ModifierType::Bonus, "Armatura", "Bonus di armatura", BonusType::Armor, "6"));
    manager.addModifier(ArmorClassSolidResource, Modifier(ModifierType::Bonus, "Scudo", "Bonus di scudo", BonusType::Shield, "2"));
    manager.addModifier(resourceName(ArmorClassType::Normal), Modifier(ModifierType::Bonus, "Difesa", "Bonus solo alla CA normale", BonusType::Circumstance, "2"));
    manager.addModifier(resourceName(ArmorClassType::Touch), Modifier(ModifierType::Bonus, "Campo", "Bonus solo a contatto", BonusType::Insight, "3"));
    manager.addModifier(resourceName(ArmorClassType::FlatFooted), Modifier(ModifierType::Bonus, "Istinto", "Bonus solo da impreparato", BonusType::Luck, "4"));

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

    manager.addModifier(maximumDexterityResourceName(MaximumDexterityLimitType::Armor), Modifier(ModifierType::Bonus, "Addestramento nelle armature", "Aumento del bonus massimo di Destrezza", BonusType::Generic, "1"));
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
    Modifier exactLimitModifier(ModifierType::Bonus, "Maestria con lo scudo", "Aumento del limite dello scudo", BonusType::Generic, "1");
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

    manager.addModifier(ArmorClassAllResource, Modifier(
        ModifierType::Bonus,
        "Monaco",
        "Bonus di Saggezza alla CA",
        BonusType::Generic,
        "@wisMod",
        std::nullopt,
        std::vector<Requirement>{Requirement("@armor.worn == 0", "Non applicabile mentre indossi un'armatura")}
    ));
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
