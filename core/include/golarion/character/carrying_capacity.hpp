#pragma once

#include "golarion/character/size.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view CarryingCapacityStrengthResource = "carryingCapacity.strength";
    inline constexpr std::string_view CarryingCapacityBodyTypeResource = "carryingCapacity.bodyType";
    inline constexpr std::string_view CarryingCapacitySizeResource = "carryingCapacity.size";
    inline constexpr std::string_view CarryingCapacityMultipliersResource = "carryingCapacity.multipliers";

    class ResourceManager;
    class Encumbrance;
    struct CarryingCapacityView;

    enum class CarryingBodyType
    {
        Biped,
        Quadruped
    };

    std::string_view displayName(CarryingBodyType type);

    struct CarryingBodyTypeBaseDefinition
    {
        std::string id;
        std::string source;
        CarryingBodyType type;
    };

    class CarryingBodyTypeBase final
    {
    public:
        explicit CarryingBodyTypeBase(CarryingBodyTypeBaseDefinition definition);

    private:
        friend class CarryingCapacity;

        std::string id_;
        std::string source_;
        CarryingBodyType type_;
    };

    struct CarryingCapacitySizeDefinition
    {
        std::string id;
        std::string source;
        SizeCategory category;
    };

    class CarryingCapacitySize final
    {
    public:
        explicit CarryingCapacitySize(CarryingCapacitySizeDefinition definition);

    private:
        friend class CarryingCapacity;

        std::string id_;
        std::string source_;
        SizeCategory category_;
    };

    struct CarryingCapacityMultiplierDefinition
    {
        std::string id;
        std::string source;
        std::string stackingGroup;
        int numerator;
        int denominator;
    };

    class CarryingCapacityMultiplier final
    {
    public:
        explicit CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition definition);

    private:
        friend class CarryingCapacity;

        std::string id_;
        std::string source_;
        std::string stackingGroup_;
        int numerator_;
        int denominator_;
    };

    class CarryingCapacity final
    {
    public:
        explicit CarryingCapacity(ResourceManager &resourceManager);

        CarryingCapacity(const CarryingCapacity &) = delete;
        CarryingCapacity &operator=(const CarryingCapacity &) = delete;
        CarryingCapacity(CarryingCapacity &&) = delete;
        CarryingCapacity &operator=(CarryingCapacity &&) = delete;

        CarryingCapacityView toView();

    private:
        friend class Encumbrance;

        struct ResolvedLoadLimits
        {
            std::int64_t lightGrams;
            std::int64_t mediumGrams;
            std::int64_t heavyGrams;
        };

        ResolvedLoadLimits resolvedLoadLimits();
        void addBodyTypeBase(CarryingBodyTypeBase bodyType);
        void removeBodyTypeBase(std::string_view bodyTypeId);
        void addSize(CarryingCapacitySize size);
        void removeSize(std::string_view sizeId);
        void addMultiplier(CarryingCapacityMultiplier multiplier);
        void removeMultiplier(std::string_view multiplierId);

        ResourceManager &resourceManager_;
        std::optional<CarryingBodyTypeBase> bodyTypeBase_;
        std::optional<CarryingCapacitySize> size_;
        std::map<std::string, CarryingCapacityMultiplier> multipliers_;
    };
}
