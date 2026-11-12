#include "golarion/character/spell_repertoire.hpp"
#include "golarion/character/spell_definition.hpp"
#include "golarion/character/spell_slot_grant.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include <algorithm>
#include <limits>

namespace golarion
{
    std::string repertoireQuotasResource(std::string_view id) { return "spellcasting." + normalize(id) + ".repertoire.quotas"; }
    std::string repertoireGrantsResource(std::string_view id) { return "spellcasting." + normalize(id) + ".repertoire.grants"; }
    std::string repertoireListsResource(std::string_view id) { return "spellcasting." + normalize(id) + ".repertoire.lists"; }

    SpellRepertoire::SpellRepertoire(ResourceManager &resources, std::string_view castingId, CharacterClassType list, bool allowManual, std::function<const SpellDefinition &(std::string_view)> resolve)
        : resources_(resources), quotasResource_(repertoireQuotasResource(castingId)), grantsResource_(repertoireGrantsResource(castingId)),
          listsResource_(repertoireListsResource(castingId)), list_(list), allowManual_(allowManual), resolve_(std::move(resolve))
    {
        resources_.registerCollectionResource<SpellRepertoireQuota>(quotasResource_, [this](auto grant)
        {
            auto validated = normalizeSpellSlotGrant({grant.id, grant.source, grant.capacityByLevel, grant.allowedSpellIds});
            grant = {validated.id, validated.source, std::move(validated.dailyCapacityByLevel), std::move(validated.allowedSpellIds)};
            const auto id = grant.id;
            if (!quotas_.emplace(id, std::move(grant)).second) throw std::invalid_argument("repertoire quota already registered");
        }, [this](auto id)
        {
            if (!quotas_.erase(normalize(id))) throw std::invalid_argument("repertoire quota is not registered");
        });
        try
        {
            resources_.registerCollectionResource<SpellRepertoireGrant>(grantsResource_, [this](auto grant)
            {
                grant.id = normalize(grant.id);
                grant.source = normalize(grant.source);
                if (grant.spellIds.empty()) throw std::invalid_argument("repertoire grant must contain spells");
                std::set<std::string> ids;
                for (const auto &id : grant.spellIds)
                {
                    const auto key = normalize(id);
                    resolve_(key);
                    if (!ids.insert(key).second) throw std::invalid_argument("duplicate normalized repertoire grant spell");
                }
                grant.spellIds = std::move(ids);
                const auto id = grant.id;
                if (!grants_.emplace(id, std::move(grant)).second) throw std::invalid_argument("repertoire grant already registered");
            }, [this](auto id)
            {
                if (!grants_.erase(normalize(id))) throw std::invalid_argument("repertoire grant is not registered");
            });
            try
            {
                resources_.registerCollectionResource<SpellRepertoireListGrant>(listsResource_, [this](auto grant)
                {
                    grant.id = normalize(grant.id);
                    grant.source = normalize(grant.source);
                    const auto id = grant.id;
                    if (!lists_.emplace(id, std::move(grant)).second) throw std::invalid_argument("repertoire list already registered");
                }, [this](auto id)
                {
                    if (!lists_.erase(normalize(id))) throw std::invalid_argument("repertoire list is not registered");
                });
            }
            catch (...) { resources_.unregisterCollectionResource(grantsResource_); throw; }
        }
        catch (...) { resources_.unregisterCollectionResource(quotasResource_); throw; }
    }

    SpellRepertoire::~SpellRepertoire()
    {
        resources_.unregisterCollectionResource(listsResource_);
        resources_.unregisterCollectionResource(grantsResource_);
        resources_.unregisterCollectionResource(quotasResource_);
    }

    int SpellRepertoire::listLevel(std::string_view id) const
    {
        const auto &spell = resolve_(id);
        const auto found = spell.spellLevelByClass.find(list_);
        return found == spell.spellLevelByClass.end() ? -1 : found->second;
    }

    void SpellRepertoire::addManual(std::string_view id)
    {
        if (!allowManual_) throw std::invalid_argument("spontaneous repertoire requires a quota selection");
        const auto key = normalize(id);
        resolve_(key); // List/level authorization remains part of preparation.
        if (std::ranges::find(data_.manualSpellIds, key) == data_.manualSpellIds.end())
        {
            data_.manualSpellIds.push_back(key);
            std::ranges::sort(data_.manualSpellIds);
        }
    }

    void SpellRepertoire::removeManual(std::string_view id)
    {
        if (!allowManual_) throw std::invalid_argument("spontaneous repertoire has no manual entries");
        if (!std::erase(data_.manualSpellIds, normalize(id))) throw std::invalid_argument("manual spell is not registered");
    }

    int SpellRepertoire::selectedCount(std::string_view quotaId, int level) const
    {
        const auto count = std::ranges::count_if(data_.selections, [&](const auto &entry) { return entry.quotaId == quotaId && entry.spellLevel == level; });
        if (count > std::numeric_limits<int>::max()) throw std::overflow_error("too many repertoire selections");
        return static_cast<int>(count);
    }

    bool SpellRepertoire::accepts(const SpellRepertoireQuota &quota, int level, std::string_view id) const
    {
        return listLevel(id) == level && (!quota.allowedSpellIds || quota.allowedSpellIds->contains(std::string(id)));
    }

    bool SpellRepertoire::active(const RepertoireSelectionDefinition &selection) const
    {
        const auto quota = quotas_.find(selection.quotaId);
        if (quota == quotas_.end()) return false;
        const auto level = quota->second.capacityByLevel.find(selection.spellLevel);
        return level != quota->second.capacityByLevel.end() && selectedCount(selection.quotaId, selection.spellLevel) <= level->second
            && accepts(quota->second, selection.spellLevel, selection.spellId);
    }

    void SpellRepertoire::select(RepertoireSelectionDefinition selection)
    {
        selection.quotaId = normalize(selection.quotaId);
        selection.spellId = normalize(selection.spellId);
        const auto found = quotas_.find(selection.quotaId);
        if (found == quotas_.end()) throw std::invalid_argument("repertoire quota is not registered");
        const auto capacity = found->second.capacityByLevel.find(selection.spellLevel);
        if (capacity == found->second.capacityByLevel.end() || selectedCount(selection.quotaId, selection.spellLevel) >= capacity->second)
            throw std::invalid_argument("repertoire quota has no available selection");
        if (!accepts(found->second, selection.spellLevel, selection.spellId)) throw std::invalid_argument("spell does not match repertoire quota");
        if (std::ranges::any_of(data_.selections, [&](const auto &entry) { return entry.spellId == selection.spellId; }))
            throw std::invalid_argument("spell already occupies a repertoire quota");
        const auto id = selection.spellId;
        data_.selections.push_back(std::move(selection));
        std::erase(data_.unassignedSpellIds, id);
    }

    void SpellRepertoire::unselect(RepertoireSelectionDefinition selection)
    {
        selection.quotaId = normalize(selection.quotaId);
        selection.spellId = normalize(selection.spellId);
        if (!std::erase(data_.selections, selection)) throw std::invalid_argument("repertoire selection does not exist");
    }

    bool SpellRepertoire::knows(std::string_view id) const
    {
        const auto key = normalize(id);
        if (std::ranges::find(data_.manualSpellIds, key) != data_.manualSpellIds.end()) return true;
        for (const auto &selection : data_.selections) if (selection.spellId == key && active(selection)) return true;
        for (const auto &[source, grant] : grants_) if (grant.spellIds.contains(key)) return true;
        return !lists_.empty() && listLevel(key) >= 0;
    }

    std::vector<std::string> SpellRepertoire::knownIds() const
    {
        std::set<std::string> ids(data_.manualSpellIds.begin(), data_.manualSpellIds.end());
        for (const auto &selection : data_.selections) if (active(selection)) ids.insert(selection.spellId);
        for (const auto &[source, grant] : grants_) ids.insert(grant.spellIds.begin(), grant.spellIds.end());
        return {ids.begin(), ids.end()};
    }

    SpellRepertoireView SpellRepertoire::toView() const
    {
        SpellRepertoireView view{allowManual_, data_.manualSpellIds, data_.unassignedSpellIds, {}, {}, {}, {}};
        for (const auto &selection : data_.selections) view.selections.push_back({selection, active(selection)});
        for (const auto &[id, quota] : quotas_)
        {
            RepertoireQuotaView entry{quota, {}};
            for (const auto &[level, capacity] : quota.capacityByLevel) entry.remainingByLevel[level] = std::max(0, capacity - selectedCount(id, level));
            view.quotas.push_back(std::move(entry));
        }
        for (const auto &[id, grant] : grants_) view.grants.push_back(grant);
        for (const auto &[id, grant] : lists_) view.listGrants.push_back(grant);
        return view;
    }

    SpellRepertoireData SpellRepertoire::toData() const { return data_; }
    void SpellRepertoire::swapData(SpellRepertoireData &data) noexcept { std::swap(data_, data); }

    SpellRepertoireData SpellRepertoire::validatedData(SpellRepertoireData data) const
    {
        if (!allowManual_ && !data.manualSpellIds.empty()) throw std::invalid_argument("spontaneous save cannot contain manual spells");
        if (allowManual_ && !data.unassignedSpellIds.empty()) throw std::invalid_argument("prepared save cannot contain unassigned spells");
        const auto normalizeIds = [&](auto &ids)
        {
            std::set<std::string> unique;
            for (auto &id : ids)
            {
                id = normalize(id);
                resolve_(id);
                if (!unique.insert(id).second) throw std::invalid_argument("duplicate repertoire spell");
            }
            std::ranges::sort(ids);
        };
        normalizeIds(data.manualSpellIds);
        normalizeIds(data.unassignedSpellIds);
        std::set<std::string> selected;
        for (auto &selection : data.selections)
        {
            selection.quotaId = normalize(selection.quotaId);
            selection.spellId = normalize(selection.spellId);
            resolve_(selection.spellId);
            if (selection.spellLevel < 0 || selection.spellLevel > 9 || !selected.insert(selection.spellId).second ||
                std::ranges::find(data.unassignedSpellIds, selection.spellId) != data.unassignedSpellIds.end())
                throw std::invalid_argument("invalid saved repertoire selection");
        }
        return data;
    }
}
