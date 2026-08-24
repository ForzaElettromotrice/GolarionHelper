#include "golarion/character/carrying_capacity.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/carrying_capacity_view.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace
{
    struct LoadLimits
    {
        std::int64_t lightGrams;
        std::int64_t mediumGrams;
        std::int64_t heavyGrams;
    };

    struct CapacityScale
    {
        std::int64_t numerator;
        std::int64_t denominator;
    };

    constexpr std::array<LoadLimits, 30> LoadLimitsByStrength{
        LoadLimits{.lightGrams = 0, .mediumGrams = 0, .heavyGrams = 0},
        LoadLimits{.lightGrams = 1500, .mediumGrams = 3000, .heavyGrams = 5000},
        LoadLimits{.lightGrams = 3000, .mediumGrams = 6500, .heavyGrams = 10000},
        LoadLimits{.lightGrams = 5000, .mediumGrams = 10000, .heavyGrams = 15000},
        LoadLimits{.lightGrams = 6500, .mediumGrams = 13000, .heavyGrams = 20000},
        LoadLimits{.lightGrams = 8000, .mediumGrams = 16500, .heavyGrams = 25000},
        LoadLimits{.lightGrams = 10000, .mediumGrams = 20000, .heavyGrams = 30000},
        LoadLimits{.lightGrams = 11500, .mediumGrams = 23000, .heavyGrams = 35000},
        LoadLimits{.lightGrams = 13000, .mediumGrams = 26500, .heavyGrams = 40000},
        LoadLimits{.lightGrams = 15000, .mediumGrams = 30000, .heavyGrams = 45000},
        LoadLimits{.lightGrams = 16500, .mediumGrams = 33000, .heavyGrams = 50000},
        LoadLimits{.lightGrams = 19000, .mediumGrams = 38000, .heavyGrams = 57500},
        LoadLimits{.lightGrams = 21500, .mediumGrams = 43000, .heavyGrams = 65000},
        LoadLimits{.lightGrams = 25000, .mediumGrams = 50000, .heavyGrams = 75000},
        LoadLimits{.lightGrams = 29000, .mediumGrams = 58000, .heavyGrams = 87500},
        LoadLimits{.lightGrams = 33000, .mediumGrams = 66500, .heavyGrams = 100000},
        LoadLimits{.lightGrams = 38000, .mediumGrams = 76500, .heavyGrams = 115000},
        LoadLimits{.lightGrams = 43000, .mediumGrams = 86500, .heavyGrams = 130000},
        LoadLimits{.lightGrams = 50000, .mediumGrams = 100000, .heavyGrams = 150000},
        LoadLimits{.lightGrams = 58000, .mediumGrams = 116500, .heavyGrams = 175000},
        LoadLimits{.lightGrams = 66500, .mediumGrams = 133000, .heavyGrams = 200000},
        LoadLimits{.lightGrams = 76500, .mediumGrams = 153000, .heavyGrams = 230000},
        LoadLimits{.lightGrams = 86500, .mediumGrams = 173000, .heavyGrams = 260000},
        LoadLimits{.lightGrams = 100000, .mediumGrams = 200000, .heavyGrams = 300000},
        LoadLimits{.lightGrams = 116500, .mediumGrams = 233000, .heavyGrams = 350000},
        LoadLimits{.lightGrams = 133000, .mediumGrams = 266500, .heavyGrams = 400000},
        LoadLimits{.lightGrams = 153000, .mediumGrams = 306500, .heavyGrams = 460000},
        LoadLimits{.lightGrams = 173000, .mediumGrams = 346500, .heavyGrams = 520000},
        LoadLimits{.lightGrams = 200000, .mediumGrams = 400000, .heavyGrams = 600000},
        LoadLimits{.lightGrams = 233000, .mediumGrams = 466500, .heavyGrams = 700000}
    };

    std::int64_t checkedMultiply(std::int64_t value, std::int64_t multiplier)
    {
        if (value != 0 && multiplier > std::numeric_limits<std::int64_t>::max() / value)
        {
            throw std::invalid_argument("carrying capacity is out of range");
        }
        return value * multiplier;
    }

    std::int64_t scaleCapacity(std::int64_t value, CapacityScale scale)
    {
        const std::int64_t divisor = std::gcd(value, scale.denominator);
        const std::int64_t reducedValue = value / divisor;
        const std::int64_t reducedDenominator = scale.denominator / divisor;
        return checkedMultiply(reducedValue, scale.numerator) / reducedDenominator;
    }

    CapacityScale multiplyScales(CapacityScale left, CapacityScale right)
    {
        const std::int64_t firstDivisor = std::gcd(left.numerator, right.denominator);
        const std::int64_t secondDivisor = std::gcd(right.numerator, left.denominator);
        return CapacityScale{
            .numerator = checkedMultiply(left.numerator / firstDivisor, right.numerator / secondDivisor),
            .denominator = checkedMultiply(left.denominator / secondDivisor, right.denominator / firstDivisor)
        };
    }

    CapacityScale capacityScale(golarion::SizeCategory category, golarion::CarryingBodyType bodyType)
    {
        if (bodyType == golarion::CarryingBodyType::Biped)
        {
            switch (category)
            {
                case golarion::SizeCategory::Fine:
                    return CapacityScale{.numerator = 1, .denominator = 8};
                case golarion::SizeCategory::Diminutive:
                    return CapacityScale{.numerator = 1, .denominator = 4};
                case golarion::SizeCategory::Tiny:
                    return CapacityScale{.numerator = 1, .denominator = 2};
                case golarion::SizeCategory::Small:
                    return CapacityScale{.numerator = 3, .denominator = 4};
                case golarion::SizeCategory::Medium:
                    return CapacityScale{.numerator = 1, .denominator = 1};
                case golarion::SizeCategory::Large:
                    return CapacityScale{.numerator = 2, .denominator = 1};
                case golarion::SizeCategory::Huge:
                    return CapacityScale{.numerator = 4, .denominator = 1};
                case golarion::SizeCategory::Gargantuan:
                    return CapacityScale{.numerator = 8, .denominator = 1};
                case golarion::SizeCategory::Colossal:
                    return CapacityScale{.numerator = 16, .denominator = 1};
            }
        }

        switch (category)
        {
            case golarion::SizeCategory::Fine:
                return CapacityScale{.numerator = 1, .denominator = 4};
            case golarion::SizeCategory::Diminutive:
                return CapacityScale{.numerator = 1, .denominator = 2};
            case golarion::SizeCategory::Tiny:
                return CapacityScale{.numerator = 3, .denominator = 4};
            case golarion::SizeCategory::Small:
                return CapacityScale{.numerator = 1, .denominator = 1};
            case golarion::SizeCategory::Medium:
                return CapacityScale{.numerator = 3, .denominator = 2};
            case golarion::SizeCategory::Large:
                return CapacityScale{.numerator = 3, .denominator = 1};
            case golarion::SizeCategory::Huge:
                return CapacityScale{.numerator = 6, .denominator = 1};
            case golarion::SizeCategory::Gargantuan:
                return CapacityScale{.numerator = 12, .denominator = 1};
            case golarion::SizeCategory::Colossal:
                return CapacityScale{.numerator = 24, .denominator = 1};
        }

        throw std::invalid_argument("unknown size category");
    }

    LoadLimits capacityForStrength(int strength)
    {
        if (strength <= 0)
        {
            return LoadLimits{.lightGrams = 0, .mediumGrams = 0, .heavyGrams = 0};
        }

        int normalizedStrength = strength;
        std::int64_t multiplier = 1;
        while (normalizedStrength > 29)
        {
            normalizedStrength -= 10;
            multiplier = checkedMultiply(multiplier, 4);
        }

        const LoadLimits base = LoadLimitsByStrength[static_cast<std::size_t>(normalizedStrength)];
        return LoadLimits{
            .lightGrams = checkedMultiply(base.lightGrams, multiplier),
            .mediumGrams = checkedMultiply(base.mediumGrams, multiplier),
            .heavyGrams = checkedMultiply(base.heavyGrams, multiplier)
        };
    }

    LoadLimits loadLimits(int strength, golarion::SizeCategory category, golarion::CarryingBodyType bodyType)
    {
        const LoadLimits base = capacityForStrength(strength);
        const CapacityScale scale = capacityScale(category, bodyType);
        return LoadLimits{
            .lightGrams = scaleCapacity(base.lightGrams, scale),
            .mediumGrams = scaleCapacity(base.mediumGrams, scale),
            .heavyGrams = scaleCapacity(base.heavyGrams, scale)
        };
    }
}

namespace golarion
{
    std::string_view displayName(CarryingBodyType type)
    {
        switch (type)
        {
            case CarryingBodyType::Biped:
                return "Bipede";
            case CarryingBodyType::Quadruped:
                return "Quadrupede";
        }

        throw std::invalid_argument("unknown carrying body type");
    }

    CarryingBodyTypeBase::CarryingBodyTypeBase(CarryingBodyTypeBaseDefinition definition)
        : id_(normalize(definition.id)), source_(normalize(definition.source)), type_(definition.type)
    {
        static_cast<void>(displayName(type_));
    }

    CarryingCapacitySize::CarryingCapacitySize(CarryingCapacitySizeDefinition definition)
        : id_(normalize(definition.id)), source_(normalize(definition.source)), category_(definition.category)
    {
        static_cast<void>(displayName(category_));
    }

    CarryingCapacityMultiplier::CarryingCapacityMultiplier(CarryingCapacityMultiplierDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          stackingGroup_(normalize(definition.stackingGroup)),
          numerator_(definition.numerator),
          denominator_(definition.denominator)
    {
        if (numerator_ <= 0 || denominator_ <= 0)
        {
            throw std::invalid_argument("carrying capacity multiplier terms must be greater than 0");
        }
        const int divisor = std::gcd(numerator_, denominator_);
        numerator_ /= divisor;
        denominator_ /= divisor;
    }

    CarryingCapacity::CarryingCapacity(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerEnhanceableResource(CarryingCapacityStrengthResource);
        resourceManager_.registerCollectionResource<CarryingBodyTypeBase>(CarryingCapacityBodyTypeResource, [this](CarryingBodyTypeBase bodyType)
        {
            addBodyTypeBase(std::move(bodyType));
        }, [this](std::string_view bodyTypeId)
        {
            removeBodyTypeBase(bodyTypeId);
        });
        resourceManager_.registerCollectionResource<CarryingCapacitySize>(CarryingCapacitySizeResource, [this](CarryingCapacitySize size)
        {
            addSize(std::move(size));
        }, [this](std::string_view sizeId)
        {
            removeSize(sizeId);
        });
        resourceManager_.registerCollectionResource<CarryingCapacityMultiplier>(CarryingCapacityMultipliersResource, [this](CarryingCapacityMultiplier multiplier)
        {
            addMultiplier(std::move(multiplier));
        }, [this](std::string_view multiplierId)
        {
            removeMultiplier(multiplierId);
        });
    }

    CarryingCapacityView CarryingCapacity::toView()
    {
        const int strength = resourceManager_.targetValue("str");
        ModifierSetView strengthModifiers = resourceManager_.modifierSetView(CarryingCapacityStrengthResource);
        const long long modifiedStrength = static_cast<long long>(strength) + strengthModifiers.total;
        if (modifiedStrength > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("effective carrying Strength is out of range");
        }
        const int effectiveStrength = static_cast<int>(std::max(modifiedStrength, 0LL));
        const CarryingBodyType bodyType = bodyTypeBase_.has_value() ? bodyTypeBase_->type_ : CarryingBodyType::Biped;
        const SizeCategory size = size_.has_value() ? size_->category_ : SizeCategory::Medium;
        const LoadLimits baseLimits = loadLimits(effectiveStrength, size, bodyType);
        const auto multiplierIsGreater = [](const CarryingCapacityMultiplier &left, const CarryingCapacityMultiplier &right)
        {
            return static_cast<std::int64_t>(left.numerator_) * right.denominator_ > static_cast<std::int64_t>(right.numerator_) * left.denominator_;
        };
        const auto multipliersAreEqual = [](const CarryingCapacityMultiplier &left, const CarryingCapacityMultiplier &right)
        {
            return static_cast<std::int64_t>(left.numerator_) * right.denominator_ == static_cast<std::int64_t>(right.numerator_) * left.denominator_;
        };
        std::map<std::string, const CarryingCapacityMultiplier *> selectedMultipliers;
        for (const auto &[id, multiplier] : multipliers_)
        {
            static_cast<void>(id);
            auto [selected, inserted] = selectedMultipliers.emplace(multiplier.stackingGroup_, &multiplier);
            if (!inserted && multiplierIsGreater(multiplier, *selected->second))
            {
                selected->second = &multiplier;
            }
        }

        CapacityScale combinedMultiplier{.numerator = 1, .denominator = 1};
        std::vector<CarryingCapacityMultiplierView> multiplierViews;
        multiplierViews.reserve(multipliers_.size());
        for (const auto &[id, multiplier] : multipliers_)
        {
            const CarryingCapacityMultiplier *selected = selectedMultipliers.at(multiplier.stackingGroup_);
            const bool applied = selected == &multiplier;
            std::optional<std::string> notAppliedReason;
            if (applied)
            {
                combinedMultiplier = multiplyScales(combinedMultiplier, CapacityScale{
                    .numerator = multiplier.numerator_,
                    .denominator = multiplier.denominator_
                });
            }
            else if (multipliersAreEqual(multiplier, *selected))
            {
                notAppliedReason = "Un'altra istanza dello stesso effetto è già applicata";
            }
            else
            {
                notAppliedReason = "Superato da un moltiplicatore più alto dello stesso effetto";
            }
            multiplierViews.push_back(CarryingCapacityMultiplierView{
                .id = id,
                .source = multiplier.source_,
                .stackingGroup = multiplier.stackingGroup_,
                .numerator = multiplier.numerator_,
                .denominator = multiplier.denominator_,
                .applied = applied,
                .notAppliedReason = std::move(notAppliedReason)
            });
        }
        const LoadLimits limits{
            .lightGrams = scaleCapacity(baseLimits.lightGrams, combinedMultiplier),
            .mediumGrams = scaleCapacity(baseLimits.mediumGrams, combinedMultiplier),
            .heavyGrams = scaleCapacity(baseLimits.heavyGrams, combinedMultiplier)
        };
        std::optional<CarryingBodyTypeBaseView> bodyTypeBaseView;
        if (bodyTypeBase_.has_value())
        {
            bodyTypeBaseView = CarryingBodyTypeBaseView{
                .id = bodyTypeBase_->id_,
                .source = bodyTypeBase_->source_,
                .type = bodyTypeBase_->type_
            };
        }

        return CarryingCapacityView{
            .strength = strength,
            .strengthModifiers = std::move(strengthModifiers),
            .effectiveStrength = effectiveStrength,
            .bodyType = bodyType,
            .bodyTypeBase = std::move(bodyTypeBaseView),
            .size = size,
            .baseLightLoadMaxGrams = baseLimits.lightGrams,
            .baseMediumLoadMaxGrams = baseLimits.mediumGrams,
            .baseHeavyLoadMaxGrams = baseLimits.heavyGrams,
            .combinedMultiplierNumerator = combinedMultiplier.numerator,
            .combinedMultiplierDenominator = combinedMultiplier.denominator,
            .multipliers = std::move(multiplierViews),
            .lightLoadMaxGrams = limits.lightGrams,
            .mediumLoadMaxGrams = limits.mediumGrams,
            .heavyLoadMaxGrams = limits.heavyGrams,
            .liftFromGroundMaxGrams = checkedMultiply(limits.heavyGrams, 2),
            .pushOrDragMaxGrams = checkedMultiply(limits.heavyGrams, 5)
        };
    }

    CarryingCapacity::ResolvedLoadLimits CarryingCapacity::resolvedLoadLimits()
    {
        const CarryingCapacityView view = toView();
        return ResolvedLoadLimits{
            .lightGrams = view.lightLoadMaxGrams,
            .mediumGrams = view.mediumLoadMaxGrams,
            .heavyGrams = view.heavyLoadMaxGrams
        };
    }

    void CarryingCapacity::addBodyTypeBase(CarryingBodyTypeBase bodyType)
    {
        if (bodyTypeBase_.has_value())
        {
            throw std::invalid_argument("carrying body type base is already registered: " + bodyTypeBase_->id_);
        }
        bodyTypeBase_ = std::move(bodyType);
    }

    void CarryingCapacity::removeBodyTypeBase(std::string_view bodyTypeId)
    {
        const std::string id = normalize(bodyTypeId);
        if (!bodyTypeBase_.has_value() || bodyTypeBase_->id_ != id)
        {
            throw std::invalid_argument("carrying body type base is not registered: " + id);
        }
        bodyTypeBase_.reset();
    }

    void CarryingCapacity::addSize(CarryingCapacitySize size)
    {
        if (size_.has_value())
        {
            throw std::invalid_argument("carrying capacity size is already registered: " + size_->id_);
        }
        size_ = std::move(size);
    }

    void CarryingCapacity::removeSize(std::string_view sizeId)
    {
        const std::string id = normalize(sizeId);
        if (!size_.has_value() || size_->id_ != id)
        {
            throw std::invalid_argument("carrying capacity size is not registered: " + id);
        }
        size_.reset();
    }

    void CarryingCapacity::addMultiplier(CarryingCapacityMultiplier multiplier)
    {
        const std::string id = multiplier.id_;
        if (!multipliers_.emplace(id, std::move(multiplier)).second)
        {
            throw std::invalid_argument("carrying capacity multiplier is already registered: " + id);
        }
    }

    void CarryingCapacity::removeMultiplier(std::string_view multiplierId)
    {
        const std::string id = normalize(multiplierId);
        if (multipliers_.erase(id) == 0)
        {
            throw std::invalid_argument("carrying capacity multiplier is not registered: " + id);
        }
    }

}
