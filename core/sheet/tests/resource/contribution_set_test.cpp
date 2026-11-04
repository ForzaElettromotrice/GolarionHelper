#include "golarion/resource/contribution.hpp"
#include "golarion/resource/contribution_set.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/contribution_set_view.hpp"

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

    ResourceManager manager;
    manager.registerTarget("level", []
    {
        return 3;
    });
    manager.registerTarget("conMod", []
    {
        return -1;
    });

    ContributionSet contributions;
    contributions.addContribution(Contribution("class.hitDice", "24"));
    contributions.addContribution(Contribution("constitution", "@level * @conMod"));
    assert(contributions.calculateTotal(manager) == 21);

    const ContributionSetView view = contributions.toView(manager);
    assert(view.total == 21);
    assert(view.contributions.size() == 2);
    assert(view.contributions[0].id == "class.hitDice");
    assert(view.contributions[1].resolvedValue == -3);

    assert(throwsInvalidArgument([&]
    {
        contributions.addContribution(Contribution("class.hitDice", "10"));
    }));

    contributions.removeContribution(" constitution ");
    assert(contributions.calculateTotal(manager) == 24);
    assert(throwsInvalidArgument([&]
    {
        contributions.removeContribution("constitution");
    }));

    return 0;
}
