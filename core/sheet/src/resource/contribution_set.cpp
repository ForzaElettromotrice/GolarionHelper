#include "golarion/resource/contribution_set.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/contribution_set_view.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace golarion
{
    void ContributionSet::addContribution(Contribution contribution)
    {
        const auto duplicate = std::ranges::find(contributions_, contribution.id_, &Contribution::id_);
        if (duplicate != contributions_.end())
        {
            throw std::invalid_argument("contribution is already registered: " + contribution.id_);
        }
        contributions_.push_back(std::move(contribution));
    }

    void ContributionSet::removeContribution(std::string_view contributionId)
    {
        const std::string normalizedId = normalize(contributionId);
        const auto previousSize = contributions_.size();
        std::erase_if(contributions_, [&normalizedId](const Contribution &contribution)
        {
            return contribution.id_ == normalizedId;
        });

        if (contributions_.size() == previousSize)
        {
            throw std::invalid_argument("contribution is not registered: " + normalizedId);
        }
    }

    int ContributionSet::calculateTotal(ResourceManager &resourceManager) const
    {
        long long total = 0;
        for (const Contribution &contribution : contributions_)
        {
            total += contribution.resolveValue(resourceManager);
            if (total < std::numeric_limits<int>::min() || total > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("contribution total is out of range");
            }
        }
        return static_cast<int>(total);
    }

    ContributionSetView ContributionSet::toView(ResourceManager &resourceManager) const
    {
        long long total = 0;
        std::vector<ContributionView> contributionViews;
        contributionViews.reserve(contributions_.size());

        for (const Contribution &contribution : contributions_)
        {
            const int resolvedValue = contribution.resolveValue(resourceManager);
            total += resolvedValue;
            if (total < std::numeric_limits<int>::min() || total > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("contribution total is out of range");
            }
            contributionViews.push_back(contribution.toView(resolvedValue));
        }

        return ContributionSetView{
            .total = static_cast<int>(total),
            .contributions = std::move(contributionViews)
        };
    }
}
