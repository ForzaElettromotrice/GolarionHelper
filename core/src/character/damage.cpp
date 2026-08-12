#include "golarion/character/damage.hpp"

#include "golarion/util/string_utils.hpp"
#include "golarion/view/damage_view.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    template<typename Value>
    bool containsDuplicates(std::vector<Value> values)
    {
        std::ranges::sort(values);
        return std::ranges::adjacent_find(values) != values.end();
    }

    struct DiceProgressionEntry
    {
        int diceCount;
        int dieSize;
        int smallerDiceCount;
        int smallerDieSize;
        int largerDiceCount;
        int largerDieSize;
    };

    constexpr std::array DiceProgression{
        DiceProgressionEntry{.diceCount = 1, .dieSize = 2, .smallerDiceCount = 1, .smallerDieSize = 1, .largerDiceCount = 1, .largerDieSize = 3},
        DiceProgressionEntry{.diceCount = 1, .dieSize = 3, .smallerDiceCount = 1, .smallerDieSize = 1, .largerDiceCount = 1, .largerDieSize = 4},
        DiceProgressionEntry{.diceCount = 1, .dieSize = 4, .smallerDiceCount = 1, .smallerDieSize = 2, .largerDiceCount = 1, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 1, .dieSize = 6, .smallerDiceCount = 1, .smallerDieSize = 3, .largerDiceCount = 1, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 1, .dieSize = 8, .smallerDiceCount = 1, .smallerDieSize = 4, .largerDiceCount = 2, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 1, .dieSize = 10, .smallerDiceCount = 1, .smallerDieSize = 6, .largerDiceCount = 2, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 1, .dieSize = 12, .smallerDiceCount = 1, .smallerDieSize = 8, .largerDiceCount = 3, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 2, .dieSize = 4, .smallerDiceCount = 1, .smallerDieSize = 4, .largerDiceCount = 2, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 2, .dieSize = 6, .smallerDiceCount = 1, .smallerDieSize = 8, .largerDiceCount = 3, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 2, .dieSize = 8, .smallerDiceCount = 1, .smallerDieSize = 10, .largerDiceCount = 3, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 2, .dieSize = 10, .smallerDiceCount = 2, .smallerDieSize = 6, .largerDiceCount = 4, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 3, .dieSize = 6, .smallerDiceCount = 2, .smallerDieSize = 6, .largerDiceCount = 4, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 3, .dieSize = 8, .smallerDiceCount = 2, .smallerDieSize = 8, .largerDiceCount = 4, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 4, .dieSize = 6, .smallerDiceCount = 3, .smallerDieSize = 6, .largerDiceCount = 6, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 4, .dieSize = 8, .smallerDiceCount = 3, .smallerDieSize = 8, .largerDiceCount = 6, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 6, .dieSize = 6, .smallerDiceCount = 4, .smallerDieSize = 6, .largerDiceCount = 8, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 6, .dieSize = 8, .smallerDiceCount = 4, .smallerDieSize = 8, .largerDiceCount = 8, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 8, .dieSize = 6, .smallerDiceCount = 6, .smallerDieSize = 6, .largerDiceCount = 12, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 8, .dieSize = 8, .smallerDiceCount = 6, .smallerDieSize = 8, .largerDiceCount = 12, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 12, .dieSize = 6, .smallerDiceCount = 8, .smallerDieSize = 6, .largerDiceCount = 16, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 12, .dieSize = 8, .smallerDiceCount = 8, .smallerDieSize = 8, .largerDiceCount = 16, .largerDieSize = 8},
        DiceProgressionEntry{.diceCount = 16, .dieSize = 6, .smallerDiceCount = 12, .smallerDieSize = 6, .largerDiceCount = 24, .largerDieSize = 6},
        DiceProgressionEntry{.diceCount = 16, .dieSize = 8, .smallerDiceCount = 12, .smallerDieSize = 8, .largerDiceCount = 24, .largerDieSize = 8}
    };
}

namespace golarion
{
    std::string_view displayName(DamageType type)
    {
        switch (type)
        {
            case DamageType::Bludgeoning:
                return "Contundente";
            case DamageType::Piercing:
                return "Perforante";
            case DamageType::Slashing:
                return "Tagliente";
            case DamageType::Acid:
                return "Acido";
            case DamageType::Cold:
                return "Freddo";
            case DamageType::Electricity:
                return "Elettricità";
            case DamageType::Fire:
                return "Fuoco";
            case DamageType::Sonic:
                return "Sonoro";
            case DamageType::Force:
                return "Forza";
            case DamageType::PositiveEnergy:
                return "Energia positiva";
            case DamageType::NegativeEnergy:
                return "Energia negativa";
            case DamageType::Untyped:
                return "Senza tipo";
        }

        throw std::invalid_argument("unknown damage type");
    }

    std::string_view displayName(DamageTypeMode mode)
    {
        switch (mode)
        {
            case DamageTypeMode::All:
                return "Tutti";
            case DamageTypeMode::Choice:
                return "A scelta";
        }

        throw std::invalid_argument("unknown damage type mode");
    }

    std::string_view displayName(DamageCriticalRule rule)
    {
        switch (rule)
        {
            case DamageCriticalRule::Multiplied:
                return "Moltiplicato nel critico";
            case DamageCriticalRule::NotMultiplied:
                return "Non moltiplicato nel critico";
            case DamageCriticalRule::CriticalOnly:
                return "Solo nel critico";
        }

        throw std::invalid_argument("unknown damage critical rule");
    }

    std::string_view displayName(DamageTrait trait)
    {
        switch (trait)
        {
            case DamageTrait::Precision:
                return "Precisione";
            case DamageTrait::Bleed:
                return "Sanguinamento";
            case DamageTrait::NonLethal:
                return "Non letale";
        }

        throw std::invalid_argument("unknown damage trait");
    }

    std::string_view displayName(DamageComponentRole role)
    {
        switch (role)
        {
            case DamageComponentRole::Base:
                return "Base";
            case DamageComponentRole::Additional:
                return "Aggiuntivo";
        }

        throw std::invalid_argument("unknown damage component role");
    }

    std::string_view displayName(DamageDiceAdjustmentType type)
    {
        switch (type)
        {
            case DamageDiceAdjustmentType::ProgressionSteps:
                return "Passi nella progressione dei dadi";
            case DamageDiceAdjustmentType::DiceCountMultiplier:
                return "Moltiplicatore del numero di dadi";
            case DamageDiceAdjustmentType::Set:
                return "Dadi impostati";
        }

        throw std::invalid_argument("unknown damage dice adjustment type");
    }

    DamageDice::DamageDice(DamageDiceDefinition definition)
        : diceCount_(definition.diceCount),
          dieSize_(definition.dieSize)
    {
        if (diceCount_ <= 0)
        {
            throw std::invalid_argument("damage dice count must be greater than zero");
        }
        if (dieSize_ <= 0)
        {
            throw std::invalid_argument("damage die size must be greater than zero");
        }
    }

    std::string DamageDice::toString() const
    {
        if (dieSize_ == 1)
        {
            return std::to_string(diceCount_);
        }
        return std::to_string(diceCount_) + "d" + std::to_string(dieSize_);
    }

    DamageDiceView DamageDice::toView() const
    {
        return DamageDiceView{
            .diceCount = diceCount_,
            .dieSize = dieSize_,
            .expression = toString()
        };
    }

    DamageDice DamageDice::adjustedByProgression(int steps) const
    {
        int diceCount = diceCount_;
        int dieSize = dieSize_;
        const int direction = steps < 0 ? -1 : 1;
        const long long stepCount = steps < 0 ? -static_cast<long long>(steps) : steps;
        for (long long step = 0; step < stepCount; ++step)
        {
            const auto entry = std::ranges::find_if(DiceProgression, [diceCount, dieSize](const DiceProgressionEntry &candidate)
            {
                return candidate.diceCount == diceCount && candidate.dieSize == dieSize;
            });
            if (entry == DiceProgression.end())
            {
                throw std::invalid_argument("damage dice are not supported by the size progression: " + std::to_string(diceCount) + "d" + std::to_string(dieSize));
            }
            diceCount = direction > 0 ? entry->largerDiceCount : entry->smallerDiceCount;
            dieSize = direction > 0 ? entry->largerDieSize : entry->smallerDieSize;
        }
        return DamageDice(DamageDiceDefinition{.diceCount = diceCount, .dieSize = dieSize});
    }

    DamageDice DamageDice::multiplied(int multiplier) const
    {
        if (multiplier < 1)
        {
            throw std::invalid_argument("damage dice count multiplier must be at least 1");
        }
        const long long diceCount = static_cast<long long>(diceCount_) * multiplier;
        if (diceCount > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("damage dice count is out of range");
        }
        return DamageDice(DamageDiceDefinition{.diceCount = static_cast<int>(diceCount), .dieSize = dieSize_});
    }

    DamageComponent::DamageComponent(DamageComponentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          role_(definition.role),
          dice_(std::move(definition.dice)),
          types_(std::move(definition.types)),
          typeMode_(definition.typeMode),
          criticalRule_(definition.criticalRule),
          traits_(std::move(definition.traits))
    {
        if (types_.empty())
        {
            throw std::invalid_argument("damage component must have at least one damage type");
        }
        if (containsDuplicates(types_))
        {
            throw std::invalid_argument("damage component types must not contain duplicates");
        }
        if (typeMode_ == DamageTypeMode::Choice && types_.size() < 2)
        {
            throw std::invalid_argument("choice damage components must have at least two damage types");
        }
        if (types_.size() > 1 && std::ranges::find(types_, DamageType::Untyped) != types_.end())
        {
            throw std::invalid_argument("untyped damage must not be combined with another damage type");
        }
        if (containsDuplicates(traits_))
        {
            throw std::invalid_argument("damage component traits must not contain duplicates");
        }
        if (criticalRule_ == DamageCriticalRule::Multiplied && std::ranges::find(traits_, DamageTrait::Precision) != traits_.end())
        {
            throw std::invalid_argument("precision damage must not be multiplied on a critical hit");
        }
    }
}
