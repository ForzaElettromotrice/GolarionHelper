#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/base_attack_bonus_view.hpp"

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

    BaseAttackBonusView view = baseAttackBonus.toView();
    assert(view.total == 0);
    assert(view.contributions.contributions.empty());
    assert(resourceManager.targetValue(BaseAttackBonusTarget) == 0);
    assert(resourceManager.evaluateExpression("@bab + 2") == 2);

    resourceManager.addContribution(BaseAttackBonusResource, Contribution("fighter", "5"));
    resourceManager.addContribution(BaseAttackBonusResource, Contribution("prestigeClass", "2"));

    view = baseAttackBonus.toView();
    assert(view.total == 7);
    assert(view.contributions.total == 7);
    assert(view.contributions.contributions.size() == 2);
    assert(resourceManager.targetValue(BaseAttackBonusTarget) == 7);

    resourceManager.removeContribution(BaseAttackBonusResource, "fighter");
    assert(baseAttackBonus.toView().total == 2);

    resourceManager.addContribution(BaseAttackBonusResource, Contribution("penalty", "-3"));
    assert(throwsInvalidArgument([&]
    {
        static_cast<void>(baseAttackBonus.toView());
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.targetValue(BaseAttackBonusTarget);
    }));

    return 0;
}
