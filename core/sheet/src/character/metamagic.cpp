#include "golarion/character/metamagic.hpp"
#include "golarion/view/metamagic_view.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>
#include <set>
#include <algorithm>

namespace
{
    using namespace golarion;

    std::size_t phaseIndex(MetamagicPhase phase)
    {
        switch (phase)
        {
        case MetamagicPhase::Preparation: return 0;
        case MetamagicPhase::Casting: return 1;
        }
        throw std::invalid_argument("invalid metamagic phase");
    }

    void validateType(MetamagicType type)
    {
        switch (type)
        {
        case MetamagicType::Empower: case MetamagicType::Enlarge: case MetamagicType::Extend:
        case MetamagicType::Heighten: case MetamagicType::Maximize: case MetamagicType::Quicken:
        case MetamagicType::Silent: case MetamagicType::Still: case MetamagicType::Widen:
        case MetamagicType::Elemental: case MetamagicType::Reach: case MetamagicType::Intensified:
        case MetamagicType::Selective: return;
        }
        throw std::invalid_argument("invalid metamagic type");
    }

    void validateEnergy(DamageType energy)
    {
        switch (energy)
        {
        case DamageType::Acid: case DamageType::Cold: case DamageType::Electricity: case DamageType::Fire: return;
        default: throw std::invalid_argument("invalid elemental metamagic energy");
        }
    }
}

namespace golarion
{
    PreparedMetamagicData preparationSnapshot(const MetamagicEvaluation &evaluation, CharacterClassType spellListClass, int paidSlots)
    {
        return {evaluation.baseSpellLevel, evaluation.effectiveSpellLevel, evaluation.requiredSlotLevel, evaluation.nominalSlotLevel, spellListClass, paidSlots};
    }

    MetamagicEvaluation restorePreparationSnapshot(std::string_view spellId, const std::vector<MetamagicApplication> &applications, const PreparedMetamagicData &snapshot, CharacterClassType spellListClass)
    {
        characterClassId(snapshot.spellListClass);
        if (snapshot.spellListClass != spellListClass || snapshot.paidSlots < 1)
            throw std::invalid_argument("incompatible saved preparation context");
        if (snapshot.baseSpellLevel < 0 || snapshot.baseSpellLevel > 9)
            throw std::invalid_argument("invalid prepared base spell level");
        int nominal = snapshot.baseSpellLevel, effective = snapshot.baseSpellLevel;
        std::set<MetamagicType> types;
        for (const auto &application : applications)
        {
            if (!types.insert(application.type).second) throw std::invalid_argument("duplicate prepared metamagic type");
            const auto adjustment = standardMetamagicAdjustment(application, snapshot.baseSpellLevel);
            nominal += adjustment.slotLevelIncrease;
            effective += adjustment.effectiveLevelIncrease;
        }
        if (snapshot.nominalSlotLevel != nominal || snapshot.effectiveSpellLevel != effective || effective > 9 ||
            snapshot.requiredSlotLevel < snapshot.baseSpellLevel || snapshot.requiredSlotLevel > nominal || snapshot.requiredSlotLevel > 9)
            throw std::invalid_argument("inconsistent prepared metamagic levels");
        return {normalize(spellId), snapshot.baseSpellLevel, effective, snapshot.requiredSlotLevel, nominal, applications, {}};
    }

    void collectMetamagicPool(MetamagicUsagePools &pools, const MetamagicUsageCost &cost)
    {
        if (!cost.pool || cost.uses <= 0 || normalize(cost.poolId) != cost.pool->id())
            throw std::invalid_argument("metamagic usage pool is unavailable or inconsistent");
        const auto [found, inserted] = pools.emplace(cost.pool->id(), cost.pool);
        if (!inserted && found->second != cost.pool) throw std::invalid_argument("conflicting metamagic pool identities");
    }

    MetamagicUsageCostView metamagicUsageCostView(const MetamagicUsageCost &cost)
    {
        return {cost.poolId, cost.uses, cost.pool ? std::optional<int>(cost.pool->remaining()) : std::nullopt};
    }

    MetamagicUsagePools MetamagicRegistry::usagePools() const
    {
        MetamagicUsagePools pools;
        for (const auto &phase : grants_) for (const auto &[id, grant] : phase)
            if (grant.usageCost && grant.usageCost->pool) collectMetamagicPool(pools, *grant.usageCost);
        return pools;
    }

    std::vector<MetamagicGrantView> MetamagicRegistry::toView() const
    {
        std::vector<MetamagicGrantView> view;
        for (const auto phase : {MetamagicPhase::Preparation, MetamagicPhase::Casting})
            for (const auto &[id, grant] : grants_[phaseIndex(phase)])
                view.push_back({id, grant.source, phase, grant.type, grant.elementalEnergy, grant.slotCost, grant.rodGrade,
                    grant.castingScope, grant.usageCost ? std::optional(metamagicUsageCostView(*grant.usageCost)) : std::nullopt,
                    static_cast<bool>(grant.isApplicable)});
        return view;
    }

    MetamagicUsagePool::MetamagicUsagePool(std::string_view id, int remaining) : id_(normalize(id)), remaining_(0)
    {
        reset(remaining);
    }

    MetamagicUsagePool::MetamagicUsagePool(std::string_view id, std::shared_ptr<UsagePool> pool)
        : id_(normalize(id)), remaining_(0), linkedPool_(std::move(pool))
    {
        if (!linkedPool_) throw std::invalid_argument("linked metamagic usage pool must not be null");
    }

    void MetamagicUsagePool::reset(int remaining)
    {
        if (linkedPool_) throw std::invalid_argument("linked usage pools must be restored by their owner");
        if (remaining < 0) throw std::invalid_argument("remaining metamagic uses must not be negative");
        remaining_ = remaining;
    }

    void MetamagicRegistry::pay(const std::vector<MetamagicUsageCost> &costs)
    {
        std::map<std::shared_ptr<MetamagicUsagePool>, int> totals;
        std::map<std::shared_ptr<UsagePool>, int> linkedTotals;
        std::map<std::string, std::shared_ptr<MetamagicUsagePool>> identities;
        for (const auto &cost : costs)
        {
            if (!cost.pool || cost.uses <= 0 || normalize(cost.poolId) != cost.pool->id_)
                throw std::invalid_argument("metamagic usage pool is unavailable or inconsistent");
            const auto [identity, inserted] = identities.emplace(cost.pool->id_, cost.pool);
            if (!inserted && identity->second != cost.pool) throw std::invalid_argument("conflicting metamagic pool identities");
            auto &total = cost.pool->linkedPool_ ? linkedTotals[cost.pool->linkedPool_] : totals[cost.pool];
            if (cost.uses > cost.pool->remaining() - total) throw std::invalid_argument("insufficient metamagic uses");
            total += cost.uses;
        }
        // No allocation, callbacks or other throwing work after validation.
        for (const auto &[pool, total] : totals) pool->remaining_ -= total;
        for (const auto &[pool, total] : linkedTotals) pool->consume(total);
    }

    MetamagicEvaluation MetamagicRegistry::evaluate(MetamagicPhase phase, const SpellDefinition &spell, int baseSpellLevel, const std::vector<MetamagicApplication> &applications, bool conversion, const MetamagicEvaluation *prepared) const
    {
        phaseIndex(phase);
        if (baseSpellLevel < 0 || baseSpellLevel > 9) throw std::invalid_argument("invalid base spell level");
        if (prepared && (phase != MetamagicPhase::Casting || conversion || prepared->spellId != spell.id || prepared->baseSpellLevel != baseSpellLevel))
            throw std::invalid_argument("invalid prepared metamagic context");
        MetamagicEvaluation result = prepared ? *prepared : MetamagicEvaluation{spell.id, baseSpellLevel, baseSpellLevel, baseSpellLevel, baseSpellLevel, {}, {}};
        result.usageCosts.clear();
        std::set<MetamagicType> types;
        for (const auto &application : result.applications) types.insert(application.type);
        std::optional<int> rodCap;
        for (auto application : applications)
        {
            application.grantId = normalize(application.grantId);
            const auto grant = resolve(phase, application);
            if (!types.insert(application.type).second) throw std::invalid_argument("duplicate metamagic type on spell");
            if (phase == MetamagicPhase::Casting && !conversion && grant.castingScope == CastingMetamagicScope::SpontaneousConversionOnly)
                throw std::invalid_argument("metamagic grant only permits spontaneous conversion");
            bool requiresProviderCheck = false;
            switch (application.type)
            {
            case MetamagicType::Extend:
                if (spell.durationType == SpellDurationType::Instantaneous || spell.durationType == SpellDurationType::Permanent || spell.durationType == SpellDurationType::Concentration)
                    throw std::invalid_argument("spell duration cannot be extended");
                requiresProviderCheck = spell.durationType == SpellDurationType::Other;
                break;
            case MetamagicType::Enlarge:
                if (spell.rangeType != SpellRangeType::Close && spell.rangeType != SpellRangeType::Medium && spell.rangeType != SpellRangeType::Long)
                    throw std::invalid_argument("spell range cannot be enlarged");
                break;
            case MetamagicType::Reach:
            {
                int available = 0;
                if (spell.rangeType == SpellRangeType::Touch) available = 3;
                if (spell.rangeType == SpellRangeType::Close) available = 2;
                if (spell.rangeType == SpellRangeType::Medium) available = 1;
                if (std::get<ReachMetamagicOptions>(application.options).rangeSteps > available)
                    throw std::invalid_argument("reach exceeds supported spell range");
                break;
            }
            case MetamagicType::Silent:
                if (!spell.components.contains(SpellComponentType::Verbal)) throw std::invalid_argument("spell has no verbal component");
                break;
            case MetamagicType::Still:
                if (!spell.components.contains(SpellComponentType::Somatic)) throw std::invalid_argument("spell has no somatic component");
                break;
            case MetamagicType::Selective:
                if (!spell.area || spell.durationType != SpellDurationType::Instantaneous) throw std::invalid_argument("spell cannot be selective");
                break;
            case MetamagicType::Quicken:
                if (spell.castingTimeCategory == SpellCastingTimeCategory::Round || spell.castingTimeCategory == SpellCastingTimeCategory::Special)
                    requiresProviderCheck = true; // The enum alone cannot distinguish one round from several.
                else if (spell.castingTimeCategory != SpellCastingTimeCategory::StandardAction && spell.castingTimeCategory != SpellCastingTimeCategory::FullRoundAction && spell.castingTimeCategory != SpellCastingTimeCategory::SwiftAction && spell.castingTimeCategory != SpellCastingTimeCategory::ImmediateAction)
                    throw std::invalid_argument("spell casting time cannot be quickened");
                break;
            case MetamagicType::Heighten: break;
            case MetamagicType::Empower: case MetamagicType::Maximize: case MetamagicType::Elemental:
            case MetamagicType::Intensified: case MetamagicType::Widen:
                requiresProviderCheck = true;
                break;
            }
            if ((requiresProviderCheck && !grant.isApplicable) || (grant.isApplicable && !grant.isApplicable(spell, application)))
                throw std::invalid_argument("metamagic applicability is not established");
            const auto adjustment = standardMetamagicAdjustment(application, baseSpellLevel);
            result.nominalSlotLevel += adjustment.slotLevelIncrease;
            result.effectiveSpellLevel += adjustment.effectiveLevelIncrease;
            if (grant.slotCost == MetamagicSlotCostPolicy::Standard) result.requiredSlotLevel += adjustment.slotLevelIncrease;
            if (grant.rodGrade)
            {
                if (rodCap) throw std::invalid_argument("only one metamagic rod may affect a spell");
                rodCap = metamagicRodMaximumSpellLevel(*grant.rodGrade);
            }
            if (grant.usageCost) result.usageCosts.push_back(*grant.usageCost);
            result.applications.push_back(std::move(application));
        }
        if (rodCap && std::max(result.requiredSlotLevel, result.effectiveSpellLevel) > *rodCap)
            throw std::invalid_argument("spell exceeds metamagic rod level limit");
        return result;
    }

    void validateMetamagicApplication(const MetamagicApplication &application)
    {
        normalize(application.grantId);
        validateType(application.type);
        switch (application.type)
        {
        case MetamagicType::Heighten:
        {
            const auto *options = std::get_if<HeightenMetamagicOptions>(&application.options);
            if (!options || options->effectiveSpellLevel < 1 || options->effectiveSpellLevel > 9)
                throw std::invalid_argument("heighten requires a target spell level between 1 and 9");
            break;
        }
        case MetamagicType::Reach:
        {
            const auto *options = std::get_if<ReachMetamagicOptions>(&application.options);
            if (!options || options->rangeSteps < 1 || options->rangeSteps > 3)
                throw std::invalid_argument("reach requires between 1 and 3 range steps");
            break;
        }
        case MetamagicType::Elemental:
        {
            const auto *options = std::get_if<ElementalMetamagicOptions>(&application.options);
            if (!options) throw std::invalid_argument("elemental metamagic requires energy and damage mode");
            validateEnergy(options->energy);
            if (options->mode != ElementalDamageMode::Replace && options->mode != ElementalDamageMode::Split)
                throw std::invalid_argument("invalid elemental damage mode");
            break;
        }
        default:
            if (!std::holds_alternative<std::monostate>(application.options))
                throw std::invalid_argument("metamagic type does not accept application options");
        }
    }

    MetamagicLevelAdjustment standardMetamagicAdjustment(const MetamagicApplication &application, int baseSpellLevel)
    {
        validateMetamagicApplication(application);
        if (baseSpellLevel < 0 || baseSpellLevel > 9) throw std::invalid_argument("invalid base spell level");
        switch (application.type)
        {
        case MetamagicType::Heighten:
        {
            const int increase = std::get<HeightenMetamagicOptions>(application.options).effectiveSpellLevel - baseSpellLevel;
            if (increase <= 0) throw std::invalid_argument("heighten must increase the spell level");
            return {increase, increase};
        }
        case MetamagicType::Reach: return {std::get<ReachMetamagicOptions>(application.options).rangeSteps, 0};
        case MetamagicType::Empower: return {2, 0};
        case MetamagicType::Maximize: case MetamagicType::Widen: return {3, 0};
        case MetamagicType::Quicken: return {4, 0};
        case MetamagicType::Enlarge: case MetamagicType::Extend: case MetamagicType::Silent:
        case MetamagicType::Still: case MetamagicType::Elemental: case MetamagicType::Intensified:
        case MetamagicType::Selective: return {1, 0};
        }
        throw std::invalid_argument("unsupported metamagic cost");
    }

    int metamagicRodMaximumSpellLevel(MetamagicRodGrade grade)
    {
        switch (grade)
        {
        case MetamagicRodGrade::Lesser: return 3;
        case MetamagicRodGrade::Normal: return 6;
        case MetamagicRodGrade::Greater: return 9;
        }
        throw std::invalid_argument("invalid metamagic rod grade");
    }

    std::string preparationMetamagicGrantsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".preparationMetamagicGrants";
    }

    std::string castingMetamagicGrantsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".castingMetamagicGrants";
    }

    MetamagicRegistry::MetamagicRegistry(ResourceManager &resources, std::string_view castingId)
        : resources_(resources), endpoints_{preparationMetamagicGrantsResource(castingId), castingMetamagicGrantsResource(castingId)}
    {
        std::size_t registered = 0;
        try
        {
            for (const auto phase : {MetamagicPhase::Preparation, MetamagicPhase::Casting})
            {
                resources_.registerCollectionResource<MetamagicGrant>(endpoints_[phaseIndex(phase)],
                    [this, phase](MetamagicGrant grant) { add(phase, std::move(grant)); },
                    [this, phase](std::string_view id) { remove(phase, id); });
                ++registered;
            }
        }
        catch (...)
        {
            while (registered) resources_.unregisterCollectionResource(endpoints_[--registered]);
            throw;
        }
    }

    MetamagicRegistry::~MetamagicRegistry()
    {
        for (const auto &endpoint : endpoints_) resources_.unregisterCollectionResource(endpoint);
    }

    void MetamagicRegistry::add(MetamagicPhase phase, MetamagicGrant grant)
    {
        grant.id = normalize(grant.id);
        grant.source = normalize(grant.source);
        validateType(grant.type);
        if (grant.type == MetamagicType::Elemental)
        {
            if (!grant.elementalEnergy) throw std::invalid_argument("elemental metamagic grant requires an energy");
            validateEnergy(*grant.elementalEnergy);
        }
        else if (grant.elementalEnergy) throw std::invalid_argument("unexpected elemental energy on metamagic grant");
        if (grant.slotCost != MetamagicSlotCostPolicy::Standard && grant.slotCost != MetamagicSlotCostPolicy::Waived)
            throw std::invalid_argument("invalid metamagic slot cost policy");
        if (grant.castingScope != CastingMetamagicScope::AnyCast && grant.castingScope != CastingMetamagicScope::SpontaneousConversionOnly)
            throw std::invalid_argument("invalid casting metamagic scope");
        if (phase == MetamagicPhase::Preparation && grant.castingScope != CastingMetamagicScope::AnyCast)
            throw std::invalid_argument("casting scope is not applicable to preparation grants");
        if (grant.usageCost)
        {
            grant.usageCost->poolId = normalize(grant.usageCost->poolId);
            if (grant.usageCost->uses <= 0) throw std::invalid_argument("metamagic usage cost must be positive");
        }
        if (grant.rodGrade)
        {
            metamagicRodMaximumSpellLevel(*grant.rodGrade);
            if (phase != MetamagicPhase::Casting || grant.slotCost != MetamagicSlotCostPolicy::Waived || !grant.usageCost)
                throw std::invalid_argument("rod grants require casting phase, waived slot cost and a usage pool");
        }
        const auto id = grant.id;
        if (!grants_[phaseIndex(phase)].emplace(id, std::move(grant)).second)
            throw std::invalid_argument("metamagic grant is already registered: " + id);
    }

    void MetamagicRegistry::remove(MetamagicPhase phase, std::string_view id)
    {
        if (!grants_[phaseIndex(phase)].erase(normalize(id))) throw std::invalid_argument("metamagic grant is not registered");
    }

    MetamagicGrant MetamagicRegistry::resolve(MetamagicPhase phase, const MetamagicApplication &application) const
    {
        validateMetamagicApplication(application);
        const auto &entries = grants_[phaseIndex(phase)];
        const auto entry = entries.find(normalize(application.grantId));
        if (entry == entries.end()) throw std::invalid_argument("metamagic is not granted in this phase");
        const auto &grant = entry->second;
        if (grant.type != application.type) throw std::invalid_argument("metamagic application does not match its grant");
        if (application.type == MetamagicType::Elemental && grant.elementalEnergy != std::get<ElementalMetamagicOptions>(application.options).energy)
            throw std::invalid_argument("elemental energy does not match its grant");
        return grant;
    }
}
