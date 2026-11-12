#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace golarion
{
    SpontaneousSpellcasting::SpontaneousSpellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, SpellLevelResolver resolveLevel, const SpellCatalog *catalog)
        : Spellcasting(resources, castingId, spellListClass, catalog, false), resources_(resources), exemptionsResource_(consumptionExemptionsResource(castingId)), resolveLevel_(std::move(resolveLevel))
    {
        resources_.registerCollectionResource<SpellConsumptionExemption>(exemptionsResource_,
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
    }

    SpontaneousSpellcasting::~SpontaneousSpellcasting()
    {
        resources_.unregisterCollectionResource(exemptionsResource_);
    }

    SpellSlotId SpontaneousSpellcasting::checkedSlot(const SpellSlotId &slot) const
    {
        SpellSlotId key{normalize(slot.groupId), slot.spellLevel, slot.index};
        const auto group = slotPools().find(key.groupId);
        if (group == slotPools().end()) throw std::invalid_argument("spell slot group is not registered");
        const auto level = group->second.dailyCapacityByLevel.find(key.spellLevel);
        if (level == group->second.dailyCapacityByLevel.end() || key.index >= static_cast<std::size_t>(level->second))
            throw std::invalid_argument("spell slot does not exist");
        return key;
    }

    SpellCastResult SpontaneousSpellcasting::consume(const SpellSlotId &slot, const SpontaneousSpellConsumptionDefinition &definition)
    {
        const auto &[spellId, applications] = definition;
        const auto key = checkedSlot(slot);
        if (consumedSlots_.contains(key)) throw std::invalid_argument("spell slot is already consumed");
        const auto id = normalize(spellId);
        if (!knowsSpell(id)) throw std::invalid_argument("spell is not in the repertoire");
        const auto &group = slotPools().at(key.groupId);
        if (group.allowedSpellIds && !group.allowedSpellIds->contains(id)) throw std::invalid_argument("spell is not allowed by the slot filter");
        const auto &spell = spellDefinition(id);
        std::optional<int> level;
        if (resolveLevel_) level = resolveLevel_(spellListClass(), group, id);
        else if (const auto entry = spell.spellLevelByClass.find(spellListClass()); entry != spell.spellLevelByClass.end()) level = entry->second;
        if (!level || *level < 0 || *level > 9) throw std::invalid_argument("spell level is unavailable or invalid");
        // Ordinary spontaneous casting is not conversion of a prepared spell.
        auto evaluation = metamagic().evaluate(MetamagicPhase::Casting, spell, *level, applications);
        if (evaluation.requiredSlotLevel > key.spellLevel || evaluation.effectiveSpellLevel > 9)
            throw std::invalid_argument("spell exceeds available slot level");
        const std::array<SpellSlotId, 1> payment{key};
        const SpellConsumptionContext context{spellListClass(), spell, payment};
        bool preserve = false;
        for (const auto &[exemptionId, exemption] : exemptions_)
            if (exemption.applies(context)) { preserve = true; break; }
        auto updated = consumedSlots_;
        if (!preserve) updated.insert(key);
        MetamagicRegistry::pay(evaluation.usageCosts);
        evaluation.usageCosts.clear();
        consumedSlots_.swap(updated);
        return exportCast(std::move(evaluation));
    }

    void SpontaneousSpellcasting::clear(const SpellSlotId &slot)
    {
        consumedSlots_.erase(checkedSlot(slot));
    }

    SpontaneousSpellSlotState SpontaneousSpellcasting::slotState(const SpellSlotId &slot) const
    {
        return consumedSlots_.contains(checkedSlot(slot)) ? SpontaneousSpellSlotState::Consumed : SpontaneousSpellSlotState::Free;
    }

    void SpontaneousSpellcasting::onSlotGroupRemoved(std::string_view id) noexcept
    {
        std::erase_if(consumedSlots_, [id](const auto &slot) { return slot.groupId == id; });
    }

    SpontaneousSpellcastingData SpontaneousSpellcasting::toData() const
    {
        return {{consumedSlots_.begin(), consumedSlots_.end()}};
    }

    void SpontaneousSpellcasting::load(const SpontaneousSpellcastingData &data)
    {
        stageLoad(data)();
    }

    std::function<void()> SpontaneousSpellcasting::stageLoad(const SpontaneousSpellcastingData &data)
    {
        std::set<SpellSlotId> loaded;
        for (const auto &slot : data.consumedSlots)
            if (!loaded.insert(checkedSlot(slot)).second) throw std::invalid_argument("duplicate consumed spell slot");
        return [this, loaded = std::move(loaded)]() mutable noexcept { consumedSlots_.swap(loaded); };
    }

    SpontaneousSpellcastingView SpontaneousSpellcasting::toView() const
    {
        SpontaneousSpellcastingView view{spellListClass(), {}};
        view.metamagic = metamagic().toView();
        for (const auto &[id, grant] : slotPools())
        {
            SpontaneousSpellSlotGroupView group{grant, {}};
            for (const auto &[level, capacity] : grant.dailyCapacityByLevel)
                for (int index = 0; index < capacity; ++index)
                {
                    SpellSlotId slot{id, level, static_cast<std::size_t>(index)};
                    group.slots.push_back({slot, slotState(slot)});
                }
            view.groups.push_back(std::move(group));
        }
        return view;
    }
}
