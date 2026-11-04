#include "golarion/character/feats.hpp"

#include "golarion/character/feat_definition_manager.hpp"
#include "golarion/resource/expression_parser.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::string_view BasePrefix = "base.feat.level";

    bool accepts(const golarion::FeatSlotFilter &filter, const golarion::FeatDefinition &feat)
    {
        return std::visit([&](const auto &value)
        {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, golarion::AnyFeat>) return true;
            else if constexpr (std::is_same_v<T, golarion::FeatTypeFilter>)
                return std::find(feat.types.begin(), feat.types.end(), value.type) != feat.types.end();
            else return std::find(value.ids.begin(), value.ids.end(), feat.id) != value.ids.end();
        }, filter);
    }
}

namespace golarion
{
    Feats::Feats(ResourceManager &resourceManager)
        : resourceManager_(resourceManager), prerequisites_(resourceManager)
    {
        resourceManager_.registerCollectionResource<FeatSlot>(FeatSlotsResource,
            [this](FeatSlot slot) { addSlot(std::move(slot)); },
            [this](std::string_view id) { removeSlot(id); });
    }

    Feats::~Feats()
    {
        for (auto &[id, slot] : slots_) clear(slot);
        resourceManager_.unregisterCollectionResource(FeatSlotsResource);
    }

    void Feats::clear(SlotState &slot)
    {
        deactivate(slot);
        slot.choices.clear();
        slot.selected = nullptr;
    }

    void Feats::deactivate(SlotState &slot)
    {
        for (auto it = slot.cleanups.rbegin(); it != slot.cleanups.rend(); ++it) (*it)();
        slot.cleanups.clear();
        slot.active = false;
    }

    void Feats::addNormalizedSlot(FeatSlot slot)
    {
        const std::string id = slot.id;
        if (!slots_.emplace(id, SlotState{.slot = std::move(slot), .choices = {}, .cleanups = {}}).second)
            throw std::invalid_argument("feat slot is already registered: " + id);
    }

    void Feats::addSlot(FeatSlot slot)
    {
        slot.id = normalize(slot.id);
        slot.source = normalize(slot.source);
        if (slot.id.starts_with(BasePrefix))
            throw std::invalid_argument("base feat slot IDs are reserved");
        std::visit([](auto &filter)
        {
            using T = std::decay_t<decltype(filter)>;
            if constexpr (std::is_same_v<T, FeatTypeFilter>) filter.type = normalize(filter.type);
            else if constexpr (std::is_same_v<T, FeatIdFilter>)
            {
                if (filter.ids.empty()) throw std::invalid_argument("feat slot ID filter must not be empty");
                std::set<std::string> ids;
                for (auto &id : filter.ids)
                {
                    id = normalize(id);
                    if (!ids.insert(id).second) throw std::invalid_argument("duplicate feat ID in slot filter");
                }
            }
        }, slot.filter);
        addNormalizedSlot(std::move(slot));
    }

    void Feats::removeSlot(std::string_view id)
    {
        const auto key = normalize(id);
        if (key.starts_with(BasePrefix)) throw std::invalid_argument("base feat slots are owned by level progression");
        removeFeat(key);
        slots_.erase(key);
    }

    void Feats::setLevel(int level)
    {
        if (level < 0) throw std::invalid_argument("feat progression level must not be negative");
        // Build new empty slots before changing any active selection.
        auto additions = std::map<std::string, SlotState>{};
        for (long long n = 1; n <= level; n += 2)
        {
            const auto id = std::string(BasePrefix) + std::to_string(n);
            if (n > level_) additions.emplace(id, SlotState{.slot = {.id = id, .source = "Avanzamento del personaggio"}, .choices = {}, .cleanups = {}});
        }
        for (long long n = 1; n <= level_; n += 2)
        {
            if (n > level)
            {
                const auto id = std::string(BasePrefix) + std::to_string(n);
                clear(slots_.at(id));
                slots_.erase(id);
            }
        }
        slots_.merge(additions);
        level_ = level;
        recheckPrerequisites();
    }

    bool Feats::requirementSatisfied(const FeatRequirementDefinition &requirement, const SlotState &slot, const FeatPrerequisiteContext &context) const
    {
        if (slot.slot.exempts && slot.slot.exempts(requirement, context)) return true;
        if (requirement.feat)
        {
            for (const auto &[id, candidate] : slots_)
            {
                if (id == context.slotId || !candidate.active || !candidate.selected || candidate.selected->id != requirement.feat->id) continue;
                bool matches = true;
                for (const auto &[requiredChoice, currentChoice] : requirement.feat->matchingChoices)
                {
                    const auto required = candidate.choices.find(requiredChoice);
                    const auto current = context.choices.find(currentChoice);
                    if (required == candidate.choices.end() || current == context.choices.end() ||
                        !std::is_permutation(required->second.begin(), required->second.end(), current->second.begin(), current->second.end()))
                    { matches = false; break; }
                }
                if (matches) return true;
            }
            return false;
        }
        if (!requirement.expression) return true;
        // Discover targets with the same parser that evaluates the expression.
        // Repeated references share one candidate within each evaluation.
        struct UnresolvedTarget { std::string name; };
        std::map<std::string, int> assigned;
        const std::function<bool()> evaluate = [&]()
        {
            try
            {
                return ExpressionParser::evaluate(*requirement.expression, [&](std::string_view name)
                {
                    const auto key = std::string(name);
                    const auto found = assigned.find(key);
                    if (found == assigned.end()) throw UnresolvedTarget{key};
                    return found->second;
                }) != 0;
            }
            catch (const UnresolvedTarget &target)
            {
                for (const auto &candidate : prerequisites_.values(target.name, context))
                {
                    assigned[target.name] = candidate.value;
                    if (evaluate()) { assigned.erase(target.name); return true; }
                }
                assigned.erase(target.name);
                return false;
            }
        };
        return evaluate();
    }

    bool Feats::qualifies(std::string_view slotId, const FeatDefinition &definition, const FeatChoices &choices) const
    {
        const auto found = slots_.find(normalize(slotId));
        if (found == slots_.end()) throw std::invalid_argument("feat slot is not registered");
        const auto &slot = found->second;
        if (!accepts(slot.slot.filter, definition)) return false;
        const FeatPrerequisiteContext context{definition, slot.slot.id, slot.slot.source, choices};
        for (const auto &requirement : definition.requirements)
            if (!requirementSatisfied(requirement, slot, context)) return false;
        if (choices.size() != definition.choices.size()) return false;
        for (const auto &choice : definition.choices)
        {
            const auto selection = choices.find(choice.id);
            if (selection == choices.end() || selection->second.size() != choice.selectionCount) return false;
            std::set<std::string> ids;
            for (const auto &id : selection->second)
            {
                if (!ids.insert(id).second) return false;
                const auto option = std::find_if(choice.options.begin(), choice.options.end(), [&](const auto &value) { return value.id == id; });
                if (option == choice.options.end()) return false;
                for (const auto &requirement : option->requirements)
                    if (!requirementSatisfied(requirement, slot, context)) return false;
            }
        }
        return true;
    }

    void Feats::selectFeat(std::string_view slotId, std::string_view featId, FeatChoices choices)
    {
        recheckPrerequisites();
        select(slotId, featId, std::move(choices), false);
        try { recheckPrerequisites(); }
        catch (...) { clear(slots_.at(normalize(slotId))); throw; }
    }

    void Feats::select(std::string_view slotId, std::string_view featId, FeatChoices choices, bool restoring)
    {
        const auto key = normalize(slotId);
        const auto found = slots_.find(key);
        if (found == slots_.end()) throw std::invalid_argument("feat slot is not registered: " + key);
        auto &slot = found->second;
        if (slot.selected) throw std::invalid_argument("feat slot is occupied; remove its feat first");
        const auto &definition = FeatDefinitionManager::instance().get(featId);
        FeatChoices normalized;
        for (const auto &[id, options] : choices)
        {
            std::vector<std::string> normalizedOptions;
            for (const auto &option : options) normalizedOptions.push_back(normalize(option));
            std::sort(normalizedOptions.begin(), normalizedOptions.end());
            if (!normalized.emplace(normalize(id), std::move(normalizedOptions)).second)
                throw std::invalid_argument("duplicate normalized feat choice ID");
        }
        for (const auto &[id, other] : slots_)
            if (other.selected && other.selected->id == definition.id &&
                (definition.repeatability == FeatRepeatability::Once ||
                 (definition.repeatability == FeatRepeatability::DifferentChoices && other.choices == normalized)))
                throw std::invalid_argument("feat repeatability does not allow this acquisition");
        // Validate structure independently: an acquired feat may lack prerequisites on load.
        if (!accepts(slot.slot.filter, definition) || normalized.size() != definition.choices.size())
            throw std::invalid_argument("invalid feat slot or choices");
        for (const auto &choice : definition.choices)
        {
            const auto selection = normalized.find(choice.id);
            if (selection == normalized.end() || selection->second.size() != choice.selectionCount)
                throw std::invalid_argument("invalid feat choice selection count");
            std::set<std::string> ids;
            for (const auto &id : selection->second)
                if (!ids.insert(id).second || std::none_of(choice.options.begin(), choice.options.end(), [&](const auto &option) { return option.id == id; }))
                    throw std::invalid_argument("invalid feat choice option");
        }
        const bool qualified = !restoring && qualifies(key, definition, normalized);
        if (!qualified && !restoring)
            throw std::invalid_argument("feat selection does not satisfy slot, choice, or prerequisite requirements");
        slot.choices = std::move(normalized);
        slot.selected = &definition;
        try { if (qualified) activate(slot); }
        catch (...) { clear(slot); throw; }
    }

    void Feats::activate(SlotState &slot)
    {
        if (slot.active) return;
        const auto &key = slot.slot.id;
        const auto &definition = *slot.selected;
        const auto &normalized = slot.choices;
        std::vector<EffectApply> applications;
        const auto compile = [&](const auto &effects, const std::string &path)
        {
            for (const auto &effect : effects)
            {
                // Length-prefixed components prevent collisions between dotted IDs.
                const auto part = [](const std::string &value) { return std::to_string(value.size()) + ":" + value; };
                applications.push_back(compileEffect(effect, {.instanceId = "feat:" + part(key) + part(definition.id) + part(path) + part(effectId(effect)), .source = definition.name}));
            }
        };
        compile(definition.effects, "direct");
        for (const auto &choice : definition.choices)
            for (const auto &option : choice.options)
                if (std::find(normalized.at(choice.id).begin(), normalized.at(choice.id).end(), option.id) != normalized.at(choice.id).end())
                    compile(option.effects, std::to_string(choice.id.size()) + ":" + choice.id + option.id);
        std::vector<EffectCleanup> cleanups;
        cleanups.reserve(applications.size());
        try
        {
            for (const auto &apply : applications) cleanups.push_back(apply(resourceManager_));
        }
        catch (...)
        {
            for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) (*it)();
            throw;
        }
        slot.cleanups = std::move(cleanups);
        slot.active = true;
    }

    void Feats::recheckPrerequisites()
    {
        // Start without feat benefits: cycles cannot sustain themselves after a prerequisite loss.
        for (auto &[id, slot] : slots_) deactivate(slot);
        std::set<std::vector<bool>> visited;
        try
        {
            for (;;)
            {
                std::vector<bool> current, next;
                for (const auto &[id, slot] : slots_)
                {
                    current.push_back(slot.active);
                    next.push_back(slot.selected && qualifies(id, *slot.selected, slot.choices));
                }
                if (current == next) return;
                if (!visited.insert(current).second)
                    throw std::logic_error("feat prerequisites do not converge");
                std::size_t index = 0;
                for (auto &[id, slot] : slots_)
                {
                    if (next[index++]) activate(slot);
                    else deactivate(slot);
                }
            }
        }
        catch (...)
        {
            for (auto &[id, slot] : slots_) deactivate(slot);
            throw;
        }
    }

    void Feats::removeFeat(std::string_view slotId)
    {
        const auto found = slots_.find(normalize(slotId));
        if (found == slots_.end()) throw std::invalid_argument("feat slot is not registered");
        clear(found->second);
        recheckPrerequisites();
    }

    FeatsView Feats::toView() const
    {
        FeatsView view;
        for (const auto &[id, slot] : slots_)
        {
            FeatSlotView slotView{id, slot.slot.source, slot.selected ? std::optional(slot.selected->id) : std::nullopt, std::nullopt, {}};
            std::visit([&](const auto &filter)
            {
                using T = std::decay_t<decltype(filter)>;
                if constexpr (std::is_same_v<T, FeatTypeFilter>) slotView.allowedType = filter.type;
                else if constexpr (std::is_same_v<T, FeatIdFilter>) slotView.allowedFeatIds = filter.ids;
            }, slot.slot.filter);
            view.slots.push_back(std::move(slotView));
            if (!slot.selected) continue;
            const auto &definition = *slot.selected;
            FeatView feat{id, definition.id, definition.name, definition.description, definition.types, {}, slot.active};
            for (const auto &choice : definition.choices)
            {
                FeatChoiceView choiceView{choice.id, choice.prompt, {}};
                for (const auto &optionId : slot.choices.at(choice.id))
                {
                    const auto option = std::find_if(choice.options.begin(), choice.options.end(), [&](const auto &value) { return value.id == optionId; });
                    choiceView.selectedOptions.push_back({option->id, option->name});
                }
                feat.choices.push_back(std::move(choiceView));
            }
            view.feats.push_back(std::move(feat));
        }
        return view;
    }

    FeatsData Feats::toData() const
    {
        FeatsData data;
        for (const auto &[id, slot] : slots_)
            if (slot.selected) data.selections.push_back({id, slot.selected->id, slot.choices});
        return data;
    }

    void Feats::load(const FeatsData &data)
    {
        for (const auto &[id, slot] : slots_)
            if (slot.selected) throw std::invalid_argument("feat loading requires empty slots");
        std::vector<std::string> restored;
        restored.reserve(data.selections.size());
        try
        {
            for (const auto &selection : data.selections)
            {
                // Store the ID before applying effects so allocation cannot break rollback.
                restored.push_back(normalize(selection.slotId));
                try { select(restored.back(), selection.featId, selection.choices, true); }
                catch (...) { restored.pop_back(); throw; }
            }
            recheckPrerequisites();
        }
        catch (...)
        {
            for (auto it = restored.rbegin(); it != restored.rend(); ++it) clear(slots_.at(*it));
            throw;
        }
    }
}
