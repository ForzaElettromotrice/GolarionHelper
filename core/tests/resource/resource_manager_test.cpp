#include "golarion/resource/modifier.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>

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

    ResourceManager manager;
    manager.registerEnhanceableResource(" str ");

    assert(throwsInvalidArgument([&]
    {
        manager.registerEnhanceableResource("str");
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.registerEnhanceableResource(" ");
    }));

    int strength = 10;
    manager.registerTarget(" str ", [&strength]
    {
        return strength;
    });

    assert(manager.targetValue("str") == 10);
    strength = 16;
    assert(manager.targetValue(" str ") == 16);
    assert(manager.evaluateExpression("@str / 2 + 2") == 10);

    int addedItem = 0;
    std::string removedItem;
    manager.registerCollectionResource<int>(" test.collection ", [&addedItem](int item)
    {
        addedItem = item;
    }, [&removedItem](std::string_view id)
    {
        removedItem = id;
    });
    manager.addToCollection("test.collection", 7);
    assert(addedItem == 7);
    manager.removeFromCollection("test.collection", "entry");
    assert(removedItem == "entry");
    assert(throwsInvalidArgument([&]
    {
        manager.addToCollection("test.collection", std::string("wrong type"));
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.registerAccumulatedResource("test.collection");
    }));

    manager.registerTarget("dex", []
    {
        return 12;
    });

    const ResourceManagerView managerView = manager.toView();
    assert(managerView.targets.size() == 2);
    assert(managerView.targets[0].name == "dex");
    assert(managerView.targets[0].value == 12);
    assert(managerView.targets[1].name == "str");
    assert(managerView.targets[1].value == 16);
    assert(managerView.enhanceableResources.size() == 1);
    assert(managerView.enhanceableResources[0].name == "str");
    assert(managerView.enhanceableResources[0].parentResources.empty());
    assert(managerView.enhanceableResources[0].modifiers.total == 0);
    assert(managerView.collections.size() == 1);
    assert(managerView.collections[0] == "test.collection");

    assert(throwsInvalidArgument([&]
    {
        manager.registerTarget("str", []
        {
            return 12;
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.targetValue("wis");
    }));

    Modifier modifier(ModifierType::Bonus, "Cintura", "Bonus alla Forza", BonusType::Enhancement, "2");
    const std::string modifierId = modifier.id();
    manager.addModifier("str", std::move(modifier));
    assert(manager.modifierTotal(" str ") == 2);
    assert(manager.modifierSetView("str").total == 2);

    manager.removeModifier(" str ", modifierId);
    assert(manager.modifierTotal("str") == 0);

    assert(throwsInvalidArgument([&]
    {
        manager.removeModifier("str", modifierId);
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.addModifier("dex", Modifier(ModifierType::Bonus, "Sorgente", "Descrizione", BonusType::Luck, "1"));
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.modifierTotal("dex");
    }));

    manager.unregisterCollectionResource("test.collection");
    assert(throwsInvalidArgument([&]
    {
        manager.removeFromCollection("test.collection", "entry");
    }));

    manager.registerAccumulatedResource(" hp.max ");
    manager.addContribution("hp.max", Contribution("class.hitDice", "24"));
    manager.addContribution("hp.max", Contribution("constitution", "-3"));
    assert(manager.contributionTotal(" hp.max ") == 21);

    manager.removeContribution("hp.max", "constitution");
    assert(manager.contributionTotal("hp.max") == 24);
    assert(throwsInvalidArgument([&]
    {
        manager.addModifier("hp.max", Modifier(ModifierType::Bonus, "Sorgente", "Descrizione", BonusType::Luck, "1"));
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.registerEnhanceableResource("hp.max");
    }));

    manager.unregisterAccumulatedResource("hp.max");
    assert(throwsInvalidArgument([&]
    {
        manager.contributionTotal("hp.max");
    }));

    ResourceManager inheritanceManager;
    inheritanceManager.registerEnhanceableResource("skill.all");
    inheritanceManager.registerEnhanceableResource("skill.craft");
    inheritanceManager.registerEnhanceableResource("skill.movement");
    inheritanceManager.registerEnhanceableResource("skill.craft.alchemy", {"skill.all", "skill.craft", "skill.movement"});

    Modifier allSkillsBonus(ModifierType::Bonus, "Tratto", "Bonus a tutte le abilità", BonusType::Racial, "1");
    const std::string allSkillsBonusId = allSkillsBonus.id();
    inheritanceManager.addModifier("skill.all", std::move(allSkillsBonus));

    Modifier craftBonus(ModifierType::Bonus, "Strumento", "Bonus ad Artigianato", BonusType::Competence, "2");
    const std::string craftBonusId = craftBonus.id();
    inheritanceManager.addModifier("skill.craft", std::move(craftBonus));

    Modifier alchemyBonus(ModifierType::Bonus, "Laboratorio", "Bonus ad Alchimia", BonusType::Competence, "4");
    const std::string alchemyBonusId = alchemyBonus.id();
    inheritanceManager.addModifier("skill.craft.alchemy", std::move(alchemyBonus));

    assert(inheritanceManager.modifierTotal("skill.craft.alchemy") == 5);
    const ModifierSetView alchemyView = inheritanceManager.modifierSetView("skill.craft.alchemy");
    assert(alchemyView.total == 5);
    assert(alchemyView.modifiers.size() == 3);

    inheritanceManager.registerEnhanceableResource("skill.craft.weapons", {"skill.all", "skill.craft"});
    assert(inheritanceManager.modifierTotal("skill.craft.weapons") == 3);

    inheritanceManager.removeModifier("skill.craft.alchemy", alchemyBonusId);
    assert(inheritanceManager.modifierTotal("skill.craft.alchemy") == 3);
    inheritanceManager.removeModifier("skill.craft", craftBonusId);
    assert(inheritanceManager.modifierTotal("skill.craft.alchemy") == 1);
    inheritanceManager.removeModifier("skill.all", allSkillsBonusId);
    assert(inheritanceManager.modifierTotal("skill.craft.alchemy") == 0);

    assert(throwsInvalidArgument([&]
    {
        inheritanceManager.registerEnhanceableResource("skill.invalid", {"skill.missing"});
    }));
    assert(throwsInvalidArgument([&]
    {
        inheritanceManager.registerEnhanceableResource("skill.self", {"skill.self"});
    }));
    assert(throwsInvalidArgument([&]
    {
        inheritanceManager.registerEnhanceableResource("skill.duplicateParents", {"skill.all", " skill.all "});
    }));
    assert(throwsInvalidArgument([&]
    {
        inheritanceManager.unregisterEnhanceableResource("skill.craft");
    }));

    inheritanceManager.unregisterEnhanceableResource("skill.craft.weapons");
    assert(throwsInvalidArgument([&]
    {
        inheritanceManager.modifierTotal("skill.craft.weapons");
    }));
    assert(throwsInvalidArgument([&]
    {
        inheritanceManager.unregisterEnhanceableResource("skill.missing");
    }));

    ResourceManager circularManager;
    circularManager.registerTarget("first", [&circularManager]
    {
        return circularManager.evaluateExpression("@second");
    });
    circularManager.registerTarget("second", [&circularManager]
    {
        return circularManager.evaluateExpression("@first");
    });

    try
    {
        circularManager.targetValue("first");
        assert(false);
    }
    catch (const std::invalid_argument &exception)
    {
        assert(std::string(exception.what()).find("first -> second -> first") != std::string::npos);
    }

    return 0;
}
