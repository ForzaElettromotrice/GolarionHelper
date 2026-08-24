#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view SizeBaseResource = "size.base";
    inline constexpr std::string_view SizeReplacementsResource = "size.replacements";
    inline constexpr std::string_view SizeAdjustmentsResource = "size.adjustments";

    class ResourceManager;
    struct SizeView;

    enum class SizeCategory
    {
        Fine,
        Diminutive,
        Tiny,
        Small,
        Medium,
        Large,
        Huge,
        Gargantuan,
        Colossal
    };

    std::string_view displayName(SizeCategory category);

    struct SizeBaseDefinition
    {
        std::string id;
        std::string source;
        SizeCategory category;
    };

    class SizeBase final
    {
    public:
        explicit SizeBase(SizeBaseDefinition definition);

    private:
        friend class SizeManager;

        std::string id_;
        std::string source_;
        SizeCategory category_;
    };

    struct SizeReplacementDefinition
    {
        std::string id;
        std::string source;
        SizeCategory category;
        bool acceptsAdjustments = true;
    };

    class SizeReplacement final
    {
    public:
        explicit SizeReplacement(SizeReplacementDefinition definition);

    private:
        friend class SizeManager;

        std::string id_;
        std::string source_;
        SizeCategory category_;
        bool acceptsAdjustments_;
    };

    struct SizeAdjustmentDefinition
    {
        std::string id;
        std::string source;
        int steps;
    };

    class SizeAdjustment final
    {
    public:
        explicit SizeAdjustment(SizeAdjustmentDefinition definition);

    private:
        friend class SizeManager;

        std::string id_;
        std::string source_;
        int steps_;
    };

    class SizeManager final
    {
    public:
        explicit SizeManager(ResourceManager &resourceManager);

        SizeManager(const SizeManager &) = delete;
        SizeManager &operator=(const SizeManager &) = delete;
        SizeManager(SizeManager &&) = delete;
        SizeManager &operator=(SizeManager &&) = delete;

        SizeView toView() const;

    private:
        struct ResolvedSize
        {
            SizeCategory baseCategory;
            SizeCategory referenceCategory;
            SizeCategory effectiveCategory;
            bool hasReplacement;
            bool acceptsAdjustments;
            int selectedAdjustmentSteps;
            int appliedAdjustmentSteps;
        };

        struct AppliedModifier
        {
            std::string resourceName;
            std::string modifierId;
        };

        void addBase(SizeBase base);
        void removeBase(std::string_view baseId);
        void addReplacement(SizeReplacement replacement);
        void removeReplacement(std::string_view replacementId);
        void addAdjustment(SizeAdjustment adjustment);
        void removeAdjustment(std::string_view adjustmentId);
        void refreshAppliedModifiers();
        void refreshAppliedModifiersIfSizeChanged(SizeCategory previousCategory);
        void refreshCarryingCapacitySize();
        void addAppliedModifier(std::string_view resourceName, std::string description, int value);
        ResolvedSize resolve() const;

        ResourceManager &resourceManager_;
        std::optional<SizeBase> base_;
        std::map<std::string, SizeReplacement> replacements_;
        std::map<std::string, SizeAdjustment> adjustments_;
        std::vector<AppliedModifier> appliedModifiers_;
        bool carryingCapacitySizeRegistered_ = false;
    };
}
