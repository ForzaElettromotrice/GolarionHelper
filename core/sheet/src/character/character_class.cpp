#include "golarion/character/character_class.hpp"

#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/character/hit_points.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace
{
    int checkedTotal(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
            throw std::overflow_error("class contribution exceeds integer range");
        return static_cast<int>(value);
    }

    std::string effectKey(const std::string &owner, const std::string &acquisition, const std::string &effect)
    {
        return "class.effect." + std::to_string(owner.size()) + ":" + owner + std::to_string(acquisition.size()) + ":" + acquisition + std::to_string(effect.size()) + ":" + effect;
    }

    void bindLevel(std::string &expression, const std::string &target)
    {
        const std::string token = "@classLevel";
        std::size_t position = 0;
        while ((position = expression.find(token, position)) != std::string::npos)
        {
            const auto end = position + token.size();
            if (end == expression.size() || !(std::isalnum(static_cast<unsigned char>(expression[end])) || expression[end] == '_' || expression[end] == '.'))
            {
                expression.replace(position, token.size(), "@" + target);
                position += target.size() + 1;
            }
            else position = end;
        }
    }

    void bindLevel(std::optional<std::string> &expression, const std::string &target)
    {
        if (expression) bindLevel(*expression, target);
    }

    template<class Effect> void bindEffectLevel(Effect &value, const std::string &target)
    {
        if constexpr (requires { value.expression; }) bindLevel(value.expression, target);
        if constexpr (requires { value.condition; }) bindLevel(value.condition, target);
        if constexpr (requires { value.baseSpeedExpression; }) bindLevel(value.baseSpeedExpression, target);
        if constexpr (requires { value.amountExpression; }) bindLevel(value.amountExpression, target);
        if constexpr (requires { value.attackPenaltyExpression; }) bindLevel(value.attackPenaltyExpression, target);
        if constexpr (requires { value.countExpression; }) bindLevel(value.countExpression, target);
        if constexpr (requires { value.attackBonusAdjustmentExpression; }) bindLevel(value.attackBonusAdjustmentExpression, target);
        if constexpr (requires { value.baseAttackBonusAdjustmentExpression; }) bindLevel(value.baseAttackBonusAdjustmentExpression, target);
        if constexpr (requires { value.progression; }) bindEffectLevel(value.progression, target);
        if constexpr (requires { value.progressions; }) for (auto &entry : value.progressions) bindEffectLevel(entry, target);
        if constexpr (requires { value.slots; }) for (auto &entry : value.slots) bindEffectLevel(entry, target);
        if constexpr (requires { value.requirement; })
        {
            auto expression = value.requirement.expression();
            bindLevel(expression, target);
            value.requirement = golarion::Requirement(std::move(expression), value.requirement.failureReason());
        }
        if constexpr (requires { value.requirements; }) for (auto &requirement : value.requirements)
        {
            if constexpr (requires { requirement.expression(); })
            {
                auto expression = requirement.expression();
                bindLevel(expression, target);
                requirement = golarion::Requirement(std::move(expression), requirement.failureReason());
            }
        }
        if constexpr (requires { value.entry; }) std::visit([&](auto &entry) { bindEffectLevel(entry, target); }, value.entry);
        if constexpr (requires { value.effects; }) for (auto &entry : value.effects)
            std::visit([&](auto &effect) { bindEffectLevel(effect, target); }, entry);
    }
}

namespace golarion
{
    CharacterClass::CharacterClass(ResourceManager &resourceManager, std::string id, ClassDefinition definition, int level)
        : resources_(resourceManager), id_(std::move(id)), definition_(std::move(definition))
    {
        if (id_.empty() || !std::ranges::all_of(id_, [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '.'; }))
            throw std::invalid_argument("class instance ID must contain only letters, digits, underscores or dots");
        if (definition_.skillRanksPerLevel < 0) throw std::invalid_argument("class skill ranks must not be negative");
        switch (definition_.hitDie)
        {
            case ClassHitDie::D6: case ClassHitDie::D8: case ClassHitDie::D10: case ClassHitDie::D12: break;
            default: throw std::invalid_argument("unknown class hit die");
        }
        std::set<int> levels;
        for (const auto &row : definition_.progression)
        {
            if (row.level <= 0 || row.baseAttackBonus < 0 || !levels.insert(row.level).second || row.baseSavingThrows.size() != 3)
                throw std::invalid_argument("invalid class level table");
            for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will})
                if (!row.baseSavingThrows.contains(type) || row.baseSavingThrows.at(type) < 0)
                    throw std::invalid_argument("invalid class base saving throw");
        }
        if (level < 0) throw std::invalid_argument("class level must not be negative");
        if (level > 0) levelDefinition(level);
        std::set<std::string> features;
        for (const auto &feature : definition_.features)
        {
            if (feature.id.empty() || !features.insert(feature.id).second) throw std::invalid_argument("invalid class feature ID");
            index(feature.progression, {}, {}, {});
        }
        rebuildReplacers();

        const std::string prefix = "class." + id_ + ".";
        auto target = [&](const std::string &name, std::function<int()> supplier)
        {
            resources_.registerTarget(name, std::move(supplier));
            targets_.push_back(name);
        };
        auto base = [&](const std::string &name, const std::string &resource, std::function<int()> supplier)
        {
            target(prefix + name, std::move(supplier));
            const auto key = effectKey(id_, "", name);
            effects_.emplace(key, compileEffect(ContributionEffectDefinition{name, resource, "@" + prefix + name}, {key, definition_.name}));
            baseEffectIds_.push_back(key);
        };
        try
        {
            target(prefix + "level", [this] { return level_; });
            base("hitPoints", std::string(MaxHitPointsResource), [this]
            {
                if (hitDieResults_.empty()) return 0;
                const long long constitution = resources_.evaluateExpression("@conMod");
                long long total = 0;
                // HitPoints already adds Constitution for every level. Only add dice and the per-die floor correction here.
                for (const auto &[level, result] : hitDieResults_)
                    total += result + std::max(0LL, 1LL - result - constitution);
                return checkedTotal(total);
            });
            base("skillRanks", std::string(MaxSkillRanksResource), [this]
            {
                if (level_ == 0) return 0;
                const long long intelligence = resources_.targetValue(SkillRankIntelligenceModifierTarget);
                return checkedTotal(level_ * std::max(1LL, definition_.skillRanksPerLevel + intelligence));
            });
            base("bab", std::string(BaseAttackBonusResource), [this] { return level_ == 0 ? 0 : levelDefinition(level_).baseAttackBonus; });
            for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will})
                base(std::string(resourceName(type)), baseResourceName(type), [this, type] { return level_ == 0 ? 0 : levelDefinition(level_).baseSavingThrows.at(type); });
            for (auto skill : definition_.classSkills)
            {
                const auto key = effectKey(id_, "", "skill." + std::string(resourceName(skill)));
                effects_.emplace(key, [this, skill, key](ResourceManager &resources) -> EffectCleanup
                {
                    resources.addToCollection(SkillClassSkillGrantsResource, SkillClassSkillGrant({key, definition_.name, std::string(resourceName(skill))}));
                    return [&resources, key] { resources.removeFromCollection(SkillClassSkillGrantsResource, key); };
                });
                baseEffectIds_.push_back(key);
            }
            update(level, {});
        }
        catch (...)
        {
            for (auto it = targets_.rbegin(); it != targets_.rend(); ++it) resources_.unregisterTarget(*it);
            throw;
        }
    }

    void CharacterClass::rebuildReplacers()
    {
        replacers_.clear();
        // Expand replacement chains: replacing an upgrade must not revive its predecessor.
        std::function<void(const std::string &, const std::string &, std::set<std::string>)> replace;
        replace = [&](const std::string &source, const std::string &target, std::set<std::string> path)
        {
            if (!acquisitions_.contains(target) || !path.insert(target).second)
                throw std::invalid_argument("unknown or cyclic class acquisition replacement");
            replacers_[target].insert(source);
            for (const auto &next : acquisitions_.at(target).definition->replaces) replace(source, next, path);
        };
        for (const auto &[key, acquisition] : acquisitions_)
            for (const auto &target : acquisition.definition->replaces) replace(key, target, {key});

        // Validate dependency cycles even in currently unselected branches.
        std::set<std::string> visiting, visited;
        std::function<void(const std::string &)> validate = [&](const std::string &key)
        {
            if (visited.contains(key)) return;
            if (!visiting.insert(key).second) throw std::invalid_argument("cyclic class acquisition dependencies");
            const auto &parent = acquisitions_.at(key).parent;
            if (!parent.empty()) validate(parent);
            for (const auto &replacer : replacers_[key]) validate(replacer);
            visiting.erase(key);
            visited.insert(key);
        };
        for (const auto &[key, acquisition] : acquisitions_) validate(key);
    }

    CharacterClass::~CharacterClass()
    {
        for (auto it = appliedIds_.rbegin(); it != appliedIds_.rend(); ++it) cleanups_.at(*it)();
        for (auto it = targets_.rbegin(); it != targets_.rend(); ++it) resources_.unregisterTarget(*it);
    }

    const ClassLevelDefinition &CharacterClass::levelDefinition(int level) const
    {
        const auto row = std::ranges::find(definition_.progression, level, &ClassLevelDefinition::level);
        if (row == definition_.progression.end()) throw std::invalid_argument("class level is not defined");
        return *row;
    }

    void CharacterClass::index(const std::vector<ClassFeatureProgressionDefinition> &progression, const std::string &parent, const std::string &choice, const std::string &option)
    {
        for (const auto &stage : progression)
        {
            if (stage.id.empty() || stage.level <= 0 || !acquisitions_.emplace(stage.id, Acquisition{&stage, parent, choice, option, {}}).second)
                throw std::invalid_argument("invalid or duplicate class acquisition");
            acquisitionOrder_.push_back(stage.id);
            auto &acquisition = acquisitions_.at(stage.id);
            for (auto effect : stage.effects)
            {
                const auto key = effectKey(id_, stage.id, effectId(effect));
                std::visit([this](auto &value)
                {
                    const auto target = "class." + id_ + ".level";
                    bindEffectLevel(value, target);
                }, effect);
                if (!effects_.emplace(key, compileEffect(std::move(effect), {key, definition_.name})).second)
                    throw std::invalid_argument("duplicate class effect ID");
                acquisition.effectIds.push_back(key);
            }
            std::set<std::string> choices;
            for (const auto &entry : stage.choices)
            {
                if (entry.id.empty() || !choices.insert(entry.id).second || entry.selectionCount == 0 || entry.selectionCount > entry.options.size())
                    throw std::invalid_argument("invalid class choice");
                std::set<std::string> options;
                for (const auto &value : entry.options)
                {
                    if (value.id.empty() || !options.insert(value.id).second) throw std::invalid_argument("invalid class option");
                    index(value.progression, stage.id, entry.id, value.id);
                }
            }
        }
    }

    std::set<std::string> CharacterClass::activeAcquisitions(int level, const Selections &selections) const
    {
        std::map<std::string, bool> cache;
        std::function<bool(const std::string &)> active = [&](const std::string &key)
        {
            if (cache.contains(key)) return cache.at(key);
            const auto &entry = acquisitions_.at(key);
            bool result = level >= entry.definition->level && !disabledAcquisitions_.contains(key);
            if (result && !entry.parent.empty())
            {
                const auto selection = selections.find({entry.parent, entry.choice});
                result = active(entry.parent) && selection != selections.end() && selection->second.contains(entry.option);
            }
            if (result)
            {
                const auto found = replacers_.find(key);
                if (found != replacers_.end()) for (const auto &replacer : found->second) if (active(replacer)) { result = false; break; }
            }
            cache.emplace(key, result);
            return result;
        };
        std::set<std::string> result;
        for (const auto &key : acquisitionOrder_) if (active(key)) result.insert(key);
        return result;
    }

    CharacterClassData CharacterClass::toData() const
    {
        CharacterClassData result{definition_.type, level_, {}};
        result.hitDieResults = hitDieResults_;
        for (const auto &[id, archetype] : archetypes_) result.archetypeIds.push_back(id);
        for (const auto &[key, options] : selections_)
            if (!options.empty()) result.choices.push_back({key.first, key.second, {options.begin(), options.end()}});
        return result;
    }

    void CharacterClass::load(const CharacterClassData &data)
    {
        if (data.type != definition_.type || data.level < 0) throw std::invalid_argument("invalid class save identity or level");
        const std::set<std::string> savedArchetypes(data.archetypeIds.begin(), data.archetypeIds.end());
        if (savedArchetypes.size() != data.archetypeIds.size() || savedArchetypes.size() != archetypes_.size())
            throw std::invalid_argument("saved archetypes do not match installed archetypes");
        for (const auto &id : savedArchetypes) if (!archetypes_.contains(id)) throw std::invalid_argument("saved archetype is not installed");
        if (data.level > 0) levelDefinition(data.level);
        Selections selections;
        for (const auto &saved : data.choices)
        {
            const auto acquisition = acquisitions_.find(saved.acquisitionId);
            if (acquisition == acquisitions_.end()) throw std::invalid_argument("unknown saved class acquisition");
            const auto &choices = acquisition->second.definition->choices;
            const auto choice = std::ranges::find(choices, saved.choiceId, &ClassChoiceDefinition::id);
            if (choice == choices.end() || saved.optionIds.size() > choice->selectionCount) throw std::invalid_argument("invalid saved class choice");
            const ChoiceKey key{saved.acquisitionId, saved.choiceId};
            if (!selections.emplace(key, std::set<std::string>{}).second) throw std::invalid_argument("duplicate saved class choice");
            for (const auto &option : saved.optionIds)
            {
                if (std::ranges::find(choice->options, option, &ClassChoiceOptionDefinition::id) == choice->options.end() || !selections.at(key).insert(option).second)
                    throw std::invalid_argument("invalid or duplicate saved class option");
            }
        }
        auto results = data.hitDieResults;
        for (const auto &[level, result] : results)
            if (level < 1 || level > data.level || result < 1 || result > static_cast<int>(definition_.hitDie))
                throw std::invalid_argument("invalid saved hit die result");
        hitDieResults_.swap(results);
        try { update(data.level, std::move(selections)); }
        catch (...) { hitDieResults_.swap(results); throw; }
    }

    CharacterClassView CharacterClass::toView() const
    {
        CharacterClassView view{definition_.type, definition_.name, level_, {}};
        view.hitDieResults = hitDieResults_;
        const auto active = activeAcquisitions(level_, selections_);
        std::vector<const ClassFeatureDefinition *> features;
        for (const auto &feature : definition_.features) features.push_back(&feature);
        for (const auto &[id, archetype] : archetypes_)
        {
            view.archetypes.push_back({id, archetype.name, archetype.description});
            for (const auto &feature : archetype.addedFeatures) features.push_back(&feature);
        }
        for (const auto *feature : features)
        {
            ClassFeatureView featureView{feature->id, feature->name, feature->description, {}};
            std::function<void(const std::vector<ClassFeatureProgressionDefinition> &)> append = [&](const auto &stages)
            {
                for (const auto &stage : stages)
                {
                    const auto &entry = acquisitions_.at(stage.id);
                    ClassAcquisitionView acquisition{stage.id, stage.level, active.contains(stage.id), entry.parent, entry.choice, entry.option, {}};
                    for (const auto &choice : stage.choices)
                    {
                        const auto selected = selections_.find({stage.id, choice.id});
                        const auto count = selected == selections_.end() ? 0 : selected->second.size();
                        ClassChoiceView choiceView{choice.id, choice.prompt, choice.selectionCount, choice.selectionCount - count, {}};
                        for (const auto &option : choice.options)
                            choiceView.options.push_back({option.id, option.name, option.description, selected != selections_.end() && selected->second.contains(option.id)});
                        acquisition.choices.push_back(std::move(choiceView));
                    }
                    featureView.acquisitions.push_back(std::move(acquisition));
                    for (const auto &choice : stage.choices) for (const auto &option : choice.options) append(option.progression);
                }
            };
            append(feature->progression);
            view.features.push_back(std::move(featureView));
        }
        return view;
    }

    int CharacterClass::level() const
    {
        return level_;
    }

    void CharacterClass::setLevel(int level, std::optional<int> hitDieResult)
    {
        if (level < 0) throw std::invalid_argument("class level must not be negative");
        if (level > 0) levelDefinition(level);
        if (hitDieResult && (level == 0 || *hitDieResult < 1 || *hitDieResult > static_cast<int>(definition_.hitDie)))
            throw std::invalid_argument("hit die result is outside the class die range");
        auto results = hitDieResults_;
        results.erase(results.upper_bound(level), results.end());
        if (hitDieResult) results[level] = *hitDieResult;
        hitDieResults_.swap(results);
        try { update(level, selections_); }
        catch (...) { hitDieResults_.swap(results); throw; }
    }

    void CharacterClass::setHitDieResult(int classLevel, int result)
    {
        if (classLevel < 1 || classLevel > level_ || result < 1 || result > static_cast<int>(definition_.hitDie))
            throw std::invalid_argument("invalid class level or hit die result");
        hitDieResults_[classLevel] = result;
    }

    void CharacterClass::selectOption(const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId)
    {
        changeOption(acquisitionId, choiceId, optionId, true);
    }

    void CharacterClass::unselectOption(const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId)
    {
        changeOption(acquisitionId, choiceId, optionId, false);
    }

    void CharacterClass::changeOption(const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId, bool selected)
    {
        if (!activeAcquisitions(level_, selections_).contains(acquisitionId)) throw std::invalid_argument("class acquisition is not active");
        const auto &choices = acquisitions_.at(acquisitionId).definition->choices;
        const auto choice = std::ranges::find(choices, choiceId, &ClassChoiceDefinition::id);
        if (choice == choices.end()) throw std::invalid_argument("unknown class choice");
        if (std::ranges::find(choice->options, optionId, &ClassChoiceOptionDefinition::id) == choice->options.end()) throw std::invalid_argument("unknown class option");
        auto selections = selections_;
        auto &options = selections[{acquisitionId, choiceId}];
        if (selected)
        {
            if (options.contains(optionId) || options.size() >= choice->selectionCount) throw std::invalid_argument("class choice is full or option is already selected");
            options.insert(optionId);
        }
        else if (options.erase(optionId) == 0) throw std::invalid_argument("class option is not selected");
        update(level_, std::move(selections));
    }

    void CharacterClass::update(int level, Selections selections)
    {
        const auto active = activeAcquisitions(level, selections);
        std::vector<std::string> desired = level > 0 ? baseEffectIds_ : std::vector<std::string>{};
        for (const auto &key : acquisitionOrder_) if (active.contains(key))
        {
            const auto &ids = acquisitions_.at(key).effectIds;
            desired.insert(desired.end(), ids.begin(), ids.end());
        }
        const std::set<std::string> wanted(desired.begin(), desired.end());
        std::vector<std::string> added;
        const int previousLevel = level_;
        level_ = level;
        // Add first so a failing new registration cannot erase old selections or counters.
        try
        {
            for (const auto &key : desired) if (!cleanups_.contains(key))
            {
                auto cleanup = effects_.at(key)(resources_);
                cleanups_.emplace(key, std::move(cleanup));
                added.push_back(key);
            }
        }
        catch (...)
        {
            for (auto it = added.rbegin(); it != added.rend(); ++it) { cleanups_.at(*it)(); cleanups_.erase(*it); }
            level_ = previousLevel;
            throw;
        }
        for (auto it = appliedIds_.rbegin(); it != appliedIds_.rend(); ++it) if (!wanted.contains(*it))
        {
            cleanups_.at(*it)();
            cleanups_.erase(*it);
        }
        appliedIds_ = std::move(desired);
        selections_ = std::move(selections);
    }
}
