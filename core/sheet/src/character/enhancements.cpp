#include "golarion/character/enhancements.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/resource/resource_manager.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace golarion
{
    namespace
    {
        std::string key(std::string_view value) { return std::to_string(value.size()) + ":" + std::string(value); }
        std::string source(std::string_view id) { return "enhancement.group." + key(normalize(id)); }
        std::optional<int> numericValue(const EffectDefinition &effect, ResourceManager &resources)
        {
            return std::visit([&](const auto &definition) -> std::optional<int>
            {
                using T = std::decay_t<decltype(definition)>;
                if constexpr (std::is_same_v<T, ContributionEffectDefinition> || std::is_same_v<T, ModifierEffectDefinition>)
                {
                    const int value = resources.evaluateExpression(definition.expression);
                    if constexpr (std::is_same_v<T, ModifierEffectDefinition>)
                        if (value < 0) throw std::invalid_argument("enhancement modifier magnitude must not be negative");
                    return value;
                }
                else return std::nullopt;
            }, effect);
        }
    }

    std::string enhancementEffectId(std::string_view groupId, std::string_view effectId)
    {
        return source(groupId) + key(normalize(effectId));
    }

    EnhancementGroup::EnhancementGroup(ResourceManager &resources, EnhancementGroupDefinition definition)
        : resources_(resources), data_{normalize(definition.id), normalize(definition.name), {}, false, std::move(definition.description), std::move(definition.origin)}
    {
        if (data_.origin)
        {
            switch (data_.origin->type)
            {
            case EnhancementOriginType::Custom: case EnhancementOriginType::Spell:
            case EnhancementOriginType::Item: case EnhancementOriginType::Ability: break;
            default: throw std::invalid_argument("unknown enhancement origin type");
            }
            if (data_.origin->definitionId) data_.origin->definitionId = normalize(*data_.origin->definitionId);
            if (data_.origin->sourceSheetId) data_.origin->sourceSheetId = normalize(*data_.origin->sourceSheetId);
        }
        for (auto &effect : definition.enhancements) add(std::move(effect));
        setActive(definition.active);
    }

    EnhancementGroup::~EnhancementGroup() { deactivate(); }

    void EnhancementGroup::deactivate()
    {
        while (!cleanups_.empty())
        {
            cleanups_.back()();
            cleanups_.pop_back();
        }
        data_.active = false;
    }

    void EnhancementGroup::setActive(bool active)
    {
        if (active == data_.active) return;
        if (!active) { deactivate(); return; }
        cleanups_.reserve(data_.enhancements.size());
        try
        {
            for (const auto &effect : data_.enhancements)
            {
                auto apply = compileEffect(effect, {enhancementEffectId(data_.id, effectId(effect)), source(data_.id)});
                cleanups_.push_back(apply(resources_));
                numericValue(effect, resources_);
            }
            data_.active = true;
        }
        catch (...) { deactivate(); throw; }
    }

    void EnhancementGroup::add(EffectDefinition definition)
    {
        if (data_.active) throw std::invalid_argument("cannot edit an active enhancement group");
        // Reuse the shared structural schema; resource references are checked on activation.
        definition = effectDefinitionFromJson(effectDefinitionToJson(definition));
        const auto id = effectId(definition);
        if (std::ranges::any_of(data_.enhancements, [&](const auto &entry) { return effectId(entry) == id; }))
            throw std::invalid_argument("duplicate enhancement ID: " + id);
        compileEffect(definition, {enhancementEffectId(data_.id, id), source(data_.id)});
        data_.enhancements.push_back(std::move(definition));
    }

    void EnhancementGroup::remove(std::string_view id)
    {
        if (data_.active) throw std::invalid_argument("cannot edit an active enhancement group");
        const auto normalized = normalize(id);
        const auto found = std::ranges::find_if(data_.enhancements, [&](const auto &effect) { return effectId(effect) == normalized; });
        if (found == data_.enhancements.end()) throw std::invalid_argument("unknown enhancement ID");
        data_.enhancements.erase(found);
    }

    EnhancementGroupData EnhancementGroup::toData() const { return data_; }

    EnhancementGroupView EnhancementGroup::toView() const
    {
        EnhancementGroupView view{data_.id, data_.name, {}, data_.active, data_.description, data_.origin};
        for (const auto &effect : data_.enhancements)
            view.enhancements.push_back({effect, data_.active ? numericValue(effect, resources_) : std::nullopt});
        return view;
    }

    Enhancements::Enhancements(ResourceManager &resources) : resources_(resources) {}
    Enhancements::~Enhancements() { while (!groups_.empty()) groups_.pop_back(); }

    EnhancementGroup &Enhancements::group(std::string_view id)
    {
        const auto normalized = normalize(id);
        const auto found = std::ranges::find(groups_, normalized, &Group::id);
        if (found == groups_.end()) throw std::invalid_argument("unknown enhancement group ID");
        return *found->instance;
    }

    void Enhancements::addGroup(EnhancementGroupDefinition definition)
    {
        const auto id = normalize(definition.id);
        if (std::ranges::find(groups_, id, &Group::id) != groups_.end()) throw std::invalid_argument("duplicate enhancement group ID");
        groups_.reserve(groups_.size() + 1);
        auto instance = std::make_unique<EnhancementGroup>(resources_, std::move(definition));
        groups_.push_back({id, std::move(instance)});
    }

    void Enhancements::removeGroup(std::string_view id)
    {
        group(id).setActive(false);
        const auto normalized = normalize(id);
        groups_.erase(std::ranges::find(groups_, normalized, &Group::id));
    }
    void Enhancements::add(std::string_view groupId, EffectDefinition definition) { group(groupId).add(std::move(definition)); }
    void Enhancements::remove(std::string_view groupId, std::string_view id) { group(groupId).remove(id); }
    void Enhancements::setGroupActive(std::string_view groupId, bool active) { group(groupId).setActive(active); }

    EnhancementsData Enhancements::toData() const
    {
        EnhancementsData data;
        for (const auto &group : groups_) data.groups.push_back(group.instance->toData());
        return data;
    }

    EnhancementsView Enhancements::toView() const
    {
        EnhancementsView view;
        for (const auto &group : groups_) view.groups.push_back(group.instance->toView());
        return view;
    }
}
