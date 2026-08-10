#include "golarion/resource/contribution_group.hpp"
#include "golarion/resource/contribution_group_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/data/contribution_group_manager_save_data.hpp"
#include "golarion/view/contribution_group_manager_view.hpp"

#include <cassert>
#include <stdexcept>
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

    golarion::ContributionGroup hitPointGroup()
    {
        return golarion::ContributionGroup(std::vector<golarion::TargetedContribution>{
            golarion::TargetedContribution{
                .resourceName = "hp.max",
                .contribution = golarion::Contribution("toughness", "3")
            },
            golarion::TargetedContribution{
                .resourceName = "hp.secondary",
                .contribution = golarion::Contribution("favoredClass", "@level")
            }
        });
    }
}

int main()
{
    using namespace golarion;

    ResourceManager manager;
    manager.registerAccumulatedResource("hp.max");
    manager.registerAccumulatedResource("hp.secondary");
    manager.registerTarget("level", []
    {
        return 4;
    });

    ContributionGroupManager groupManager(manager);
    groupManager.addGroup("user.hitPoints", hitPointGroup(), false);
    assert(manager.contributionTotal("hp.max") == 0);
    assert(manager.contributionTotal("hp.secondary") == 0);
    ContributionGroupManagerView view = groupManager.toView();
    assert(view.groups.size() == 1);
    assert(view.groups[0].id == "user.hitPoints");
    assert(!view.groups[0].enabled);

    const ContributionGroupManagerSaveData data = groupManager.toSaveData();
    assert(data.groups.size() == 1);
    assert(data.groups[0].group.contributions[0].contribution.id == "toughness");

    groupManager.setGroupEnabled(" user.hitPoints ", true);
    assert(manager.contributionTotal("hp.max") == 3);
    assert(manager.contributionTotal("hp.secondary") == 4);
    view = groupManager.toView();
    assert(view.groups[0].enabled);

    groupManager.setGroupEnabled("user.hitPoints", true);
    assert(manager.contributionTotal("hp.max") == 3);

    groupManager.setGroupEnabled("user.hitPoints", false);
    assert(manager.contributionTotal("hp.max") == 0);
    assert(manager.contributionTotal("hp.secondary") == 0);

    assert(throwsInvalidArgument([&]
    {
        groupManager.addGroup("user.hitPoints", hitPointGroup(), false);
    }));

    groupManager.removeGroup("user.hitPoints");
    assert(throwsInvalidArgument([&]
    {
        groupManager.setGroupEnabled("user.hitPoints", true);
    }));

    ContributionGroup invalidGroup(std::vector<TargetedContribution>{
        TargetedContribution{.resourceName = "hp.max", .contribution = Contribution("valid", "2")},
        TargetedContribution{.resourceName = "hp.missing", .contribution = Contribution("missing", "5")}
    });
    assert(throwsInvalidArgument([&]
    {
        groupManager.addGroup("invalid", std::move(invalidGroup), true);
    }));
    assert(manager.contributionTotal("hp.max") == 0);
    assert(throwsInvalidArgument([&]
    {
        groupManager.setGroupEnabled("invalid", true);
    }));

    groupManager.addGroup("active", hitPointGroup(), true);
    assert(manager.contributionTotal("hp.max") == 3);
    groupManager.removeGroup("active");
    assert(manager.contributionTotal("hp.max") == 0);

    return 0;
}
