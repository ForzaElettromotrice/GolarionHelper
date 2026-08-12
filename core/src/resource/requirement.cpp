#include "golarion/resource/requirement.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/requirement_view.hpp"

#include <utility>

namespace golarion
{
    Requirement::Requirement(std::string expression, std::string failureReason)
        : expression_(normalize(expression)),
          failureReason_(normalize(failureReason))
    {
    }

    RequirementView Requirement::toView(ResourceManager &resourceManager) const
    {
        return RequirementView{
            .expression = expression_,
            .failureReason = failureReason_,
            .satisfied = isSatisfied(resourceManager)
        };
    }

    bool Requirement::isSatisfied(ResourceManager &resourceManager) const
    {
        return resourceManager.evaluateExpression(expression_) != 0;
    }

    const std::string &Requirement::expression() const
    {
        return expression_;
    }

    const std::string &Requirement::failureReason() const
    {
        return failureReason_;
    }
}
