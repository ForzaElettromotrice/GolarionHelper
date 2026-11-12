#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <algorithm>
#include <utility>

namespace golarion
{
    PreparedSpellcasting::PreparedSpellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, SpellLevelResolver resolveLevel, const SpellCatalog *catalog)
        : Spellcasting(resources, castingId, spellListClass, catalog), resolveLevel_(std::move(resolveLevel)), preparationCosts_(resources, castingId), preparationRules_(resources, castingId) {}

    SpellSlotId PreparedSpellcasting::checkedSlot(const SpellSlotId &slot) const
    {
        SpellSlotId key{normalize(slot.groupId), slot.spellLevel, slot.index};
        const auto group = slotPools().find(key.groupId);
        if (group == slotPools().end()) throw std::invalid_argument("spell slot group is not registered");
        const auto level = group->second.dailyCapacityByLevel.find(key.spellLevel);
        if (level == group->second.dailyCapacityByLevel.end() || key.index >= static_cast<std::size_t>(level->second))
            throw std::invalid_argument("spell slot does not exist");
        return key;
    }

    PreparedSpellSlot PreparedSpellcasting::slotState(const SpellSlotId &slot) const
    {
        const auto key = checkedSlot(slot);
        for (const auto &[primary, preparation] : preparations_)
            if (std::ranges::find(preparation.slots, key) != preparation.slots.end()) return preparation.state;
        return {};
    }

    void PreparedSpellcasting::prepare(const SpellSlotId &slot, const SpellPreparationDefinition &definition)
    {
        prepare(std::vector<SpellSlotId>{slot}, definition);
    }

    void PreparedSpellcasting::prepare(const std::vector<SpellSlotId> &slots, const SpellPreparationDefinition &definition)
    {
        const auto &[spellId, applications] = definition;
        const auto id = normalize(spellId);
        auto preparation = checkedPreparation(slots, id, PreparationValidation::Preparing, applications);
        for (const auto &key : preparation.slots)
            if (slotState(key).state != PreparedSpellSlotState::Free) throw std::invalid_argument("spell slot is not free");
        const auto primary = preparation.slots.front();
        auto updated = preparations_;
        auto costs = std::move(preparation.evaluation.usageCosts);
        updated.emplace(primary, std::move(preparation));
        MetamagicRegistry::pay(costs);
        preparations_.swap(updated);
    }

    PreparedSpellcasting::Preparation PreparedSpellcasting::checkedPreparation(const std::vector<SpellSlotId> &slots, const std::string &id, PreparationValidation mode, const std::vector<MetamagicApplication> &applications) const
    {
        if (slots.empty()) throw std::invalid_argument("preparation requires slots");
        std::vector<SpellSlotId> keys;
        std::optional<MetamagicEvaluation> evaluation;
        for (const auto &slot : slots)
        {
            const auto key = checkedSlot(slot);
            if (key.spellLevel != slots.front().spellLevel) throw std::invalid_argument("preparation slots must have the same level");
            const int level = preparationLevel(key, id);
            const auto &definition = spellDefinition(id);
            if (!evaluation) evaluation = metamagic().evaluate(MetamagicPhase::Preparation, definition, level, applications);
            if (evaluation->baseSpellLevel != level) throw std::invalid_argument("linked slots have incompatible spell levels");
            if (evaluation->requiredSlotLevel > key.spellLevel || evaluation->effectiveSpellLevel > 9)
                throw std::invalid_argument("metamagic exceeds available slot level");
            if (mode == PreparationValidation::Preparing)
                preparationRules_.validatePreparation({spellListClass(), definition, slotPools().at(key.groupId), key, evaluation->effectiveSpellLevel});
            keys.push_back(key);
        }
        std::ranges::sort(keys);
        if (std::adjacent_find(keys.begin(), keys.end()) != keys.end()) throw std::invalid_argument("duplicate preparation slot");
        const auto &definition = spellDefinition(id);
        const auto cost = preparationCosts_.calculate(definition.school, keys.front().spellLevel);
        if (keys.size() != static_cast<std::size_t>(cost.requiredSlots)) throw std::invalid_argument("incorrect number of preparation slots");
        return {{PreparedSpellSlotState::Prepared, id, evaluation->applications}, std::move(keys), std::move(*evaluation)};
    }

    int PreparedSpellcasting::preparationLevel(const SpellSlotId &key, const std::string &id) const
    {
        if (!knowsSpell(id)) throw std::invalid_argument("spell is not in the repertoire");
        const auto &group = slotPools().at(key.groupId);
        if (group.allowedSpellIds && !group.allowedSpellIds->contains(id))
            throw std::invalid_argument("spell is not allowed by the slot filter");
        auto level = preparationRules_.levelOverride(key.groupId, id);
        if (!level && resolveLevel_)
            level = resolveLevel_(spellListClass(), group, id);
        else if (!level)
        {
            const auto &definition = spellDefinition(id);
            const auto entry = definition.spellLevelByClass.find(spellListClass());
            if (entry != definition.spellLevelByClass.end()) level = entry->second;
        }
        if (!level || *level < 0 || *level > 9 || *level > key.spellLevel)
            throw std::invalid_argument("spell level is unavailable or incompatible with the slot");
        return *level;
    }

    MetamagicEvaluation PreparedSpellcasting::consume(const SpellSlotId &slot, const PreparedSpellConsumptionDefinition &definition)
    {
        const auto &[replacementSpellId, applications] = definition;
        const auto key = checkedSlot(slot);
        for (auto &[primary, preparation] : preparations_)
            if (std::ranges::find(preparation.slots, key) != preparation.slots.end())
            {
                if (preparation.state.state != PreparedSpellSlotState::Prepared) break;
                const auto &prepared = spellDefinition(*preparation.state.spellId);
                MetamagicEvaluation evaluation;
                bool preserve = false;
                if (replacementSpellId)
                {
                    const auto &replacement = spellDefinition(normalize(*replacementSpellId));
                    const auto acceptsLevel = [&](int level)
                    {
                        evaluation = metamagic().evaluate(MetamagicPhase::Casting, replacement, level, applications, true);
                        return evaluation.requiredSlotLevel <= key.spellLevel && evaluation.effectiveSpellLevel <= 9;
                    };
                    if (!preparationRules_.allowsConversion({spellListClass(), prepared, replacement, preparation.slots, slotPools()}, acceptsLevel))
                        throw std::invalid_argument("spontaneous conversion is not allowed");
                    // Conversion sacrifices the preparation; normal-use exemptions
                    // must not turn a repeatable spell into unlimited conversion fuel.
                }
                else
                {
                    evaluation = metamagic().evaluate(MetamagicPhase::Casting, prepared, preparation.evaluation.baseSpellLevel, applications, false, &preparation.evaluation);
                    if (evaluation.requiredSlotLevel > key.spellLevel || evaluation.effectiveSpellLevel > 9)
                        throw std::invalid_argument("metamagic exceeds available slot level");
                    preserve = preparationRules_.preservesPreparation({spellListClass(), prepared, preparation.slots});
                }
                MetamagicRegistry::pay(evaluation.usageCosts);
                evaluation.usageCosts.clear();
                if (!preserve) preparation.state.state = PreparedSpellSlotState::Consumed;
                return evaluation;
            }
        throw std::invalid_argument("spell slot is not prepared");
    }

    void PreparedSpellcasting::clear(const SpellSlotId &slot)
    {
        const auto key = checkedSlot(slot);
        std::erase_if(preparations_, [&key](const auto &entry) { return std::ranges::find(entry.second.slots, key) != entry.second.slots.end(); });
    }

    void PreparedSpellcasting::onSlotGroupRemoved(std::string_view id) noexcept
    {
        std::erase_if(preparations_, [id](const auto &entry) {
            return std::ranges::any_of(entry.second.slots, [id](const auto &slot) { return slot.groupId == id; });
        });
    }

    PreparedSpellcastingData PreparedSpellcasting::toData() const
    {
        PreparedSpellcastingData data;
        for (const auto &[slot, preparation] : preparations_)
            data.preparations.push_back({slot, *preparation.state.spellId, preparation.state.state == PreparedSpellSlotState::Consumed,
                {preparation.slots.begin() + 1, preparation.slots.end()}, preparation.state.metamagic, preparationSnapshot(preparation.evaluation, spellListClass(), static_cast<int>(preparation.slots.size()))});
        return data;
    }

    void PreparedSpellcasting::load(const PreparedSpellcastingData &data)
    {
        stageLoad(data)();
    }

    std::function<void()> PreparedSpellcasting::stageLoad(const PreparedSpellcastingData &data)
    {
        std::map<SpellSlotId, Preparation> loaded;
        std::set<SpellSlotId> occupied;
        for (const auto &entry : data.preparations)
        {
            const auto id = normalize(entry.spellId);
            auto slots = entry.additionalSlots;
            slots.push_back(entry.slot);
            auto preparation = [&]() -> Preparation
            {
                // Legacy saves do not contain a historical result and still need current grants.
                if (!entry.evaluation) return checkedPreparation(slots, id, PreparationValidation::Restoring, entry.metamagic);
                auto evaluation = restorePreparationSnapshot(id, entry.metamagic, *entry.evaluation, spellListClass());
                if (slots.size() != static_cast<std::size_t>(entry.evaluation->paidSlots))
                    throw std::invalid_argument("saved preparation slot payment does not match its snapshot");
                spellDefinition(id);
                // Historical preparation survives loss of its repertoire source.
                for (auto &slot : slots)
                {
                    slot = checkedSlot(slot);
                    const auto &group = slotPools().at(slot.groupId);
                    if (slot.spellLevel != entry.slot.spellLevel || slot.spellLevel < evaluation.requiredSlotLevel ||
                        (group.allowedSpellIds && !group.allowedSpellIds->contains(id)))
                        throw std::invalid_argument("saved preparation is incompatible with its slots");
                }
                std::ranges::sort(slots);
                return {{PreparedSpellSlotState::Prepared, id, evaluation.applications}, slots, std::move(evaluation)};
            }();
            for (const auto &key : preparation.slots)
                if (!occupied.insert(key).second) throw std::invalid_argument("duplicate prepared spell slot");
            const auto state = entry.consumed ? PreparedSpellSlotState::Consumed : PreparedSpellSlotState::Prepared;
            const auto primary = preparation.slots.front();
            preparation.state.state = state;
            preparation.evaluation.usageCosts.clear();
            loaded.emplace(primary, std::move(preparation));
        }
        return [this, loaded = std::move(loaded)]() mutable noexcept { preparations_.swap(loaded); };
    }

    PreparedSpellcastingView PreparedSpellcasting::toView() const
    {
        PreparedSpellcastingView view{spellListClass(), {}};
        view.metamagic = metamagic().toView();
        for (const auto &[id, grant] : slotPools())
        {
            PreparedSpellSlotGroupView group{grant, {}};
            for (const auto &[level, capacity] : grant.dailyCapacityByLevel)
                for (int index = 0; index < capacity; ++index)
                {
                    const SpellSlotId slot{id, level, static_cast<std::size_t>(index)};
                    const auto state = slotState(slot);
                    std::optional<SpellSlotId> preparationSlot;
                    std::optional<PreparedMetamagicData> evaluation;
                    for (const auto &[primary, preparation] : preparations_)
                        if (std::ranges::find(preparation.slots, slot) != preparation.slots.end())
                        {
                            preparationSlot = primary;
                            evaluation = preparationSnapshot(preparation.evaluation, spellListClass(), static_cast<int>(preparation.slots.size()));
                        }
                    group.slots.push_back({slot, state.state, state.spellId, preparationSlot, state.metamagic, evaluation});
                }
            view.groups.push_back(std::move(group));
        }
        return view;
    }
}
