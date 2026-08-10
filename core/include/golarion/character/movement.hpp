#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view MovementGrantsResource = "movement.grants";
    inline constexpr std::string_view MovementAdjustmentsResource = "movement.adjustments";

    class ResourceManager;
    struct MovementAdjustmentSaveData;
    struct MovementGrantSaveData;
    struct MovementView;

    enum class MovementType
    {
        Land,
        Climb,
        Swim,
        Burrow,
        Fly
    };

    enum class Maneuverability
    {
        Clumsy,
        Poor,
        Average,
        Good,
        Perfect
    };

    enum class MovementAdjustmentType
    {
        // The expression resolves to a percentage: 50 halves speed, 200 doubles it.
        SpeedMultiplier,
        SpeedLimit,
        ManeuverabilityChange,
        Block
    };

    std::string_view displayName(MovementType type);
    std::string_view displayName(Maneuverability maneuverability);
    std::string_view displayName(MovementAdjustmentType type);
    int flyCheckModifier(Maneuverability maneuverability);
    std::string movementResourceName(MovementType type);
    std::string movementResourceName(MovementType type, std::string_view grantId);

    struct MovementGrantDefinition
    {
        std::string id;
        std::string source;
        MovementType type;
        std::string baseSpeedExpression;
        std::optional<Maneuverability> maneuverability;
        bool affectedByArmor;
        bool affectedByLoad;
    };

    class MovementGrant final
    {
    public:
        explicit MovementGrant(MovementGrantDefinition definition);
        explicit MovementGrant(const MovementGrantSaveData &data);

        MovementGrantSaveData toSaveData() const;

    private:
        friend class Movement;

        std::string id_;
        std::string source_;
        MovementType type_;
        std::string baseSpeedExpression_;
        std::optional<Maneuverability> maneuverability_;
        bool affectedByArmor_;
        bool affectedByLoad_;
    };

    struct MovementSelector
    {
        std::optional<MovementType> type;
        std::optional<std::string> grantId;
    };

    struct MovementAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string description;
        MovementAdjustmentType type;
        MovementSelector selector;
        std::optional<std::string> expression;
        std::optional<std::string> condition;
    };

    class MovementAdjustment final
    {
    public:
        explicit MovementAdjustment(MovementAdjustmentDefinition definition);
        explicit MovementAdjustment(const MovementAdjustmentSaveData &data);

        MovementAdjustmentSaveData toSaveData() const;

    private:
        friend class Movement;

        std::string id_;
        std::string source_;
        std::string description_;
        MovementAdjustmentType type_;
        MovementSelector selector_;
        std::optional<std::string> expression_;
        std::optional<std::string> condition_;
    };

    class Movement final
    {
    public:
        explicit Movement(ResourceManager &resourceManager);

        MovementView toView();

    private:
        void addGrant(MovementGrant grant);
        void removeGrant(std::string_view grantId);
        void addAdjustment(MovementAdjustment adjustment);
        void removeAdjustment(std::string_view adjustmentId);
        bool adjustmentMatches(const MovementAdjustment &adjustment, const MovementGrant &grant) const;

        ResourceManager &resourceManager_;
        std::vector<MovementGrant> grants_;
        std::vector<MovementAdjustment> adjustments_;
    };
}
