#pragma once

#include "golarion/character/carrying_capacity.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view CarriedWeightsResource = "encumbrance.weights";

    class ResourceManager;
    struct EncumbranceView;

    enum class LoadCategory
    {
        Light,
        Medium,
        Heavy,
        Overloaded
    };

    std::string_view displayName(LoadCategory category);

    struct CarriedWeightDefinition
    {
        std::string id;
        std::string source;
        std::int64_t grams;
    };

    class CarriedWeight final
    {
    public:
        explicit CarriedWeight(CarriedWeightDefinition definition);

    private:
        friend class Encumbrance;

        std::string id_;
        std::string source_;
        std::int64_t grams_;
    };

    class Encumbrance final
    {
    public:
        Encumbrance(ResourceManager &resourceManager, CarryingCapacity &carryingCapacity);

        Encumbrance(const Encumbrance &) = delete;
        Encumbrance &operator=(const Encumbrance &) = delete;
        Encumbrance(Encumbrance &&) = delete;
        Encumbrance &operator=(Encumbrance &&) = delete;

        EncumbranceView toView();

    private:
        struct Resolution
        {
            std::int64_t totalWeightGrams;
            CarryingCapacity::ResolvedLoadLimits limits;
            LoadCategory category;
        };

        Resolution resolve();
        void synchronizeEffects();
        void applyEffects(LoadCategory category);
        void clearEffects();
        void addWeight(CarriedWeight weight);
        void removeWeight(std::string_view weightId);

        ResourceManager &resourceManager_;
        CarryingCapacity &carryingCapacity_;
        std::map<std::string, CarriedWeight> weights_;
        std::optional<LoadCategory> appliedCategory_;
        std::vector<std::function<void()>> effectCleanups_;
    };
}
