#pragma once

#include "golarion/character/movement.hpp"

#include <vector>

namespace golarion
{
    struct MovementGroupSaveData;
    struct MovementGroupView;

    class MovementGroup final
    {
    public:
        MovementGroup(std::vector<MovementGrant> grants, std::vector<MovementAdjustment> adjustments);
        explicit MovementGroup(const MovementGroupSaveData &data);

        MovementGroupView toView() const;
        MovementGroupSaveData toSaveData() const;
        const std::vector<MovementGrant> &grants() const;
        const std::vector<MovementAdjustment> &adjustments() const;

    private:
        std::vector<MovementGrant> grants_;
        std::vector<MovementAdjustment> adjustments_;
    };
}
