#pragma once

#include <string>

namespace golarion
{
    class ResourceManager;
    struct RequirementView;

    class Requirement final
    {
    public:
        Requirement(std::string expression, std::string failureReason);
        RequirementView toView(ResourceManager &resourceManager) const;
        bool isSatisfied(ResourceManager &resourceManager) const;

        const std::string &expression() const;
        const std::string &failureReason() const;

    private:
        std::string expression_;
        std::string failureReason_;
    };
}
