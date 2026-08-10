#include "golarion/resource/contribution_group.hpp"

#include "golarion/data/contribution_group_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/contribution_group_view.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace
{
    std::vector<golarion::TargetedContribution> targetedContributions(const golarion::ContributionGroupSaveData &data)
    {
        std::vector<golarion::TargetedContribution> contributions;
        contributions.reserve(data.contributions.size());

        for (const auto &[resourceName, contribution] : data.contributions)
        {
            contributions.push_back(golarion::TargetedContribution{
                .resourceName = resourceName,
                .contribution = golarion::Contribution(contribution)
            });
        }
        return contributions;
    }
}

namespace golarion
{
    ContributionGroup::ContributionGroup(std::vector<TargetedContribution> contributions) : contributions_(std::move(contributions))
    {
        if (contributions_.empty())
        {
            throw std::invalid_argument("contribution group must not be empty");
        }

        std::unordered_set<std::string> contributionIds;
        for (TargetedContribution &targetedContribution : contributions_)
        {
            targetedContribution.resourceName = normalize(targetedContribution.resourceName);
            if (!contributionIds.insert(targetedContribution.contribution.id_).second)
            {
                throw std::invalid_argument("contribution is duplicated in group: " + targetedContribution.contribution.id_);
            }
        }
    }

    ContributionGroup::ContributionGroup(const ContributionGroupSaveData &data) : ContributionGroup(targetedContributions(data))
    {
    }

    ContributionGroupView ContributionGroup::toView(ResourceManager &resourceManager) const
    {
        std::vector<ContributionGroupView::TargetedContributionView> contributionViews;
        contributionViews.reserve(contributions_.size());

        for (const TargetedContribution &targetedContribution : contributions_)
        {
            contributionViews.push_back(ContributionGroupView::TargetedContributionView{
                .resourceName = targetedContribution.resourceName,
                .contribution = targetedContribution.contribution.toView(resourceManager)
            });
        }
        return ContributionGroupView{.contributions = std::move(contributionViews)};
    }

    ContributionGroupSaveData ContributionGroup::toSaveData() const
    {
        std::vector<ContributionGroupSaveData::TargetedContributionSaveData> contributionData;
        contributionData.reserve(contributions_.size());

        for (const TargetedContribution &targetedContribution : contributions_)
        {
            contributionData.push_back(ContributionGroupSaveData::TargetedContributionSaveData{
                .resourceName = targetedContribution.resourceName,
                .contribution = targetedContribution.contribution.toSaveData()
            });
        }
        return ContributionGroupSaveData{.contributions = std::move(contributionData)};
    }

    const std::vector<TargetedContribution> &ContributionGroup::contributions() const
    {
        return contributions_;
    }
}
