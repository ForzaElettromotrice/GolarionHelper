#pragma once

#include <string>
#include <string_view>

namespace golarion
{
    class ContributionSet;
    class ResourceManager;
    struct ContributionView;

    class Contribution final
    {
    public:
        Contribution(std::string id, std::string expression);
        ContributionView toView(ResourceManager &resourceManager) const;

    private:
        friend class ContributionSet;

        int resolveValue(ResourceManager &resourceManager) const;
        ContributionView toView(int resolvedValue) const;

        std::string id_;
        std::string expression_;
    };
}
