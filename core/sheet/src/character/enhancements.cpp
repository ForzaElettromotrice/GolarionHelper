#include "golarion/character/enhancements.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <type_traits>

namespace golarion
{
    namespace
    {
        std::string key(std::string_view value)
        {
            return std::to_string(value.size()) + ":" + std::string(value);
        }
    }

    EnhancementGroup::EnhancementGroup(ResourceManager &resources, std::string id, std::string name)
        : resources_(resources), id_(normalize(id)), name_(normalize(name)) {}

    EnhancementGroup::~EnhancementGroup()
    {
        for (auto &[id, entry] : entries_) if (entry.cleanup) entry.cleanup();
    }

    void EnhancementGroup::activate(Entry &entry)
    {
        std::visit([&](const auto &definition)
        {
            using Definition = std::decay_t<decltype(definition)>;
            const auto source = "enhancement.group." + key(id_);
            if constexpr (std::is_same_v<Definition, ContributionEffectDefinition>)
            {
                const auto runtimeId = source + key(definition.id);
                entry.cleanup = [this, resource = definition.resource, runtimeId] { resources_.removeContribution(resource, runtimeId); };
                try { resources_.addContribution(definition.resource, Contribution(runtimeId, definition.expression)); }
                catch (...) { entry.cleanup = {}; throw; }
            }
            else
            {
                Modifier modifier({.type = definition.type, .source = source, .description = definition.description, .bonusType = definition.bonusType, .expression = definition.expression, .condition = definition.condition, .requirements = {}, .skillRankBudget = definition.skillRankBudget});
                entry.cleanup = [this, resource = definition.resource, id = modifier.id()] { resources_.removeModifier(resource, id); };
                try { resources_.addModifier(definition.resource, std::move(modifier)); }
                catch (...) { entry.cleanup = {}; throw; }
            }
            try
            {
                const int value = resources_.evaluateExpression(definition.expression);
                if constexpr (std::is_same_v<Definition, ModifierEffectDefinition>)
                    if (value < 0) throw std::invalid_argument("enhancement modifier magnitude must not be negative");
            }
            catch (...) { entry.cleanup(); entry.cleanup = {}; throw; }
        }, entry.data.definition);
    }

    void EnhancementGroup::add(EnhancementDefinition data)
    {
        const auto id = std::visit([&](auto &definition)
        {
            definition.id = normalize(definition.id);
            definition.resource = normalize(definition.resource);
            definition.expression = normalize(definition.expression);
            // Inactive entries retain their definitions even when a target is unavailable.
            // Runtime expression and resource validation happens on activation.
            if constexpr (std::is_same_v<std::decay_t<decltype(definition)>, ModifierEffectDefinition>)
            {
                displayName(definition.type);
                if (definition.bonusType) stackingRule(*definition.bonusType);
                if (definition.skillRankBudget != SkillRankBudgetPolicy::Included && definition.skillRankBudget != SkillRankBudgetPolicy::Excluded)
                    throw std::invalid_argument("unknown skill rank budget policy");
                definition.description = normalize(definition.description);
                if (definition.condition) definition.condition = normalize(*definition.condition);
                Modifier modifier({.type = definition.type, .source = id_, .description = definition.description, .bonusType = definition.bonusType, .expression = definition.expression, .condition = definition.condition, .requirements = {}, .skillRankBudget = definition.skillRankBudget});
            }
            return definition.id;
        }, data.definition);
        auto [position, inserted] = entries_.try_emplace(id, Entry{EnhancementData{std::move(data.definition), data.active}, {}});
        if (!inserted) throw std::invalid_argument("duplicate enhancement ID: " + id);
        try { if (position->second.data.active) activate(position->second); }
        catch (...) { entries_.erase(position); throw; }
    }

    void EnhancementGroup::remove(std::string_view id)
    {
        auto position = entries_.find(normalize(id));
        if (position == entries_.end()) throw std::invalid_argument("unknown enhancement ID");
        if (position->second.cleanup) position->second.cleanup();
        entries_.erase(position);
    }

    void EnhancementGroup::setActive(std::string_view id, bool active)
    {
        auto position = entries_.find(normalize(id));
        if (position == entries_.end()) throw std::invalid_argument("unknown enhancement ID");
        auto &entry = position->second;
        if (entry.data.active == active) return;
        if (active) activate(entry);
        else { entry.cleanup(); entry.cleanup = {}; }
        entry.data.active = active;
    }

    EnhancementGroupData EnhancementGroup::toData() const
    {
        EnhancementGroupData result{id_, name_, {}};
        for (const auto &[id, entry] : entries_) result.enhancements.push_back(entry.data);
        return result;
    }

    EnhancementGroupView EnhancementGroup::toView() const
    {
        EnhancementGroupView result{id_, name_, {}};
        for (const auto &[id, entry] : entries_)
        {
            std::optional<int> value;
            if (entry.data.active) value = std::visit([&](const auto &definition) { return resources_.evaluateExpression(definition.expression); }, entry.data.definition);
            result.enhancements.push_back({entry.data, value});
        }
        return result;
    }

    Enhancements::Enhancements(ResourceManager &resources) : resources_(resources) {}

    EnhancementGroup &Enhancements::group(std::string_view id)
    {
        auto position = groups_.find(normalize(id));
        if (position == groups_.end()) throw std::invalid_argument("unknown enhancement group ID");
        return *position->second;
    }

    void Enhancements::addGroup(EnhancementGroupDefinition data)
    {
        const auto id = normalize(data.id);
        if (groups_.contains(id)) throw std::invalid_argument("duplicate enhancement group ID: " + id);
        auto group = std::make_unique<EnhancementGroup>(resources_, id, std::move(data.name));
        for (auto &entry : data.enhancements) group->add(std::move(entry));
        groups_.emplace(id, std::move(group));
    }

    void Enhancements::removeGroup(std::string_view id) { group(id); groups_.erase(normalize(id)); }
    void Enhancements::add(std::string_view groupId, EnhancementDefinition data) { group(groupId).add(std::move(data)); }
    void Enhancements::remove(std::string_view groupId, std::string_view id) { group(groupId).remove(id); }
    void Enhancements::setActive(std::string_view groupId, std::string_view id, bool active) { group(groupId).setActive(id, active); }

    EnhancementsData Enhancements::toData() const
    {
        EnhancementsData result;
        for (const auto &[id, group] : groups_) result.groups.push_back(group->toData());
        return result;
    }

    EnhancementsView Enhancements::toView() const
    {
        EnhancementsView result;
        for (const auto &[id, group] : groups_) result.groups.push_back(group->toView());
        return result;
    }
}
