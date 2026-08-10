#pragma once

#include "golarion/resource/contribution.hpp"

#include <string>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct ContributionGroupSaveData;
    struct ContributionGroupView;

    struct TargetedContribution
    {
        std::string resourceName;
        Contribution contribution;
    };

    class ContributionGroup final
    {
    public:
        explicit ContributionGroup(std::vector<TargetedContribution> contributions);
        explicit ContributionGroup(const ContributionGroupSaveData &data);

        ContributionGroupView toView(ResourceManager &resourceManager) const;
        ContributionGroupSaveData toSaveData() const;
        const std::vector<TargetedContribution> &contributions() const;

    private:
        std::vector<TargetedContribution> contributions_;
    };
}
