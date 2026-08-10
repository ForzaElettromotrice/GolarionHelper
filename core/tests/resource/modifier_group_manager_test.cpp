#include "golarion/resource/modifier_group.hpp"
#include "golarion/resource/modifier_group_manager.hpp"
#include "golarion/data/modifier_group_manager_save_data.hpp"
#include "golarion/view/modifier_group_manager_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    golarion::ModifierGroup groupFor(std::string resourceName, int value)
    {
        return golarion::ModifierGroup(std::vector<golarion::TargetedModifier>{
            golarion::TargetedModifier{
                std::move(resourceName),
                golarion::Modifier(golarion::ModifierType::Bonus, "Potenziamento", "Bonus di test", golarion::BonusType::Racial, std::to_string(value))
            }
        });
    }

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
    resourceManager.registerEnhanceableResource("str");
    ModifierGroupManager groupManager(resourceManager);

    groupManager.addGroup("manual.strength", groupFor("str", 2), false);
    assert(resourceManager.modifierTotal("str") == 0);

    groupManager.setGroupEnabled("manual.strength", true);
    assert(resourceManager.modifierTotal("str") == 2);

    groupManager.setGroupEnabled("manual.strength", true);
    assert(resourceManager.modifierTotal("str") == 2);

    groupManager.setGroupEnabled("manual.strength", false);
    assert(resourceManager.modifierTotal("str") == 0);

    groupManager.removeGroup("manual.strength");
    assert(throwsInvalidArgument([&]
    {
        groupManager.setGroupEnabled("manual.strength", true);
    }));

    groupManager.addGroup("active", groupFor("str", 3), true);
    assert(resourceManager.modifierTotal("str") == 3);
    groupManager.removeGroup("active");
    assert(resourceManager.modifierTotal("str") == 0);

    resourceManager.registerEnhanceableResource("skill.custom");
    groupManager.addGroup("custom.only", groupFor("skill.custom", 2), true);
    ModifierGroup mixedGroup(std::vector<TargetedModifier>{
        TargetedModifier{"str", Modifier(ModifierType::Bonus, "Potenziamento", "Bonus alla Forza", BonusType::Racial, "3")},
        TargetedModifier{"skill.custom", Modifier(ModifierType::Bonus, "Potenziamento", "Bonus custom", BonusType::Racial, "2")}
    });
    groupManager.addGroup("custom.mixed", std::move(mixedGroup), true);
    assert(resourceManager.modifierTotal("str") == 3);
    assert(resourceManager.modifierSetView("skill.custom").modifiers.size() == 2);

    groupManager.removeModifiersForResource("skill.custom");
    assert(resourceManager.modifierTotal("str") == 3);
    assert(resourceManager.modifierSetView("skill.custom").modifiers.empty());
    ModifierGroupManagerView cleanedView = groupManager.toView();
    assert(cleanedView.groups.size() == 1);
    assert(cleanedView.groups[0].id == "custom.mixed");
    assert(cleanedView.groups[0].enabled);
    assert(cleanedView.groups[0].group.modifiers.size() == 1);
    assert(cleanedView.groups[0].group.modifiers[0].resourceName == "str");
    groupManager.removeGroup("custom.mixed");
    resourceManager.unregisterEnhanceableResource("skill.custom");

    groupManager.addGroup("duplicate", groupFor("str", 1), false);
    assert(throwsInvalidArgument([&]
    {
        groupManager.addGroup(" duplicate ", groupFor("str", 1), false);
    }));

    ModifierGroup partiallyInvalid(std::vector<TargetedModifier>{
        TargetedModifier{"str", Modifier(ModifierType::Bonus, "Primo", "Bonus valido", BonusType::Racial, "2")},
        TargetedModifier{"unknown", Modifier(ModifierType::Bonus, "Secondo", "Bonus non valido", BonusType::Racial, "4")}
    });
    assert(throwsInvalidArgument([&]
    {
        groupManager.addGroup("invalid", std::move(partiallyInvalid), true);
    }));
    assert(resourceManager.modifierTotal("str") == 0);

    ModifierGroupManagerView view = groupManager.toView();
    assert(view.groups.size() == 1);
    assert(view.groups[0].id == "duplicate");
    assert(!view.groups[0].enabled);
    assert(view.groups[0].group.modifiers.size() == 1);

    groupManager.addGroup("active", groupFor("str", 3), true);
    ModifierGroupManagerSaveData saveData = groupManager.toSaveData();
    assert(saveData.groups.size() == 2);
    assert(saveData.groups[0].id == "active");
    assert(saveData.groups[0].enabled);
    assert(saveData.groups[0].group.modifiers[0].modifier.expression == "3");
    assert(saveData.groups[1].id == "duplicate");
    assert(!saveData.groups[1].enabled);

    return 0;
}
