#include "golarion/character/hit_points.hpp"

#include "golarion/data/hit_points_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/hit_points_view.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::string_view MaxHitPointsResource = "hp.max";

    int checkedHitPointValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("hit point value is out of range");
        }
        return static_cast<int>(value);
    }
}

namespace golarion
{
    std::string_view displayName(DamageLethality lethality)
    {
        switch (lethality)
        {
            case DamageLethality::Lethal:
                return "Letale";
            case DamageLethality::NonLethal:
                return "Non letale";
        }
        throw std::invalid_argument("unknown damage type");
    }

    HitPoints::HitPoints(ResourceManager &resourceManager)
        : resourceManager_(resourceManager), baseMax_(0), damageTaken_(0), nonLethal_(0)
    {
        resourceManager_.registerAccumulatedResource(MaxHitPointsResource);
        resourceManager_.registerCollectionResource<TemporaryHitPointGrant>(TemporaryHitPointsResource, [this](TemporaryHitPointGrant grant)
        {
            addTemporary(std::move(grant.id), resourceManager_.evaluateExpression(grant.amountExpression), grant.duration);
        }, [this](std::string_view grantId)
        {
            removeTemporary(grantId);
        });
    }

    void HitPoints::setMax(int value)
    {
        if (value < 0)
        {
            throw std::invalid_argument("maximum hit points must not be negative");
        }
        static_cast<void>(checkedHitPointValue(static_cast<long long>(value) + resourceManager_.contributionTotal(MaxHitPointsResource)));
        baseMax_ = value;
    }

    void HitPoints::setCurrent(int value)
    {
        const int maximum = maxValue();
        if (value > maximum)
        {
            throw std::invalid_argument("current hit points must not exceed maximum hit points");
        }

        const long long damageTaken = static_cast<long long>(maximum) - value;
        if (damageTaken > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("hit point damage is out of range");
        }
        damageTaken_ = static_cast<int>(damageTaken);
    }

    void HitPoints::addTemporary(std::string id, int amount, std::optional<GameDuration> duration)
    {
        temporary_.add(std::move(id), amount, duration);
    }

    void HitPoints::removeTemporary(std::string_view id)
    {
        temporary_.remove(id);
    }

    void HitPoints::advanceTime(GameDuration duration)
    {
        temporary_.advanceTime(duration);
    }

    void HitPoints::setNonLethal(int value)
    {
        if (value < 0)
        {
            throw std::invalid_argument("non-lethal damage must not be negative");
        }
        nonLethal_ = value;
    }

    void HitPoints::heal(int amount)
    {
        if (amount < 0)
        {
            throw std::invalid_argument("healing amount must not be negative");
        }

        damageTaken_ -= std::min(damageTaken_, amount);
        nonLethal_ = std::max(0, nonLethal_ - std::min(nonLethal_, amount));
    }

    void HitPoints::damage(int amount, DamageLethality lethality)
    {
        if (amount < 0)
        {
            throw std::invalid_argument("damage amount must not be negative");
        }

        if (lethality != DamageLethality::Lethal && lethality != DamageLethality::NonLethal)
        {
            throw std::invalid_argument("unknown damage type");
        }

        TemporaryHitPoints newTemporary = temporary_;
        int remainingDamage = newTemporary.absorb(amount);
        int newDamageTaken = damageTaken_;
        int newNonLethal = nonLethal_;

        switch (lethality)
        {
            case DamageLethality::Lethal:
                newDamageTaken = checkedHitPointValue(static_cast<long long>(damageTaken_) + remainingDamage);
                break;
            case DamageLethality::NonLethal:
            {
                const int maximum = maxValue();
                const int nonLethalCapacity = std::max(0, maximum - std::min(nonLethal_, maximum));
                const int appliedNonLethalDamage = std::min(remainingDamage, nonLethalCapacity);
                newNonLethal = checkedHitPointValue(static_cast<long long>(nonLethal_) + appliedNonLethalDamage);
                remainingDamage -= appliedNonLethalDamage;
                newDamageTaken = checkedHitPointValue(static_cast<long long>(damageTaken_) + remainingDamage);
                break;
            }
            default:
                return;
        }

        temporary_ = std::move(newTemporary);
        damageTaken_ = newDamageTaken;
        nonLethal_ = newNonLethal;
    }

    HitPointsView HitPoints::toView()
    {
        ContributionSetView maxContributions = resourceManager_.contributionSetView(MaxHitPointsResource);
        const int maximum = std::max(0, checkedHitPointValue(static_cast<long long>(baseMax_) + maxContributions.total));

        return HitPointsView{
            .baseMax = baseMax_,
            .max = maximum,
            .current = maximum - damageTaken_,
            .temporary = temporary_.toView(),
            .nonLethal = nonLethal_,
            .maxContributions = std::move(maxContributions)
        };
    }

    HitPointsSaveData HitPoints::toSaveData() const
    {
        return HitPointsSaveData{
            .baseMax = baseMax_,
            .damageTaken = damageTaken_,
            .temporary = temporary_.toSaveData(),
            .nonLethal = nonLethal_
        };
    }

    int HitPoints::maxValue()
    {
        return std::max(0, checkedHitPointValue(static_cast<long long>(baseMax_) + resourceManager_.contributionTotal(MaxHitPointsResource)));
    }

    void HitPoints::load(const HitPointsSaveData &data)
    {
        setMax(data.baseMax);
        if (data.damageTaken < 0)
        {
            throw std::invalid_argument("hit point damage must not be negative");
        }
        damageTaken_ = data.damageTaken;
        temporary_.load(data.temporary);
        setNonLethal(data.nonLethal);
    }
}
