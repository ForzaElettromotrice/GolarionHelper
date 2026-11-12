#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    std::string preparationSlotsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".preparationSlots";
    }

    std::string metamagicMixingPermissionsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".metamagicMixingPermissions";
    }

    FlexibleSpellcasting::FlexibleSpellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, SpellLevelResolver resolveLevel, const SpellCatalog *catalog)
        : Spellcasting(resources, castingId, spellListClass, catalog), resources_(resources), endpoints_{preparationSlotsResource(castingId), metamagicMixingPermissionsResource(castingId), consumptionExemptionsResource(castingId)}, resolveLevel_(std::move(resolveLevel))
    {
        std::size_t registered = 0;
        try
        {
            resources_.registerCollectionResource<SpellSlotGrant>(endpoints_[0],
                [this](SpellSlotGrant grant)
                {
                    grant = normalizeSpellSlotGrant(std::move(grant));
                    const auto id = grant.id;
                    if (!preparationGroups_.emplace(id, std::move(grant)).second) throw std::invalid_argument("preparation group is already registered");
                },
                [this](std::string_view id)
                {
                    const auto key = normalize(id);
                    if (!preparationGroups_.erase(key)) throw std::invalid_argument("preparation group is not registered");
                    std::erase_if(preparations_, [&key](const auto &entry) { return entry.first.groupId == key; });
                });
            ++registered;
            resources_.registerCollectionResource<MetamagicMixingPermission>(endpoints_[1],
                [this](MetamagicMixingPermission permission)
                {
                    permission.id = normalize(permission.id);
                    permission.source = normalize(permission.source);
                    if (!permission.applies) throw std::invalid_argument("mixing permission callback must not be empty");
                    if (permission.usageCost)
                    {
                        permission.usageCost->poolId = normalize(permission.usageCost->poolId);
                        if (permission.usageCost->uses <= 0) throw std::invalid_argument("mixing permission usage cost must be positive");
                    }
                    const auto id = permission.id;
                    if (!mixingPermissions_.emplace(id, std::move(permission)).second) throw std::invalid_argument("mixing permission is already registered");
                },
                [this](std::string_view id)
                {
                    if (!mixingPermissions_.erase(normalize(id))) throw std::invalid_argument("mixing permission is not registered");
                });
            ++registered;
            resources_.registerCollectionResource<SpellConsumptionExemption>(endpoints_[2],
                [this](SpellConsumptionExemption exemption)
                {
                    exemption.id = normalize(exemption.id);
                    exemption.source = normalize(exemption.source);
                    if (!exemption.applies) throw std::invalid_argument("consumption exemption callback must not be empty");
                    const auto id = exemption.id;
                    if (!exemptions_.emplace(id, std::move(exemption)).second) throw std::invalid_argument("consumption exemption is already registered");
                },
                [this](std::string_view id)
                {
                    if (!exemptions_.erase(normalize(id))) throw std::invalid_argument("consumption exemption is not registered");
                });
            ++registered;
        }
        catch (...)
        {
            while (registered) resources_.unregisterCollectionResource(endpoints_[--registered]);
            throw;
        }
    }

    FlexibleSpellcasting::~FlexibleSpellcasting()
    {
        for (const auto &endpoint : endpoints_) resources_.unregisterCollectionResource(endpoint);
    }

    SpellSlotId FlexibleSpellcasting::checkedSlot(const SpellSlotId &id, const std::map<std::string, SpellSlotGrant> &groups) const
    {
        SpellSlotId key{normalize(id.groupId), id.spellLevel, id.index};
        const auto group = groups.find(key.groupId);
        if (group == groups.end()) throw std::invalid_argument("spell slot group is not registered");
        const auto level = group->second.dailyCapacityByLevel.find(key.spellLevel);
        if (level == group->second.dailyCapacityByLevel.end() || key.index >= static_cast<std::size_t>(level->second))
            throw std::invalid_argument("spell slot does not exist");
        return key;
    }

    int FlexibleSpellcasting::spellLevel(const SpellSlotGrant &group, const std::string &id) const
    {
        if (group.allowedSpellIds && !group.allowedSpellIds->contains(id)) throw std::invalid_argument("spell is not allowed by the slot filter");
        const auto &spell = spellDefinition(id);
        std::optional<int> level;
        if (resolveLevel_) level = resolveLevel_(spellListClass(), group, id);
        else if (const auto entry = spell.spellLevelByClass.find(spellListClass()); entry != spell.spellLevelByClass.end()) level = entry->second;
        if (!level || *level < 0 || *level > 9) throw std::invalid_argument("spell level is unavailable or invalid");
        return *level;
    }

    MetamagicEvaluation FlexibleSpellcasting::checkedPreparation(const SpellSlotId &id, const std::string &spellId, const std::vector<MetamagicApplication> &applications) const
    {
        if (!knowsSpell(spellId)) throw std::invalid_argument("spell is not in the repertoire");
        const auto level = spellLevel(preparationGroups_.at(id.groupId), spellId);
        auto evaluation = metamagic().evaluate(MetamagicPhase::Preparation, spellDefinition(spellId), level, applications);
        if (evaluation.requiredSlotLevel > id.spellLevel || evaluation.effectiveSpellLevel > 9) throw std::invalid_argument("spell exceeds preparation slot level");
        return evaluation;
    }

    void FlexibleSpellcasting::prepare(const SpellSlotId &preparationId, const SpellPreparationDefinition &definition)
    {
        const auto &[spellId, applications] = definition;
        const auto key = checkedSlot(preparationId, preparationGroups_);
        if (preparations_.contains(key)) throw std::invalid_argument("preparation slot is not free");
        auto evaluation = checkedPreparation(key, normalize(spellId), applications);
        auto costs = std::move(evaluation.usageCosts);
        auto updated = preparations_;
        updated.emplace(key, std::move(evaluation));
        MetamagicRegistry::pay(costs);
        preparations_.swap(updated);
    }

    void FlexibleSpellcasting::clearPreparation(const SpellSlotId &preparationId)
    {
        preparations_.erase(checkedSlot(preparationId, preparationGroups_));
    }

    MetamagicEvaluation FlexibleSpellcasting::consume(const SpellSlotId &slotId, const FlexibleSpellConsumptionDefinition &definition)
    {
        const auto &[preparationId, applications, mixingPermissionId] = definition;
        const auto slot = checkedSlot(slotId, slotPools());
        const auto selection = checkedSlot(preparationId, preparationGroups_);
        if (consumedSlots_.contains(slot)) throw std::invalid_argument("spell slot is already consumed");
        const auto entry = preparations_.find(selection);
        if (entry == preparations_.end()) throw std::invalid_argument("spell is not prepared");
        const auto &prepared = entry->second;
        const auto &spell = spellDefinition(prepared.spellId);
        if (spellLevel(slotPools().at(slot.groupId), prepared.spellId) != prepared.baseSpellLevel)
            throw std::invalid_argument("casting group has an incompatible spell level");
        auto evaluation = metamagic().evaluate(MetamagicPhase::Casting, spell, prepared.baseSpellLevel, applications, false, &prepared);
        if (evaluation.requiredSlotLevel > slot.spellLevel || evaluation.effectiveSpellLevel > 9)
            throw std::invalid_argument("spell exceeds available casting slot level");
        if (!prepared.applications.empty() && !applications.empty())
        {
            const MetamagicMixingContext context{spellListClass(), spell, selection, slot, prepared, applications};
            const MetamagicMixingPermission *permission = nullptr;
            if (mixingPermissionId)
            {
                const auto match = mixingPermissions_.find(normalize(*mixingPermissionId));
                if (match != mixingPermissions_.end() && match->second.applies(context)) permission = &match->second;
            }
            else
                for (const auto &[id, candidate] : mixingPermissions_)
                    if (!candidate.usageCost && candidate.applies(context)) { permission = &candidate; break; }
            if (!permission) throw std::invalid_argument("metamagic mixing requires an applicable permission; paid permissions must be selected");
            if (permission->usageCost) evaluation.usageCosts.push_back(*permission->usageCost);
        }
        else if (mixingPermissionId) throw std::invalid_argument("mixing permission supplied without mixed metamagic");
        const std::array<SpellSlotId, 1> payment{slot};
        const SpellConsumptionContext context{spellListClass(), spell, payment};
        bool preserve = false;
        for (const auto &[id, exemption] : exemptions_)
            if (exemption.applies(context)) { preserve = true; break; }
        auto updated = consumedSlots_;
        if (!preserve) updated.insert(slot);
        MetamagicRegistry::pay(evaluation.usageCosts);
        evaluation.usageCosts.clear();
        consumedSlots_.swap(updated);
        return evaluation;
    }

    void FlexibleSpellcasting::clearSlot(const SpellSlotId &slotId)
    {
        consumedSlots_.erase(checkedSlot(slotId, slotPools()));
    }

    void FlexibleSpellcasting::onSlotGroupRemoved(std::string_view id) noexcept
    {
        std::erase_if(consumedSlots_, [id](const auto &slot) { return slot.groupId == id; });
    }

    FlexibleSpellcastingData FlexibleSpellcasting::toData() const
    {
        FlexibleSpellcastingData data;
        for (const auto &[id, evaluation] : preparations_) data.preparations.push_back({id, evaluation.spellId, evaluation.applications, preparationSnapshot(evaluation, spellListClass())});
        data.consumedSlots.assign(consumedSlots_.begin(), consumedSlots_.end());
        return data;
    }

    void FlexibleSpellcasting::load(const FlexibleSpellcastingData &data)
    {
        stageLoad(data)();
    }

    std::function<void()> FlexibleSpellcasting::stageLoad(const FlexibleSpellcastingData &data)
    {
        std::map<SpellSlotId, MetamagicEvaluation> prepared;
        std::set<SpellSlotId> consumed;
        for (const auto &entry : data.preparations)
        {
            const auto key = checkedSlot(entry.preparationId, preparationGroups_);
            const auto spellId = normalize(entry.spellId);
            if (entry.evaluation && entry.evaluation->paidSlots != 1) throw std::invalid_argument("flexible preparation requires one preparation slot");
            auto evaluation = entry.evaluation ? restorePreparationSnapshot(spellId, entry.metamagic, *entry.evaluation, spellListClass())
                                               : checkedPreparation(key, spellId, entry.metamagic);
            spellDefinition(spellId);
            const auto &group = preparationGroups_.at(key.groupId);
            if (evaluation.requiredSlotLevel > key.spellLevel ||
                (group.allowedSpellIds && !group.allowedSpellIds->contains(spellId)))
                throw std::invalid_argument("saved preparation is incompatible with its slot");
            evaluation.usageCosts.clear();
            if (!prepared.emplace(key, std::move(evaluation)).second) throw std::invalid_argument("duplicate preparation slot");
        }
        for (const auto &id : data.consumedSlots)
            if (!consumed.insert(checkedSlot(id, slotPools())).second) throw std::invalid_argument("duplicate consumed spell slot");
        return [this, prepared = std::move(prepared), consumed = std::move(consumed)]() mutable noexcept
        {
            preparations_.swap(prepared);
            consumedSlots_.swap(consumed);
        };
    }

    FlexibleSpellcastingView FlexibleSpellcasting::toView() const
    {
        FlexibleSpellcastingView view{spellListClass(), {}, {}};
        view.metamagic = metamagic().toView();
        for (const auto &[id, permission] : mixingPermissions_)
            view.mixingPermissions.push_back({id, permission.source, permission.usageCost ? std::optional(metamagicUsageCostView(*permission.usageCost)) : std::nullopt});
        for (const auto &[groupId, grant] : preparationGroups_)
        {
            FlexiblePreparationGroupView group{grant, {}};
            for (const auto &[level, count] : grant.dailyCapacityByLevel)
                for (int index = 0; index < count; ++index)
                {
                    SpellSlotId id{groupId, level, static_cast<std::size_t>(index)};
                    FlexibleSpellPreparationView entry{id, std::nullopt, {}};
                    if (const auto found = preparations_.find(id); found != preparations_.end())
                    {
                        entry.spellId = found->second.spellId;
                        entry.metamagic = found->second.applications;
                        entry.evaluation = preparationSnapshot(found->second, spellListClass());
                    }
                    group.preparations.push_back(std::move(entry));
                }
            view.preparationGroups.push_back(std::move(group));
        }
        for (const auto &[groupId, grant] : slotPools())
        {
            SpontaneousSpellSlotGroupView group{grant, {}};
            for (const auto &[level, count] : grant.dailyCapacityByLevel)
                for (int index = 0; index < count; ++index)
                {
                    SpellSlotId id{groupId, level, static_cast<std::size_t>(index)};
                    group.slots.push_back({id, consumedSlots_.contains(id) ? SpontaneousSpellSlotState::Consumed : SpontaneousSpellSlotState::Free});
                }
            view.slotGroups.push_back(std::move(group));
        }
        return view;
    }

    MetamagicUsagePools FlexibleSpellcasting::usagePools() const
    {
        auto pools = Spellcasting::usagePools();
        for (const auto &[id, permission] : mixingPermissions_)
            if (permission.usageCost && permission.usageCost->pool) collectMetamagicPool(pools, *permission.usageCost);
        return pools;
    }
}
