#pragma once

#include <string>
#include <string_view>

namespace golarion
{
    class ContributionGroup;
    class ContributionGroupManager;
    class ContributionSet;
    class ResourceManager;
    struct ContributionSaveData;
    struct ContributionView;

    class Contribution final
    {
    public:
        Contribution(std::string id, std::string expression);
        explicit Contribution(const ContributionSaveData &data);

        ContributionView toView(ResourceManager &resourceManager) const;
        ContributionSaveData toSaveData() const;

    private:
        friend class ContributionGroup;
        friend class ContributionGroupManager;
        friend class ContributionSet;

        int resolveValue(ResourceManager &resourceManager) const;
        ContributionView toView(int resolvedValue) const;

        std::string id_;
        std::string expression_;
    };
}
