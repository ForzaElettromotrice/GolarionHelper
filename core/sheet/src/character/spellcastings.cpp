#include "golarion/character/spellcastings.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    Spellcastings::Spellcastings(ResourceManager &resources) : resources_(resources)
    {
        resources_.registerCollectionResource<SpellcastingGrant>(Resource,
            [this](SpellcastingGrant entry) { grant(std::move(entry)); },
            [this](std::string_view id) { revoke(id); });
        try
        {
            resources_.registerCollectionResource<std::shared_ptr<MetamagicUsagePool>>(UsagePoolsResource,
                [this](std::shared_ptr<MetamagicUsagePool> pool)
                {
                    if (!pool) throw std::invalid_argument("metamagic source pool must not be null");
                    const auto existing = usagePools();
                    if (const auto found = existing.find(pool->id()); found != existing.end() && found->second != pool)
                        throw std::invalid_argument("conflicting metamagic pool identities");
                    const auto id = pool->id();
                    if (!sourcePools_.emplace(id, std::move(pool)).second) throw std::invalid_argument("metamagic source pool is already registered");
                },
                [this](std::string_view id)
                {
                    if (!sourcePools_.erase(normalize(id))) throw std::invalid_argument("metamagic source pool is not registered");
                });
        }
        catch (...) { resources_.unregisterCollectionResource(Resource); throw; }
    }

    Spellcastings::~Spellcastings()
    {
        resources_.unregisterCollectionResource(UsagePoolsResource);
        resources_.unregisterCollectionResource(Resource);
    }

    void Spellcastings::grant(SpellcastingGrant entry)
    {
        entry.id = normalize(entry.id);
        entry.source = normalize(entry.source);
        if (entries_.contains(entry.id)) throw std::invalid_argument("spellcasting is already granted");
        std::unique_ptr<Spellcasting> casting;
        switch (entry.mode)
        {
        case SpellcastingMode::Prepared:
            casting = std::make_unique<PreparedSpellcasting>(resources_, entry.id, entry.spellListClass);
            break;
        case SpellcastingMode::Spontaneous:
            casting = std::make_unique<SpontaneousSpellcasting>(resources_, entry.id, entry.spellListClass);
            break;
        case SpellcastingMode::Flexible:
            casting = std::make_unique<FlexibleSpellcasting>(resources_, entry.id, entry.spellListClass);
            break;
        default:
            throw std::invalid_argument("invalid spellcasting mode");
        }
        const auto id = entry.id;
        entries_.emplace(id, Entry{std::move(entry), std::move(casting)});
    }

    void Spellcastings::revoke(std::string_view id)
    {
        if (!entries_.erase(normalize(id))) throw std::invalid_argument("spellcasting is not granted");
    }

    Spellcasting &Spellcastings::get(std::string_view id)
    {
        const auto found = entries_.find(normalize(id));
        if (found == entries_.end()) throw std::invalid_argument("spellcasting is not granted");
        return *found->second.casting;
    }

    SpellcastingsData Spellcastings::toData() const
    {
        SpellcastingsData data;
        data.metamagicUses.emplace();
        for (const auto &[id, pool] : usagePools())
            if (!pool->isExternallyOwned()) data.metamagicUses->emplace(id, pool->remaining());
        for (const auto &[id, entry] : entries_)
        {
            SpellcastingData saved{id, {entry.casting->repertoire_.begin(), entry.casting->repertoire_.end()}, {}};
            switch (entry.grant.mode)
            {
            case SpellcastingMode::Prepared: saved.state = static_cast<const PreparedSpellcasting &>(*entry.casting).toData(); break;
            case SpellcastingMode::Spontaneous: saved.state = static_cast<const SpontaneousSpellcasting &>(*entry.casting).toData(); break;
            case SpellcastingMode::Flexible: saved.state = static_cast<const FlexibleSpellcasting &>(*entry.casting).toData(); break;
            }
            data.castings.push_back(std::move(saved));
        }
        return data;
    }

    SpellcastingsView Spellcastings::toView() const
    {
        SpellcastingsView view;
        for (const auto &[id, pool] : usagePools()) view.metamagicPools.push_back({id, pool->remaining()});
        for (const auto &[id, entry] : entries_)
        {
            SpellcastingView casting{id, entry.grant.source, entry.grant.mode, {entry.casting->repertoire_.begin(), entry.casting->repertoire_.end()}, {}};
            switch (entry.grant.mode)
            {
            case SpellcastingMode::Prepared: casting.state = static_cast<const PreparedSpellcasting &>(*entry.casting).toView(); break;
            case SpellcastingMode::Spontaneous: casting.state = static_cast<const SpontaneousSpellcasting &>(*entry.casting).toView(); break;
            case SpellcastingMode::Flexible: casting.state = static_cast<const FlexibleSpellcasting &>(*entry.casting).toView(); break;
            }
            view.castings.push_back(std::move(casting));
        }
        return view;
    }

    void Spellcastings::load(const SpellcastingsData &data)
    {
        std::vector<std::pair<std::shared_ptr<MetamagicUsagePool>, int>> poolUpdates;
        if (data.metamagicUses)
        {
            auto pools = usagePools();
            // Linked balances belong to the source's save data, never to this container.
            std::erase_if(pools, [](const auto &entry) { return entry.second->isExternallyOwned(); });
            if (pools.size() != data.metamagicUses->size()) throw std::invalid_argument("saved metamagic pools do not match sources");
            for (const auto &[id, remaining] : *data.metamagicUses)
            {
                const auto found = pools.find(id);
                if (found == pools.end() || remaining < 0) throw std::invalid_argument("invalid saved metamagic pool");
                poolUpdates.emplace_back(found->second, remaining);
            }
        }
        std::set<std::string> ids;
        std::vector<std::function<void()>> commits;
        for (const auto &saved : data.castings)
        {
            const auto id = normalize(saved.id);
            if (!ids.insert(id).second) throw std::invalid_argument("duplicate spellcasting ID");
            auto &casting = get(id);
            std::set<std::string> repertoire;
            for (const auto &spellId : saved.repertoire)
                if (!repertoire.insert(normalize(spellId)).second) throw std::invalid_argument("duplicate repertoire spell ID");
            // Validate against the restored repertoire, then restore the live one until commit.
            casting.repertoire_.swap(repertoire);
            std::function<void()> commit;
            try
            {
                commit = std::visit([&](const auto &state) -> std::function<void()>
                {
                    using Data = std::decay_t<decltype(state)>;
                    using Casting = std::conditional_t<std::is_same_v<Data, PreparedSpellcastingData>, PreparedSpellcasting,
                        std::conditional_t<std::is_same_v<Data, SpontaneousSpellcastingData>, SpontaneousSpellcasting, FlexibleSpellcasting>>;
                    auto *typed = dynamic_cast<Casting *>(&casting);
                    if (!typed) throw std::invalid_argument("saved spellcasting mode does not match its grant");
                    return typed->stageLoad(state);
                }, saved.state);
            }
            catch (...)
            {
                casting.repertoire_.swap(repertoire);
                throw;
            }
            casting.repertoire_.swap(repertoire);
            commits.push_back([&casting, repertoire = std::move(repertoire), commit = std::move(commit)]() mutable
            {
                casting.repertoire_.swap(repertoire);
                commit();
            });
        }
        if (ids.size() != entries_.size()) throw std::invalid_argument("save is missing a granted spellcasting");
        // Every staged operation now consists only of non-throwing swaps. Never pay source uses here.
        for (auto &commit : commits) commit();
        for (const auto &[pool, remaining] : poolUpdates) pool->reset(remaining);
    }

    MetamagicUsagePools Spellcastings::usagePools() const
    {
        auto pools = sourcePools_;
        for (const auto &[id, entry] : entries_)
            for (const auto &[poolId, pool] : entry.casting->usagePools())
                collectMetamagicPool(pools, {poolId, 1, pool});
        return pools;
    }
}
