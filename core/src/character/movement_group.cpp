#include "golarion/character/movement_group.hpp"

#include "golarion/data/movement_group_save_data.hpp"
#include "golarion/view/movement_group_view.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace
{
    std::vector<golarion::MovementGrant> grantsFrom(const golarion::MovementGroupSaveData &data)
    {
        std::vector<golarion::MovementGrant> grants;
        grants.reserve(data.grants.size());
        for (const golarion::MovementGrantSaveData &grant : data.grants)
        {
            grants.emplace_back(grant);
        }
        return grants;
    }

    std::vector<golarion::MovementAdjustment> adjustmentsFrom(const golarion::MovementGroupSaveData &data)
    {
        std::vector<golarion::MovementAdjustment> adjustments;
        adjustments.reserve(data.adjustments.size());
        for (const golarion::MovementAdjustmentSaveData &adjustment : data.adjustments)
        {
            adjustments.emplace_back(adjustment);
        }
        return adjustments;
    }
}

namespace golarion
{
    MovementGroup::MovementGroup(std::vector<MovementGrant> grants, std::vector<MovementAdjustment> adjustments)
        : grants_(std::move(grants)), adjustments_(std::move(adjustments))
    {
        if (grants_.empty() && adjustments_.empty())
        {
            throw std::invalid_argument("movement group must not be empty");
        }

        std::unordered_set<std::string> grantIds;
        for (const MovementGrant &grant : grants_)
        {
            const MovementGrantSaveData data = grant.toSaveData();
            if (!grantIds.insert(data.id).second)
            {
                throw std::invalid_argument("movement grant is duplicated in group: " + data.id);
            }
        }

        std::unordered_set<std::string> adjustmentIds;
        for (const MovementAdjustment &adjustment : adjustments_)
        {
            const MovementAdjustmentSaveData data = adjustment.toSaveData();
            if (!adjustmentIds.insert(data.id).second)
            {
                throw std::invalid_argument("movement adjustment is duplicated in group: " + data.id);
            }
        }
    }

    MovementGroup::MovementGroup(const MovementGroupSaveData &data) : MovementGroup(grantsFrom(data), adjustmentsFrom(data))
    {
    }

    MovementGroupView MovementGroup::toView() const
    {
        std::vector<MovementGroupView::GrantView> grants;
        grants.reserve(grants_.size());
        for (const MovementGrant &grant : grants_)
        {
            const MovementGrantSaveData data = grant.toSaveData();
            grants.push_back(MovementGroupView::GrantView{
                .id = data.id,
                .source = data.source,
                .type = data.type,
                .baseSpeedExpression = data.baseSpeedExpression,
                .maneuverability = data.maneuverability,
                .affectedByArmor = data.affectedByArmor,
                .affectedByLoad = data.affectedByLoad
            });
        }

        std::vector<MovementGroupView::AdjustmentView> adjustments;
        adjustments.reserve(adjustments_.size());
        for (const MovementAdjustment &adjustment : adjustments_)
        {
            const MovementAdjustmentSaveData data = adjustment.toSaveData();
            adjustments.push_back(MovementGroupView::AdjustmentView{
                .id = data.id,
                .source = data.source,
                .description = data.description,
                .type = data.type,
                .selector = data.selector,
                .expression = data.expression,
                .condition = data.condition
            });
        }
        return MovementGroupView{.grants = std::move(grants), .adjustments = std::move(adjustments)};
    }

    MovementGroupSaveData MovementGroup::toSaveData() const
    {
        std::vector<MovementGrantSaveData> grants;
        grants.reserve(grants_.size());
        for (const MovementGrant &grant : grants_)
        {
            grants.push_back(grant.toSaveData());
        }

        std::vector<MovementAdjustmentSaveData> adjustments;
        adjustments.reserve(adjustments_.size());
        for (const MovementAdjustment &adjustment : adjustments_)
        {
            adjustments.push_back(adjustment.toSaveData());
        }
        return MovementGroupSaveData{.grants = std::move(grants), .adjustments = std::move(adjustments)};
    }

    const std::vector<MovementGrant> &MovementGroup::grants() const
    {
        return grants_;
    }

    const std::vector<MovementAdjustment> &MovementGroup::adjustments() const
    {
        return adjustments_;
    }
}
