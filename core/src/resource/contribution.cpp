#include "golarion/resource/contribution.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/contribution_view.hpp"

#include <utility>

namespace golarion
{
    Contribution::Contribution(std::string id, std::string expression) : id_(normalize(id)), expression_(normalize(expression))
    {
    }

    ContributionView Contribution::toView(ResourceManager &resourceManager) const
    {
        return toView(resolveValue(resourceManager));
    }

    int Contribution::resolveValue(ResourceManager &resourceManager) const
    {
        return resourceManager.evaluateExpression(expression_);
    }

    ContributionView Contribution::toView(int resolvedValue) const
    {
        return ContributionView{
            .id = id_,
            .expression = expression_,
            .resolvedValue = resolvedValue
        };
    }
}
