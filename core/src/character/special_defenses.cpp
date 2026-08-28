#include "golarion/character/special_defenses.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/special_defenses_view.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::array EnergyResistanceTypes{
        golarion::DamageType::Acid,
        golarion::DamageType::Electricity,
        golarion::DamageType::Cold,
        golarion::DamageType::Fire,
        golarion::DamageType::Sonic,
        golarion::DamageType::PositiveEnergy,
        golarion::DamageType::NegativeEnergy,
        golarion::DamageType::Force
    };

    bool supportsEnergyResistance(golarion::DamageType type)
    {
        return std::ranges::find(EnergyResistanceTypes, type) != EnergyResistanceTypes.end();
    }

    void validateEnergyResistanceType(golarion::DamageType type)
    {
        if (!supportsEnergyResistance(type))
        {
            throw std::invalid_argument("damage type cannot be used for energy resistance");
        }
    }

    void normalizeUniqueValues(std::vector<std::string> &values, std::string_view field)
    {
        for (std::string &value : values)
        {
            value = golarion::normalize(value);
        }
        std::ranges::sort(values);
        if (std::ranges::adjacent_find(values) != values.end())
        {
            throw std::invalid_argument(std::string(field) + " must not contain duplicates");
        }
    }

    void normalizeDamageReductionBypass(golarion::DamageReductionBypass &bypass)
    {
        for (golarion::DamageReductionBypassAlternative &alternative : bypass.anyOf)
        {
            if (alternative.allOf.empty())
            {
                throw std::invalid_argument("damage reduction bypass alternatives must not be empty");
            }
            std::ranges::sort(alternative.allOf);
            if (std::ranges::adjacent_find(alternative.allOf) != alternative.allOf.end())
            {
                throw std::invalid_argument("damage reduction bypass alternatives must not contain duplicate traits");
            }
        }

        std::ranges::sort(bypass.anyOf, [](const golarion::DamageReductionBypassAlternative &left, const golarion::DamageReductionBypassAlternative &right)
        {
            return left.allOf < right.allOf;
        });
        if (std::ranges::adjacent_find(bypass.anyOf, {}, &golarion::DamageReductionBypassAlternative::allOf) != bypass.anyOf.end())
        {
            throw std::invalid_argument("damage reduction bypass must not contain duplicate alternatives");
        }
    }

    int checkedEnergyResistanceValue(long long value)
    {
        if (value < 0 || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("energy resistance value is out of range");
        }
        return static_cast<int>(value);
    }

    int checkedDamageReductionValue(long long value)
    {
        if (value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("damage reduction value is out of range");
        }
        return static_cast<int>(std::max(0LL, value));
    }

    int checkedSpellResistanceValue(long long value)
    {
        if (value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("spell resistance value is out of range");
        }
        return static_cast<int>(std::max(0LL, value));
    }

    int checkedRecoveryRate(long long value, std::string_view resourceName)
    {
        if (value < 0 || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument(std::string(resourceName) + " value is out of range");
        }
        return static_cast<int>(value);
    }

    bool strongerSignedAdjustment(int candidate, int selected, std::string_view resourceName)
    {
        if (candidate == 0)
        {
            return false;
        }
        if (selected == 0)
        {
            return true;
        }
        if ((candidate < 0) != (selected < 0))
        {
            throw std::invalid_argument(std::string(resourceName) + " adjustments in one stacking group must not have opposite signs");
        }
        return candidate > 0 ? candidate > selected : candidate < selected;
    }

    std::vector<std::string> commonSortedValues(const std::vector<std::string> &left, const std::vector<std::string> &right)
    {
        std::vector<std::string> common;
        std::ranges::set_intersection(left, right, std::back_inserter(common));
        return common;
    }

    bool damageReductionsCanStack(const golarion::DamageReductionGrantView &left, const golarion::DamageReductionGrantView &right)
    {
        return !commonSortedValues(left.stacksWithTags, right.tags).empty() || !commonSortedValues(right.stacksWithTags, left.tags).empty();
    }

    std::vector<std::vector<std::size_t>> maximalDamageReductionCombinations(const std::vector<golarion::DamageReductionGrantView> &grants)
    {
        std::vector<std::vector<bool>> compatible(grants.size(), std::vector<bool>(grants.size(), false));
        for (std::size_t left = 0; left < grants.size(); ++left)
        {
            for (std::size_t right = left + 1; right < grants.size(); ++right)
            {
                compatible[left][right] = damageReductionsCanStack(grants[left], grants[right]);
                compatible[right][left] = compatible[left][right];
            }
        }

        const auto compatibleWith = [&compatible](const std::vector<std::size_t> &vertices, std::size_t selected)
        {
            std::vector<std::size_t> matches;
            std::ranges::copy_if(vertices, std::back_inserter(matches), [&compatible, selected](std::size_t candidate)
            {
                return compatible[selected][candidate];
            });
            return matches;
        };

        std::vector<std::vector<std::size_t>> combinations;
        const auto findCliques = [&compatibleWith, &combinations](auto &&self, std::vector<std::size_t> current, std::vector<std::size_t> candidates, std::vector<std::size_t> excluded) -> void
        {
            if (candidates.empty() && excluded.empty())
            {
                if (current.size() > 1)
                {
                    combinations.push_back(std::move(current));
                }
                return;
            }

            while (!candidates.empty())
            {
                const std::size_t selected = candidates.front();
                std::vector<std::size_t> extended = current;
                extended.push_back(selected);
                self(self, std::move(extended), compatibleWith(candidates, selected), compatibleWith(excluded, selected));
                candidates.erase(candidates.begin());
                excluded.push_back(selected);
            }
        };

        std::vector<std::size_t> candidates(grants.size());
        std::iota(candidates.begin(), candidates.end(), std::size_t{0});
        findCliques(findCliques, {}, std::move(candidates), {});
        std::ranges::sort(combinations);
        return combinations;
    }
}

namespace golarion
{
    std::string_view displayName(DamageReductionBypassTrait trait)
    {
        switch (trait)
        {
            case DamageReductionBypassTrait::Bludgeoning:
                return "Contundente";
            case DamageReductionBypassTrait::Piercing:
                return "Perforante";
            case DamageReductionBypassTrait::Slashing:
                return "Tagliente";
            case DamageReductionBypassTrait::Magic:
                return "Magia";
            case DamageReductionBypassTrait::Epic:
                return "Epico";
            case DamageReductionBypassTrait::Adamantine:
                return "Adamantio";
            case DamageReductionBypassTrait::Silver:
                return "Argento";
            case DamageReductionBypassTrait::ColdIron:
                return "Ferro freddo";
            case DamageReductionBypassTrait::Good:
                return "Bene";
            case DamageReductionBypassTrait::Evil:
                return "Male";
            case DamageReductionBypassTrait::Lawful:
                return "Legale";
            case DamageReductionBypassTrait::Chaotic:
                return "Caotico";
        }

        throw std::invalid_argument("unknown damage reduction bypass trait");
    }

    Immunity::Immunity(ImmunityDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetId_(normalize(definition.targetId)),
          name_(normalize(definition.name)),
          applicability_(std::move(definition.applicability))
    {
        if (applicability_.has_value())
        {
            applicability_ = normalize(*applicability_);
        }
    }

    SpellResistance::SpellResistance(SpellResistanceDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          expression_(normalize(definition.expression)),
          applicability_(std::move(definition.applicability)),
          canBeLowered_(definition.canBeLowered),
          tags_(std::move(definition.tags))
    {
        if (applicability_.has_value())
        {
            applicability_ = normalize(*applicability_);
        }
        normalizeUniqueValues(tags_, "spell resistance tags");
    }

    SpellResistanceSelector::SpellResistanceSelector(SpellResistanceSelectorDefinition definition)
        : grantId_(std::move(definition.grantId)),
          anyTags_(std::move(definition.anyTags)),
          excludedGrantIds_(std::move(definition.excludedGrantIds))
    {
        if (grantId_.has_value())
        {
            grantId_ = normalize(*grantId_);
        }
        normalizeUniqueValues(anyTags_, "spell resistance selector tags");
        normalizeUniqueValues(excludedGrantIds_, "spell resistance selector excluded grant IDs");
        if (grantId_.has_value() && std::ranges::binary_search(excludedGrantIds_, *grantId_))
        {
            throw std::invalid_argument("spell resistance selector cannot exclude its exact grant");
        }
    }

    bool SpellResistanceSelector::matches(const SpellResistance &spellResistance) const
    {
        if (grantId_.has_value() && *grantId_ != spellResistance.id_)
        {
            return false;
        }
        if (std::ranges::binary_search(excludedGrantIds_, spellResistance.id_))
        {
            return false;
        }
        return anyTags_.empty() || std::ranges::any_of(anyTags_, [&spellResistance](const std::string &tag)
        {
            return std::ranges::binary_search(spellResistance.tags_, tag);
        });
    }

    SpellResistanceAdjustment::SpellResistanceAdjustment(SpellResistanceAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          selector_(std::move(definition.selector)),
          expression_(normalize(definition.expression)),
          stackingGroup_(normalize(definition.stackingGroup)),
          applicability_(std::move(definition.applicability))
    {
        if (applicability_.has_value())
        {
            applicability_ = normalize(*applicability_);
        }
    }

    FastHealing::FastHealing(FastHealingDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          expression_(normalize(definition.expression)),
          applicability_(std::move(definition.applicability)),
          stackingGroup_(normalize(definition.stackingGroup)),
          tags_(std::move(definition.tags))
    {
        if (applicability_.has_value())
        {
            applicability_ = normalize(*applicability_);
        }
        normalizeUniqueValues(tags_, "fast healing tags");
    }

    Regeneration::Regeneration(RegenerationDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          expression_(normalize(definition.expression)),
          interruption_(std::move(definition.interruption)),
          applicability_(std::move(definition.applicability)),
          tags_(std::move(definition.tags))
    {
        if (interruption_.has_value())
        {
            interruption_ = normalize(*interruption_);
        }
        if (applicability_.has_value())
        {
            applicability_ = normalize(*applicability_);
        }
        normalizeUniqueValues(tags_, "regeneration tags");
    }

    EnergyResistance::EnergyResistance(EnergyResistanceDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          energy_(definition.energy),
          expression_(normalize(definition.expression)),
          tags_(std::move(definition.tags))
    {
        validateEnergyResistanceType(energy_);
        normalizeUniqueValues(tags_, "energy resistance tags");
    }

    EnergyResistanceSelector::EnergyResistanceSelector(EnergyResistanceSelectorDefinition definition)
        : energy_(definition.energy),
          grantId_(std::move(definition.grantId)),
          anyTags_(std::move(definition.anyTags)),
          excludedGrantIds_(std::move(definition.excludedGrantIds))
    {
        if (energy_.has_value())
        {
            validateEnergyResistanceType(*energy_);
        }
        if (grantId_.has_value())
        {
            grantId_ = normalize(*grantId_);
        }
        normalizeUniqueValues(anyTags_, "energy resistance selector tags");
        normalizeUniqueValues(excludedGrantIds_, "energy resistance selector excluded grant IDs");
        if (grantId_.has_value() && std::ranges::binary_search(excludedGrantIds_, *grantId_))
        {
            throw std::invalid_argument("energy resistance selector cannot exclude its exact grant");
        }
    }

    bool EnergyResistanceSelector::matches(const EnergyResistance &resistance) const
    {
        if (energy_.has_value() && *energy_ != resistance.energy_)
        {
            return false;
        }
        if (grantId_.has_value() && *grantId_ != resistance.id_)
        {
            return false;
        }
        if (std::ranges::binary_search(excludedGrantIds_, resistance.id_))
        {
            return false;
        }
        return anyTags_.empty() || std::ranges::any_of(anyTags_, [&resistance](const std::string &tag)
        {
            return std::ranges::binary_search(resistance.tags_, tag);
        });
    }

    EnergyResistanceAdjustment::EnergyResistanceAdjustment(EnergyResistanceAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          selector_(std::move(definition.selector)),
          expression_(normalize(definition.expression)),
          stackingGroup_(normalize(definition.stackingGroup))
    {
    }

    DamageReduction::DamageReduction(DamageReductionDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          expression_(normalize(definition.expression)),
          bypass_(std::move(definition.bypass)),
          applicability_(std::move(definition.applicability)),
          tags_(std::move(definition.tags)),
          stacksWithTags_(std::move(definition.stacksWithTags))
    {
        normalizeDamageReductionBypass(bypass_);
        if (applicability_.has_value())
        {
            applicability_ = normalize(*applicability_);
        }
        normalizeUniqueValues(tags_, "damage reduction tags");
        normalizeUniqueValues(stacksWithTags_, "damage reduction stacking tags");
    }

    DamageReductionSelector::DamageReductionSelector(DamageReductionSelectorDefinition definition)
        : grantId_(std::move(definition.grantId)),
          anyTags_(std::move(definition.anyTags)),
          excludedGrantIds_(std::move(definition.excludedGrantIds))
    {
        if (grantId_.has_value())
        {
            grantId_ = normalize(*grantId_);
        }
        normalizeUniqueValues(anyTags_, "damage reduction selector tags");
        normalizeUniqueValues(excludedGrantIds_, "damage reduction selector excluded grant IDs");
        if (grantId_.has_value() && std::ranges::binary_search(excludedGrantIds_, *grantId_))
        {
            throw std::invalid_argument("damage reduction selector cannot exclude its exact grant");
        }
    }

    bool DamageReductionSelector::matches(const DamageReduction &damageReduction) const
    {
        if (grantId_.has_value() && *grantId_ != damageReduction.id_)
        {
            return false;
        }
        if (std::ranges::binary_search(excludedGrantIds_, damageReduction.id_))
        {
            return false;
        }
        return anyTags_.empty() || std::ranges::any_of(anyTags_, [&damageReduction](const std::string &tag)
        {
            return std::ranges::binary_search(damageReduction.tags_, tag);
        });
    }

    DamageReductionAdjustment::DamageReductionAdjustment(DamageReductionAdjustmentDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          selector_(std::move(definition.selector)),
          expression_(normalize(definition.expression)),
          stackingGroup_(normalize(definition.stackingGroup))
    {
    }

    SpecialDefenses::SpecialDefenses(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerCollectionResource<EnergyResistance>(EnergyResistanceGrantsResource, [this](EnergyResistance resistance)
        {
            addEnergyResistance(std::move(resistance));
        }, [this](std::string_view resistanceId)
        {
            removeEnergyResistance(resistanceId);
        });
        resourceManager_.registerCollectionResource<EnergyResistanceAdjustment>(EnergyResistanceAdjustmentsResource, [this](EnergyResistanceAdjustment adjustment)
        {
            addEnergyResistanceAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeEnergyResistanceAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<DamageReduction>(DamageReductionGrantsResource, [this](DamageReduction damageReduction)
        {
            addDamageReduction(std::move(damageReduction));
        }, [this](std::string_view damageReductionId)
        {
            removeDamageReduction(damageReductionId);
        });
        resourceManager_.registerCollectionResource<DamageReductionAdjustment>(DamageReductionAdjustmentsResource, [this](DamageReductionAdjustment adjustment)
        {
            addDamageReductionAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeDamageReductionAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<Immunity>(ImmunityGrantsResource, [this](Immunity immunity)
        {
            addImmunity(std::move(immunity));
        }, [this](std::string_view immunityId)
        {
            removeImmunity(immunityId);
        });
        resourceManager_.registerCollectionResource<SpellResistance>(SpellResistanceGrantsResource, [this](SpellResistance spellResistance)
        {
            addSpellResistance(std::move(spellResistance));
        }, [this](std::string_view spellResistanceId)
        {
            removeSpellResistance(spellResistanceId);
        });
        resourceManager_.registerCollectionResource<SpellResistanceAdjustment>(SpellResistanceAdjustmentsResource, [this](SpellResistanceAdjustment adjustment)
        {
            addSpellResistanceAdjustment(std::move(adjustment));
        }, [this](std::string_view adjustmentId)
        {
            removeSpellResistanceAdjustment(adjustmentId);
        });
        resourceManager_.registerCollectionResource<FastHealing>(FastHealingGrantsResource, [this](FastHealing fastHealing)
        {
            addFastHealing(std::move(fastHealing));
        }, [this](std::string_view fastHealingId)
        {
            removeFastHealing(fastHealingId);
        });
        resourceManager_.registerCollectionResource<Regeneration>(RegenerationGrantsResource, [this](Regeneration regeneration)
        {
            addRegeneration(std::move(regeneration));
        }, [this](std::string_view regenerationId)
        {
            removeRegeneration(regenerationId);
        });
    }

    SpecialDefensesView SpecialDefenses::toView()
    {
        std::map<std::string, int> adjustmentValues;
        const auto adjustmentValue = [this, &adjustmentValues](const EnergyResistanceAdjustment &adjustment)
        {
            const auto resolved = adjustmentValues.find(adjustment.id_);
            if (resolved != adjustmentValues.end())
            {
                return resolved->second;
            }
            const int value = resourceManager_.evaluateExpression(adjustment.expression_);
            if (value < 0)
            {
                throw std::invalid_argument("energy resistance adjustment must not resolve to a negative value: " + adjustment.id_);
            }
            adjustmentValues.emplace(adjustment.id_, value);
            return value;
        };

        std::vector<EnergyResistanceView> resistanceViews;
        for (const DamageType energy : EnergyResistanceTypes)
        {
            std::vector<EnergyResistanceGrantView> grantViews;
            for (const auto &[resistanceId, resistance] : energyResistances_)
            {
                if (resistance.energy_ != energy)
                {
                    continue;
                }

                const int baseValue = resourceManager_.evaluateExpression(resistance.expression_);
                if (baseValue < 0)
                {
                    throw std::invalid_argument("energy resistance grant must not resolve to a negative value: " + resistanceId);
                }

                std::map<std::string, const EnergyResistanceAdjustment *> selectedAdjustments;
                for (const auto &[adjustmentId, adjustment] : energyResistanceAdjustments_)
                {
                    static_cast<void>(adjustmentId);
                    if (!adjustment.selector_.matches(resistance))
                    {
                        continue;
                    }

                    auto [selected, inserted] = selectedAdjustments.emplace(adjustment.stackingGroup_, &adjustment);
                    if (!inserted && adjustmentValue(adjustment) > adjustmentValue(*selected->second))
                    {
                        selected->second = &adjustment;
                    }
                }

                long long effectiveValue = baseValue;
                std::vector<EnergyResistanceAdjustmentApplicationView> adjustmentViews;
                for (const auto &[adjustmentId, adjustment] : energyResistanceAdjustments_)
                {
                    if (!adjustment.selector_.matches(resistance))
                    {
                        continue;
                    }

                    const int resolvedIncrease = adjustmentValue(adjustment);
                    const EnergyResistanceAdjustment *selected = selectedAdjustments.at(adjustment.stackingGroup_);
                    const bool applied = selected == &adjustment;
                    std::optional<std::string> notAppliedReason;
                    if (applied)
                    {
                        effectiveValue += resolvedIncrease;
                    }
                    else if (resolvedIncrease == adjustmentValue(*selected))
                    {
                        notAppliedReason = "Un'altra istanza dello stesso incremento è già applicata";
                    }
                    else
                    {
                        notAppliedReason = "Superato da un incremento maggiore dello stesso gruppo";
                    }

                    adjustmentViews.push_back(EnergyResistanceAdjustmentApplicationView{
                        .id = adjustmentId,
                        .source = adjustment.source_,
                        .expression = adjustment.expression_,
                        .resolvedIncrease = resolvedIncrease,
                        .stackingGroup = adjustment.stackingGroup_,
                        .applied = applied,
                        .notAppliedReason = std::move(notAppliedReason)
                    });
                }

                grantViews.push_back(EnergyResistanceGrantView{
                    .id = resistanceId,
                    .source = resistance.source_,
                    .energy = energy,
                    .expression = resistance.expression_,
                    .baseValue = baseValue,
                    .effectiveValue = checkedEnergyResistanceValue(effectiveValue),
                    .tags = resistance.tags_,
                    .adjustments = std::move(adjustmentViews),
                    .determinesEffectiveValue = false,
                    .notAppliedReason = std::nullopt
                });
            }

            if (grantViews.empty())
            {
                continue;
            }

            const int effectiveValue = std::ranges::max(grantViews, {}, &EnergyResistanceGrantView::effectiveValue).effectiveValue;
            for (EnergyResistanceGrantView &grantView : grantViews)
            {
                grantView.determinesEffectiveValue = grantView.effectiveValue == effectiveValue;
                if (!grantView.determinesEffectiveValue)
                {
                    grantView.notAppliedReason = "Superata da una resistenza più alta alla stessa energia";
                }
            }
            resistanceViews.push_back(EnergyResistanceView{
                .energy = energy,
                .effectiveValue = effectiveValue,
                .grants = std::move(grantViews)
            });
        }

        std::vector<DamageReductionGrantView> damageReductionViews;
        damageReductionViews.reserve(damageReductions_.size());
        std::map<std::string, int> damageReductionAdjustmentValues;
        const auto damageReductionAdjustmentValue = [this, &damageReductionAdjustmentValues](const DamageReductionAdjustment &adjustment)
        {
            const auto resolved = damageReductionAdjustmentValues.find(adjustment.id_);
            if (resolved != damageReductionAdjustmentValues.end())
            {
                return resolved->second;
            }
            const int value = resourceManager_.evaluateExpression(adjustment.expression_);
            damageReductionAdjustmentValues.emplace(adjustment.id_, value);
            return value;
        };

        for (const auto &[damageReductionId, damageReduction] : damageReductions_)
        {
            const int baseValue = resourceManager_.evaluateExpression(damageReduction.expression_);
            if (baseValue < 0)
            {
                throw std::invalid_argument("damage reduction grant must not resolve to a negative value: " + damageReductionId);
            }

            std::map<std::string, const DamageReductionAdjustment *> selectedAdjustments;
            for (const auto &[adjustmentId, adjustment] : damageReductionAdjustments_)
            {
                static_cast<void>(adjustmentId);
                if (!adjustment.selector_.matches(damageReduction))
                {
                    continue;
                }

                auto [selected, inserted] = selectedAdjustments.emplace(adjustment.stackingGroup_, &adjustment);
                if (!inserted && strongerSignedAdjustment(damageReductionAdjustmentValue(adjustment), damageReductionAdjustmentValue(*selected->second), "damage reduction"))
                {
                    selected->second = &adjustment;
                }
            }

            long long effectiveValue = baseValue;
            std::vector<DamageReductionAdjustmentApplicationView> adjustmentViews;
            for (const auto &[adjustmentId, adjustment] : damageReductionAdjustments_)
            {
                if (!adjustment.selector_.matches(damageReduction))
                {
                    continue;
                }

                const int resolvedDelta = damageReductionAdjustmentValue(adjustment);
                const DamageReductionAdjustment *selected = selectedAdjustments.at(adjustment.stackingGroup_);
                const bool applied = selected == &adjustment;
                std::optional<std::string> notAppliedReason;
                if (applied)
                {
                    effectiveValue += resolvedDelta;
                }
                else if (resolvedDelta == damageReductionAdjustmentValue(*selected))
                {
                    notAppliedReason = "Un'altra istanza della stessa modifica è già applicata";
                }
                else if (resolvedDelta > 0)
                {
                    notAppliedReason = "Superata da un incremento maggiore dello stesso gruppo";
                }
                else if (resolvedDelta < 0)
                {
                    notAppliedReason = "Superata da una riduzione maggiore dello stesso gruppo";
                }
                else
                {
                    notAppliedReason = "Superata da una modifica non nulla dello stesso gruppo";
                }

                adjustmentViews.push_back(DamageReductionAdjustmentApplicationView{
                    .id = adjustmentId,
                    .source = adjustment.source_,
                    .expression = adjustment.expression_,
                    .resolvedDelta = resolvedDelta,
                    .stackingGroup = adjustment.stackingGroup_,
                    .applied = applied,
                    .notAppliedReason = std::move(notAppliedReason)
                });
            }

            damageReductionViews.push_back(DamageReductionGrantView{
                .id = damageReductionId,
                .source = damageReduction.source_,
                .expression = damageReduction.expression_,
                .baseValue = baseValue,
                .effectiveValue = checkedDamageReductionValue(effectiveValue),
                .bypass = damageReduction.bypass_,
                .applicability = damageReduction.applicability_,
                .tags = damageReduction.tags_,
                .stacksWithTags = damageReduction.stacksWithTags_,
                .stackableWithGrantIds = {},
                .adjustments = std::move(adjustmentViews)
            });
        }

        for (std::size_t left = 0; left < damageReductionViews.size(); ++left)
        {
            for (std::size_t right = left + 1; right < damageReductionViews.size(); ++right)
            {
                if (!damageReductionsCanStack(damageReductionViews[left], damageReductionViews[right]))
                {
                    continue;
                }
                damageReductionViews[left].stackableWithGrantIds.push_back(damageReductionViews[right].id);
                damageReductionViews[right].stackableWithGrantIds.push_back(damageReductionViews[left].id);
            }
        }

        std::vector<DamageReductionCombinationView> damageReductionCombinations;
        for (const std::vector<std::size_t> &combination : maximalDamageReductionCombinations(damageReductionViews))
        {
            long long maximumValue = 0;
            const DamageReductionGrantView &firstGrant = damageReductionViews[combination.front()];
            bool canFlatten = true;
            std::vector<std::string> grantIds;
            std::vector<DamageReductionStackingAuthorizationView> authorizations;
            grantIds.reserve(combination.size());

            for (const std::size_t grantIndex : combination)
            {
                const DamageReductionGrantView &grant = damageReductionViews[grantIndex];
                grantIds.push_back(grant.id);
                maximumValue += grant.effectiveValue;
                canFlatten = canFlatten && grant.bypass == firstGrant.bypass && grant.applicability == firstGrant.applicability;
            }

            for (std::size_t leftIndex = 0; leftIndex < combination.size(); ++leftIndex)
            {
                for (std::size_t rightIndex = leftIndex + 1; rightIndex < combination.size(); ++rightIndex)
                {
                    const DamageReductionGrantView &left = damageReductionViews[combination[leftIndex]];
                    const DamageReductionGrantView &right = damageReductionViews[combination[rightIndex]];
                    std::vector<std::string> leftMatches = commonSortedValues(left.stacksWithTags, right.tags);
                    if (!leftMatches.empty())
                    {
                        authorizations.push_back(DamageReductionStackingAuthorizationView{
                            .declaringGrantId = left.id,
                            .compatibleGrantId = right.id,
                            .matchedTags = std::move(leftMatches)
                        });
                    }
                    std::vector<std::string> rightMatches = commonSortedValues(right.stacksWithTags, left.tags);
                    if (!rightMatches.empty())
                    {
                        authorizations.push_back(DamageReductionStackingAuthorizationView{
                            .declaringGrantId = right.id,
                            .compatibleGrantId = left.id,
                            .matchedTags = std::move(rightMatches)
                        });
                    }
                }
            }

            damageReductionCombinations.push_back(DamageReductionCombinationView{
                .grantIds = std::move(grantIds),
                .maximumValue = checkedDamageReductionValue(maximumValue),
                .flattenedBypass = canFlatten ? std::optional(firstGrant.bypass) : std::nullopt,
                .flattenedApplicability = canFlatten ? firstGrant.applicability : std::nullopt,
                .authorizations = std::move(authorizations)
            });
        }

        std::map<std::string, ImmunityView> immunityViewsByTarget;
        for (const auto &[immunityId, immunity] : immunities_)
        {
            auto [target, inserted] = immunityViewsByTarget.emplace(immunity.targetId_, ImmunityView{
                .targetId = immunity.targetId_,
                .name = immunity.name_,
                .grants = {}
            });
            static_cast<void>(inserted);
            target->second.grants.push_back(ImmunityGrantView{
                .id = immunityId,
                .source = immunity.source_,
                .applicability = immunity.applicability_
            });
        }

        std::vector<ImmunityView> immunityViews;
        immunityViews.reserve(immunityViewsByTarget.size());
        for (auto &[targetId, immunityView] : immunityViewsByTarget)
        {
            static_cast<void>(targetId);
            immunityViews.push_back(std::move(immunityView));
        }

        std::vector<std::optional<std::string>> spellResistanceContexts;
        if (std::ranges::any_of(spellResistances_, [](const auto &entry)
        {
            return !entry.second.applicability_.has_value();
        }))
        {
            spellResistanceContexts.push_back(std::nullopt);
        }
        for (const auto &[spellResistanceId, spellResistance] : spellResistances_)
        {
            static_cast<void>(spellResistanceId);
            if (spellResistance.applicability_.has_value())
            {
                spellResistanceContexts.push_back(spellResistance.applicability_);
            }
        }
        for (const auto &[adjustmentId, adjustment] : spellResistanceAdjustments_)
        {
            static_cast<void>(adjustmentId);
            if (!adjustment.applicability_.has_value())
            {
                continue;
            }
            const bool hasApplicableGrant = std::ranges::any_of(spellResistances_, [&adjustment](const auto &entry)
            {
                const SpellResistance &spellResistance = entry.second;
                return adjustment.selector_.matches(spellResistance) && (!spellResistance.applicability_.has_value() || spellResistance.applicability_ == adjustment.applicability_);
            });
            if (hasApplicableGrant)
            {
                spellResistanceContexts.push_back(adjustment.applicability_);
            }
        }
        std::ranges::sort(spellResistanceContexts);
        const auto uniqueSpellResistanceContexts = std::ranges::unique(spellResistanceContexts);
        spellResistanceContexts.erase(uniqueSpellResistanceContexts.begin(), uniqueSpellResistanceContexts.end());

        std::map<std::string, int> spellResistanceBaseValues;
        const auto spellResistanceBaseValue = [this, &spellResistanceBaseValues](const SpellResistance &spellResistance)
        {
            const auto resolved = spellResistanceBaseValues.find(spellResistance.id_);
            if (resolved != spellResistanceBaseValues.end())
            {
                return resolved->second;
            }
            const int value = resourceManager_.evaluateExpression(spellResistance.expression_);
            if (value < 0)
            {
                throw std::invalid_argument("spell resistance grant must not resolve to a negative value: " + spellResistance.id_);
            }
            spellResistanceBaseValues.emplace(spellResistance.id_, value);
            return value;
        };
        std::map<std::string, int> spellResistanceAdjustmentValues;
        const auto spellResistanceAdjustmentValue = [this, &spellResistanceAdjustmentValues](const SpellResistanceAdjustment &adjustment)
        {
            const auto resolved = spellResistanceAdjustmentValues.find(adjustment.id_);
            if (resolved != spellResistanceAdjustmentValues.end())
            {
                return resolved->second;
            }
            const int value = resourceManager_.evaluateExpression(adjustment.expression_);
            spellResistanceAdjustmentValues.emplace(adjustment.id_, value);
            return value;
        };

        std::vector<SpellResistanceContextView> spellResistanceViews;
        spellResistanceViews.reserve(spellResistanceContexts.size());
        for (const std::optional<std::string> &context : spellResistanceContexts)
        {
            std::vector<SpellResistanceGrantView> grantViews;
            for (const auto &[spellResistanceId, spellResistance] : spellResistances_)
            {
                if (spellResistance.applicability_.has_value() && spellResistance.applicability_ != context)
                {
                    continue;
                }

                std::map<std::string, const SpellResistanceAdjustment *> selectedAdjustments;
                for (const auto &[adjustmentId, adjustment] : spellResistanceAdjustments_)
                {
                    static_cast<void>(adjustmentId);
                    if (!adjustment.selector_.matches(spellResistance) || (adjustment.applicability_.has_value() && adjustment.applicability_ != context))
                    {
                        continue;
                    }
                    auto [selected, inserted] = selectedAdjustments.emplace(adjustment.stackingGroup_, &adjustment);
                    if (!inserted && strongerSignedAdjustment(spellResistanceAdjustmentValue(adjustment), spellResistanceAdjustmentValue(*selected->second), "spell resistance"))
                    {
                        selected->second = &adjustment;
                    }
                }

                long long effectiveValue = spellResistanceBaseValue(spellResistance);
                std::vector<SpellResistanceAdjustmentApplicationView> adjustmentViews;
                for (const auto &[adjustmentId, adjustment] : spellResistanceAdjustments_)
                {
                    if (!adjustment.selector_.matches(spellResistance) || (adjustment.applicability_.has_value() && adjustment.applicability_ != context))
                    {
                        continue;
                    }

                    const int resolvedDelta = spellResistanceAdjustmentValue(adjustment);
                    const SpellResistanceAdjustment *selected = selectedAdjustments.at(adjustment.stackingGroup_);
                    const bool applied = selected == &adjustment;
                    std::optional<std::string> notAppliedReason;
                    if (applied)
                    {
                        effectiveValue += resolvedDelta;
                    }
                    else if (resolvedDelta == spellResistanceAdjustmentValue(*selected))
                    {
                        notAppliedReason = "Un'altra istanza della stessa modifica è già applicata";
                    }
                    else if (resolvedDelta > 0)
                    {
                        notAppliedReason = "Superata da un incremento maggiore dello stesso gruppo";
                    }
                    else if (resolvedDelta < 0)
                    {
                        notAppliedReason = "Superata da una riduzione maggiore dello stesso gruppo";
                    }
                    else
                    {
                        notAppliedReason = "Superata da una modifica non nulla dello stesso gruppo";
                    }

                    adjustmentViews.push_back(SpellResistanceAdjustmentApplicationView{
                        .id = adjustmentId,
                        .source = adjustment.source_,
                        .expression = adjustment.expression_,
                        .resolvedDelta = resolvedDelta,
                        .stackingGroup = adjustment.stackingGroup_,
                        .applicability = adjustment.applicability_,
                        .applied = applied,
                        .notAppliedReason = std::move(notAppliedReason)
                    });
                }

                grantViews.push_back(SpellResistanceGrantView{
                    .id = spellResistanceId,
                    .source = spellResistance.source_,
                    .expression = spellResistance.expression_,
                    .baseValue = spellResistanceBaseValue(spellResistance),
                    .effectiveValue = checkedSpellResistanceValue(effectiveValue),
                    .applicability = spellResistance.applicability_,
                    .canBeLowered = spellResistance.canBeLowered_,
                    .tags = spellResistance.tags_,
                    .adjustments = std::move(adjustmentViews),
                    .determinesEffectiveValue = false,
                    .notAppliedReason = std::nullopt
                });
            }

            const int effectiveValue = std::ranges::max(grantViews, {}, &SpellResistanceGrantView::effectiveValue).effectiveValue;
            for (SpellResistanceGrantView &grantView : grantViews)
            {
                grantView.determinesEffectiveValue = grantView.effectiveValue == effectiveValue;
                if (!grantView.determinesEffectiveValue)
                {
                    grantView.notAppliedReason = "Superata da una resistenza agli incantesimi più alta nello stesso contesto";
                }
            }
            spellResistanceViews.push_back(SpellResistanceContextView{
                .applicability = context,
                .effectiveValue = effectiveValue,
                .grants = std::move(grantViews)
            });
        }

        std::vector<std::optional<std::string>> fastHealingContexts;
        if (std::ranges::any_of(fastHealingGrants_, [](const auto &entry)
        {
            return !entry.second.applicability_.has_value();
        }))
        {
            fastHealingContexts.push_back(std::nullopt);
        }
        for (const auto &[fastHealingId, fastHealing] : fastHealingGrants_)
        {
            static_cast<void>(fastHealingId);
            if (fastHealing.applicability_.has_value())
            {
                fastHealingContexts.push_back(fastHealing.applicability_);
            }
        }
        std::ranges::sort(fastHealingContexts);
        const auto uniqueFastHealingContexts = std::ranges::unique(fastHealingContexts);
        fastHealingContexts.erase(uniqueFastHealingContexts.begin(), uniqueFastHealingContexts.end());

        std::map<std::string, int> fastHealingValues;
        const auto fastHealingValue = [this, &fastHealingValues](const FastHealing &fastHealing)
        {
            const auto resolved = fastHealingValues.find(fastHealing.id_);
            if (resolved != fastHealingValues.end())
            {
                return resolved->second;
            }
            const int value = resourceManager_.evaluateExpression(fastHealing.expression_);
            const int checkedValue = checkedRecoveryRate(value, "fast healing");
            fastHealingValues.emplace(fastHealing.id_, checkedValue);
            return checkedValue;
        };

        std::vector<FastHealingContextView> fastHealingViews;
        fastHealingViews.reserve(fastHealingContexts.size());
        for (const std::optional<std::string> &context : fastHealingContexts)
        {
            std::map<std::string, int> effectiveValuesByGroup;
            for (const auto &[fastHealingId, fastHealing] : fastHealingGrants_)
            {
                static_cast<void>(fastHealingId);
                if (fastHealing.applicability_.has_value() && fastHealing.applicability_ != context)
                {
                    continue;
                }
                auto [group, inserted] = effectiveValuesByGroup.emplace(fastHealing.stackingGroup_, fastHealingValue(fastHealing));
                if (!inserted)
                {
                    group->second = std::max(group->second, fastHealingValue(fastHealing));
                }
            }

            long long effectiveValue = 0;
            for (const auto &[stackingGroup, groupValue] : effectiveValuesByGroup)
            {
                static_cast<void>(stackingGroup);
                effectiveValue += groupValue;
            }

            std::vector<FastHealingGrantView> grantViews;
            for (const auto &[fastHealingId, fastHealing] : fastHealingGrants_)
            {
                if (fastHealing.applicability_.has_value() && fastHealing.applicability_ != context)
                {
                    continue;
                }
                const int resolvedValue = fastHealingValue(fastHealing);
                const bool contributes = resolvedValue == effectiveValuesByGroup.at(fastHealing.stackingGroup_);
                grantViews.push_back(FastHealingGrantView{
                    .id = fastHealingId,
                    .source = fastHealing.source_,
                    .expression = fastHealing.expression_,
                    .resolvedValue = resolvedValue,
                    .applicability = fastHealing.applicability_,
                    .stackingGroup = fastHealing.stackingGroup_,
                    .tags = fastHealing.tags_,
                    .contributesToEffectiveValue = contributes,
                    .notAppliedReason = contributes ? std::nullopt : std::optional<std::string>("Superata da una guarigione rapida più alta dello stesso gruppo")
                });
            }

            fastHealingViews.push_back(FastHealingContextView{
                .applicability = context,
                .effectiveValue = checkedRecoveryRate(effectiveValue, "fast healing"),
                .grants = std::move(grantViews)
            });
        }

        std::vector<RegenerationGrantView> regenerationViews;
        regenerationViews.reserve(regenerationGrants_.size());
        for (const auto &[regenerationId, regeneration] : regenerationGrants_)
        {
            regenerationViews.push_back(RegenerationGrantView{
                .id = regenerationId,
                .source = regeneration.source_,
                .expression = regeneration.expression_,
                .resolvedValue = checkedRecoveryRate(resourceManager_.evaluateExpression(regeneration.expression_), "regeneration"),
                .interruption = regeneration.interruption_,
                .applicability = regeneration.applicability_,
                .tags = regeneration.tags_
            });
        }

        return SpecialDefensesView{
            .energyResistances = std::move(resistanceViews),
            .damageReductions = std::move(damageReductionViews),
            .damageReductionCombinations = std::move(damageReductionCombinations),
            .immunities = std::move(immunityViews),
            .spellResistances = std::move(spellResistanceViews),
            .fastHealing = std::move(fastHealingViews),
            .regeneration = std::move(regenerationViews)
        };
    }

    void SpecialDefenses::addEnergyResistance(EnergyResistance resistance)
    {
        const std::string id = resistance.id_;
        if (!energyResistances_.emplace(id, std::move(resistance)).second)
        {
            throw std::invalid_argument("energy resistance grant is already registered: " + id);
        }
    }

    void SpecialDefenses::removeEnergyResistance(std::string_view resistanceId)
    {
        const std::string id = normalize(resistanceId);
        const auto dependentAdjustment = std::ranges::find_if(energyResistanceAdjustments_, [&id](const auto &entry)
        {
            return entry.second.selector_.grantId_ == id;
        });
        if (dependentAdjustment != energyResistanceAdjustments_.end())
        {
            throw std::invalid_argument("energy resistance grant is targeted by adjustment: " + dependentAdjustment->first);
        }
        if (energyResistances_.erase(id) == 0)
        {
            throw std::invalid_argument("energy resistance grant is not registered: " + id);
        }
    }

    void SpecialDefenses::addEnergyResistanceAdjustment(EnergyResistanceAdjustment adjustment)
    {
        const std::string id = adjustment.id_;
        if (energyResistanceAdjustments_.contains(id))
        {
            throw std::invalid_argument("energy resistance adjustment is already registered: " + id);
        }
        if (adjustment.selector_.grantId_.has_value())
        {
            const auto resistance = energyResistances_.find(*adjustment.selector_.grantId_);
            if (resistance == energyResistances_.end())
            {
                throw std::invalid_argument("energy resistance adjustment grant is not registered: " + *adjustment.selector_.grantId_);
            }
            if (!adjustment.selector_.matches(resistance->second))
            {
                throw std::invalid_argument("energy resistance adjustment selector does not match its exact grant");
            }
        }
        energyResistanceAdjustments_.emplace(id, std::move(adjustment));
    }

    void SpecialDefenses::removeEnergyResistanceAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (energyResistanceAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("energy resistance adjustment is not registered: " + id);
        }
    }

    void SpecialDefenses::addDamageReduction(DamageReduction damageReduction)
    {
        const std::string id = damageReduction.id_;
        if (!damageReductions_.emplace(id, std::move(damageReduction)).second)
        {
            throw std::invalid_argument("damage reduction grant is already registered: " + id);
        }
    }

    void SpecialDefenses::removeDamageReduction(std::string_view damageReductionId)
    {
        const std::string id = normalize(damageReductionId);
        const auto dependentAdjustment = std::ranges::find_if(damageReductionAdjustments_, [&id](const auto &entry)
        {
            return entry.second.selector_.grantId_ == id;
        });
        if (dependentAdjustment != damageReductionAdjustments_.end())
        {
            throw std::invalid_argument("damage reduction grant is targeted by adjustment: " + dependentAdjustment->first);
        }
        if (damageReductions_.erase(id) == 0)
        {
            throw std::invalid_argument("damage reduction grant is not registered: " + id);
        }
    }

    void SpecialDefenses::addDamageReductionAdjustment(DamageReductionAdjustment adjustment)
    {
        const std::string id = adjustment.id_;
        if (damageReductionAdjustments_.contains(id))
        {
            throw std::invalid_argument("damage reduction adjustment is already registered: " + id);
        }
        if (adjustment.selector_.grantId_.has_value())
        {
            const auto damageReduction = damageReductions_.find(*adjustment.selector_.grantId_);
            if (damageReduction == damageReductions_.end())
            {
                throw std::invalid_argument("damage reduction adjustment grant is not registered: " + *adjustment.selector_.grantId_);
            }
            if (!adjustment.selector_.matches(damageReduction->second))
            {
                throw std::invalid_argument("damage reduction adjustment selector does not match its exact grant");
            }
        }
        damageReductionAdjustments_.emplace(id, std::move(adjustment));
    }

    void SpecialDefenses::removeDamageReductionAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (damageReductionAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("damage reduction adjustment is not registered: " + id);
        }
    }

    void SpecialDefenses::addImmunity(Immunity immunity)
    {
        const std::string id = immunity.id_;
        if (immunities_.contains(id))
        {
            throw std::invalid_argument("immunity grant is already registered: " + id);
        }
        const auto inconsistentName = std::ranges::find_if(immunities_, [&immunity](const auto &entry)
        {
            return entry.second.targetId_ == immunity.targetId_ && entry.second.name_ != immunity.name_;
        });
        if (inconsistentName != immunities_.end())
        {
            throw std::invalid_argument("immunity target uses a different display name: " + immunity.targetId_);
        }
        immunities_.emplace(id, std::move(immunity));
    }

    void SpecialDefenses::removeImmunity(std::string_view immunityId)
    {
        const std::string id = normalize(immunityId);
        if (immunities_.erase(id) == 0)
        {
            throw std::invalid_argument("immunity grant is not registered: " + id);
        }
    }

    void SpecialDefenses::addSpellResistance(SpellResistance spellResistance)
    {
        const std::string id = spellResistance.id_;
        if (!spellResistances_.emplace(id, std::move(spellResistance)).second)
        {
            throw std::invalid_argument("spell resistance grant is already registered: " + id);
        }
    }

    void SpecialDefenses::removeSpellResistance(std::string_view spellResistanceId)
    {
        const std::string id = normalize(spellResistanceId);
        const auto dependentAdjustment = std::ranges::find_if(spellResistanceAdjustments_, [&id](const auto &entry)
        {
            return entry.second.selector_.grantId_ == id;
        });
        if (dependentAdjustment != spellResistanceAdjustments_.end())
        {
            throw std::invalid_argument("spell resistance grant is targeted by adjustment: " + dependentAdjustment->first);
        }
        if (spellResistances_.erase(id) == 0)
        {
            throw std::invalid_argument("spell resistance grant is not registered: " + id);
        }
    }

    void SpecialDefenses::addSpellResistanceAdjustment(SpellResistanceAdjustment adjustment)
    {
        const std::string id = adjustment.id_;
        if (spellResistanceAdjustments_.contains(id))
        {
            throw std::invalid_argument("spell resistance adjustment is already registered: " + id);
        }
        if (adjustment.selector_.grantId_.has_value())
        {
            const auto spellResistance = spellResistances_.find(*adjustment.selector_.grantId_);
            if (spellResistance == spellResistances_.end())
            {
                throw std::invalid_argument("spell resistance adjustment grant is not registered: " + *adjustment.selector_.grantId_);
            }
            if (!adjustment.selector_.matches(spellResistance->second))
            {
                throw std::invalid_argument("spell resistance adjustment selector does not match its exact grant");
            }
        }
        spellResistanceAdjustments_.emplace(id, std::move(adjustment));
    }

    void SpecialDefenses::removeSpellResistanceAdjustment(std::string_view adjustmentId)
    {
        const std::string id = normalize(adjustmentId);
        if (spellResistanceAdjustments_.erase(id) == 0)
        {
            throw std::invalid_argument("spell resistance adjustment is not registered: " + id);
        }
    }

    void SpecialDefenses::addFastHealing(FastHealing fastHealing)
    {
        const std::string id = fastHealing.id_;
        if (!fastHealingGrants_.emplace(id, std::move(fastHealing)).second)
        {
            throw std::invalid_argument("fast healing grant is already registered: " + id);
        }
    }

    void SpecialDefenses::removeFastHealing(std::string_view fastHealingId)
    {
        const std::string id = normalize(fastHealingId);
        if (fastHealingGrants_.erase(id) == 0)
        {
            throw std::invalid_argument("fast healing grant is not registered: " + id);
        }
    }

    void SpecialDefenses::addRegeneration(Regeneration regeneration)
    {
        const std::string id = regeneration.id_;
        if (!regenerationGrants_.emplace(id, std::move(regeneration)).second)
        {
            throw std::invalid_argument("regeneration grant is already registered: " + id);
        }
    }

    void SpecialDefenses::removeRegeneration(std::string_view regenerationId)
    {
        const std::string id = normalize(regenerationId);
        if (regenerationGrants_.erase(id) == 0)
        {
            throw std::invalid_argument("regeneration grant is not registered: " + id);
        }
    }
}
