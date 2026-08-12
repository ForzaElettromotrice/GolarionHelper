#include "golarion/character/base_attack_bonus.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/base_attack_bonus_view.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    BaseAttackBonus::BaseAttackBonus(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerAccumulatedResource(BaseAttackBonusResource);
        resourceManager_.registerTarget(BaseAttackBonusTarget, [this]
        {
            return total();
        });
    }

    BaseAttackBonusView BaseAttackBonus::toView()
    {
        ContributionSetView contributions = resourceManager_.contributionSetView(BaseAttackBonusResource);
        if (contributions.total < 0)
        {
            throw std::invalid_argument("base attack bonus must not be negative");
        }

        return BaseAttackBonusView{
            .total = contributions.total,
            .contributions = std::move(contributions)
        };
    }

    int BaseAttackBonus::total()
    {
        const int value = resourceManager_.contributionTotal(BaseAttackBonusResource);
        if (value < 0)
        {
            throw std::invalid_argument("base attack bonus must not be negative");
        }
        return value;
    }
}
