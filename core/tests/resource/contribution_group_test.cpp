#include "golarion/resource/contribution_group.hpp"
#include "golarion/data/contribution_group_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/contribution_group_view.hpp"

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
}

int main()
{
    using namespace golarion;

    ContributionGroup group(std::vector<TargetedContribution>{
        TargetedContribution{
            .resourceName = " hp.max ",
            .contribution = Contribution("toughness", "3")
        },
        TargetedContribution{
            .resourceName = "hp.max",
            .contribution = Contribution("favoredClass", "@level")
        }
    });

    assert(group.contributions().size() == 2);
    assert(group.contributions()[0].resourceName == "hp.max");

    ResourceManager manager;
    manager.registerTarget("level", []
    {
        return 4;
    });
    const ContributionGroupView view = group.toView(manager);
    assert(view.contributions.size() == 2);
    assert(view.contributions[1].contribution.resolvedValue == 4);

    const ContributionGroupSaveData data = group.toSaveData();
    assert(data.contributions.size() == 2);
    assert(data.contributions[0].contribution.id == "toughness");
    ContributionGroup restored(data);
    assert(restored.contributions()[1].resourceName == "hp.max");

    assert(throwsInvalidArgument([]
    {
        ContributionGroup empty(std::vector<TargetedContribution>{});
    }));
    assert(throwsInvalidArgument([]
    {
        ContributionGroup duplicate(std::vector<TargetedContribution>{
            TargetedContribution{.resourceName = "hp.max", .contribution = Contribution("source", "2")},
            TargetedContribution{.resourceName = "hp.temporary", .contribution = Contribution(" source ", "3")}
        });
    }));

    return 0;
}
