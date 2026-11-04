#pragma once

#include "golarion/resource/contribution.hpp"

#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct ContributionSetView;

    class ContributionSet final
    {
    public:
        void addContribution(Contribution contribution);
        void removeContribution(std::string_view contributionId);
        int calculateTotal(ResourceManager &resourceManager) const;
        ContributionSetView toView(ResourceManager &resourceManager) const;

    private:
        std::vector<Contribution> contributions_;
    };
}
