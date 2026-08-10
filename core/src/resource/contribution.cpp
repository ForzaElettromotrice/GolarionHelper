#include "golarion/resource/contribution.hpp"

#include "golarion/data/contribution_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/contribution_view.hpp"

#include <utility>

namespace golarion
{
    Contribution::Contribution(std::string id, std::string expression) : id_(normalize(id)), expression_(normalize(expression))
    {
    }

    Contribution::Contribution(const ContributionSaveData &data) : Contribution(data.id, data.expression)
    {
    }

    ContributionView Contribution::toView(ResourceManager &resourceManager) const
    {
        return toView(resolveValue(resourceManager));
    }

    ContributionSaveData Contribution::toSaveData() const
    {
        return ContributionSaveData{
            .id = id_,
            .expression = expression_
        };
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
