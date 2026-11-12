#include "golarion/character/favored_classes.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace golarion
{
    FavoredClasses::FavoredClasses(ResourceManager &resources) : resources_(resources)
    {
        options_.emplace(std::string(HitPointOption), FavoredClassBonusOption{
            .id = std::string(HitPointOption), .name = "+1 PF", .description = "Un punto ferita aggiuntivo.",
            .effects = {ContributionEffectDefinition{"hitPoint", "hp.max", "1"}}
        });
        options_.emplace(std::string(SkillRankOption), FavoredClassBonusOption{
            .id = std::string(SkillRankOption), .name = "+1 grado", .description = "Un grado di abilità aggiuntivo.",
            .effects = {ContributionEffectDefinition{"skillRank", "skill.ranks.max", "1"}}
        });
        resources_.registerCollectionResource<FavoredClassGrant>(GrantsResource, [this](auto grant) { addGrant(std::move(grant)); }, [this](auto id) { removeGrant(id); });
        try
        {
            resources_.registerCollectionResource<FavoredClassBonusOption>(OptionsResource, [this](auto option) { addOption(std::move(option)); }, [this](auto id) { removeOption(id); });
        }
        catch (...) { resources_.unregisterCollectionResource(GrantsResource); throw; }
    }

    FavoredClasses::~FavoredClasses()
    {
        clearState();
        resources_.unregisterCollectionResource(OptionsResource);
        resources_.unregisterCollectionResource(GrantsResource);
    }

    void FavoredClasses::clearState()
    {
        for (auto it = applied_.rbegin(); it != applied_.rend(); ++it) it->second();
        applied_.clear();
        selections_.clear();
        selected_.clear();
        levels_.clear();
    }

    int FavoredClasses::capacity() const
    {
        long long result = 1;
        for (const auto &[id, grant] : grants_) result += grant.additionalChoices;
        if (result > std::numeric_limits<int>::max()) throw std::overflow_error("favored class choice capacity exceeds integer range");
        return static_cast<int>(result);
    }

    bool FavoredClasses::favored(CharacterClassType type) const
    {
        for (const auto &[id, grant] : grants_) if (grant.allClasses || grant.classes.contains(type)) return true;
        return selected_.size() <= static_cast<std::size_t>(capacity()) && selected_.contains(type);
    }

    bool FavoredClasses::available(CharacterClassType type, const std::string &id) const
    {
        const auto found = options_.find(id);
        if (!favored(type) || found == options_.end()) return false;
        const auto &option = found->second;
        if (!option.classes.empty() && !option.classes.contains(type)) return false;
        if (id == HitPointOption || id == SkillRankOption)
        {
            const auto base = id == HitPointOption ? FavoredClassBaseBonus::HitPoint : FavoredClassBaseBonus::SkillRank;
            for (const auto &[key, other] : options_)
                if ((other.classes.empty() || other.classes.contains(type)) && other.replaces.contains(base)) return false;
        }
        return true;
    }

    void FavoredClasses::reconcile()
    {
        if (reconciling_) throw std::logic_error("favored class effects must not modify favored class registrations");
        reconciling_ = true;
        struct Guard { bool &value; ~Guard() { value = false; } } guard{reconciling_};
        std::map<std::pair<CharacterClassType, std::string>, int> counts;
        for (const auto &[key, id] : selections_)
            if (levels_.contains(key.first) && key.second <= levels_.at(key.first) && available(key.first, id)) ++counts[{key.first, id}];
        std::map<EffectKey, EffectApply> wanted;
        for (const auto &[key, count] : counts)
        {
            const auto &[type, id] = key;
            const auto &option = options_.at(id);
            for (int unit = 0; unit < count / option.selectionsPerBenefit; ++unit)
                for (std::size_t index = 0; index < option.effects.size(); ++index)
                {
                    EffectKey effectKey{type, id, unit, index};
                    if (applied_.contains(effectKey)) { wanted.emplace(effectKey, EffectApply{}); continue; }
                    const auto instanceId = "favored." + std::string(characterClassId(type)) + "." + std::to_string(id.size()) + ":" + id + "." + std::to_string(unit) + "." + std::to_string(index);
                    wanted.emplace(effectKey, std::visit([&](const auto &effect) { return compileEffect(effect, {instanceId, option.name, option.usagePool}); }, option.effects[index]));
                }
        }
        std::map<EffectKey, EffectCleanup> added;
        try
        {
            for (const auto &[key, apply] : wanted)
            {
                if (applied_.contains(key)) continue;
                auto entry = added.emplace(key, EffectCleanup{}).first;
                entry->second = apply(resources_);
            }
        }
        catch (...)
        {
            for (auto it = added.rbegin(); it != added.rend(); ++it) if (it->second) it->second();
            throw;
        }
        for (auto it = applied_.begin(); it != applied_.end();)
        {
            if (wanted.contains(it->first)) { ++it; continue; }
            it->second();
            it = applied_.erase(it);
        }
        applied_.merge(added);
    }

    void FavoredClasses::setClasses(std::set<CharacterClassType> classes)
    {
        for (auto type : classes) characterClassId(type);
        if (classes.size() > static_cast<std::size_t>(capacity())) throw std::invalid_argument("too many favored classes selected");
        selected_.swap(classes);
        try { reconcile(); } catch (...) { selected_.swap(classes); throw; }
    }

    void FavoredClasses::selectBonus(CharacterClassType type, int classLevel, std::string optionId)
    {
        if (!levels_.contains(type) || classLevel < 1 || classLevel > levels_.at(type) || !available(type, optionId))
            throw std::invalid_argument("favored class bonus is not available for this level");
        auto previous = selections_;
        selections_[{type, classLevel}] = std::move(optionId);
        try { reconcile(); } catch (...) { selections_.swap(previous); throw; }
    }

    void FavoredClasses::clearBonus(CharacterClassType type, int classLevel)
    {
        auto previous = selections_;
        if (!selections_.erase({type, classLevel})) throw std::invalid_argument("favored class bonus is not selected");
        try { reconcile(); } catch (...) { selections_.swap(previous); throw; }
    }

    void FavoredClasses::addGrant(FavoredClassGrant grant)
    {
        if (grant.id.empty() || grant.additionalChoices < 0) throw std::invalid_argument("invalid favored class grant");
        for (auto type : grant.classes) characterClassId(type);
        const auto id = grant.id;
        if (!grants_.emplace(id, std::move(grant)).second) throw std::invalid_argument("duplicate favored class grant");
        try { capacity(); reconcile(); } catch (...) { grants_.erase(id); throw; }
    }

    void FavoredClasses::removeGrant(std::string_view id)
    {
        auto node = grants_.extract(std::string(id));
        if (node.empty()) throw std::invalid_argument("unknown favored class grant");
        try { reconcile(); } catch (...) { grants_.insert(std::move(node)); throw; }
    }

    void FavoredClasses::addOption(FavoredClassBonusOption option)
    {
        if (option.id.empty() || option.id.starts_with("base.") || option.selectionsPerBenefit < 1) throw std::invalid_argument("invalid favored class bonus option");
        for (auto type : option.classes) characterClassId(type);
        for (auto base : option.replaces)
            if (base != FavoredClassBaseBonus::HitPoint && base != FavoredClassBaseBonus::SkillRank) throw std::invalid_argument("unknown favored class base bonus");
        std::set<std::string> ids;
        for (const auto &effect : option.effects) std::visit([&](const auto &value)
        {
            if (!ids.insert(value.id).second) throw std::invalid_argument("duplicate favored class benefit ID");
            compileEffect(value, {"favored.validation", option.name, option.usagePool});
        }, effect);
        const auto id = option.id;
        if (!options_.emplace(id, std::move(option)).second) throw std::invalid_argument("duplicate favored class bonus option");
        try { reconcile(); } catch (...) { options_.erase(id); throw; }
    }

    void FavoredClasses::removeOption(std::string_view id)
    {
        if (id == HitPointOption || id == SkillRankOption) throw std::invalid_argument("base favored class options cannot be removed");
        auto node = options_.extract(std::string(id));
        if (node.empty()) throw std::invalid_argument("unknown favored class bonus option");
        try { reconcile(); } catch (...) { options_.insert(std::move(node)); throw; }
    }

    FavoredClassesData FavoredClasses::toData() const
    {
        FavoredClassesData data{selected_, {}};
        for (const auto &[key, id] : selections_) data.bonuses.push_back({key.first, key.second, id});
        return data;
    }

    void FavoredClasses::load(const FavoredClassesData &data)
    {
        if (!selections_.empty() || !selected_.empty()) throw std::logic_error("favored class restoration requires empty selections");
        for (auto type : data.selectedClasses) characterClassId(type);
        std::map<SelectionKey, std::string> restored;
        for (const auto &choice : data.bonuses)
            if (!levels_.contains(choice.type) || choice.classLevel < 1 || choice.classLevel > levels_.at(choice.type) || choice.optionId.empty() ||
                !restored.emplace(SelectionKey{choice.type, choice.classLevel}, choice.optionId).second)
                throw std::invalid_argument("invalid or duplicate saved favored class bonus");
        selected_ = data.selectedClasses;
        selections_.swap(restored);
        try { reconcile(); }
        catch (...) { selections_.clear(); selected_.clear(); throw; }
    }

    FavoredClassesView FavoredClasses::toView() const
    {
        FavoredClassesView view{selected_, capacity(), selected_.size() > static_cast<std::size_t>(capacity()), {}};
        for (const auto &[type, level] : levels_)
        {
            FavoredClassView entry{type, favored(type), favored(type) ? level : 0, favored(type) ? level : 0, {}, {}};
            std::map<std::string, int> counts;
            for (const auto &[key, id] : selections_) if (key.first == type)
            {
                const bool active = available(type, id);
                entry.selections.push_back({key.second, id, active});
                if (active) { ++counts[id]; --entry.unassignedLevels; }
            }
            for (const auto &[id, option] : options_)
                if (option.classes.empty() || option.classes.contains(type))
                    entry.options.push_back({id, option.name, option.description, available(type, id), option.selectionsPerBenefit, counts[id], counts[id] / option.selectionsPerBenefit});
            view.classes.push_back(std::move(entry));
        }
        return view;
    }
    std::vector<PendingChoice> FavoredClasses::pendingChoices() const
    {
        std::vector<PendingChoice> result;
        const auto maximum = static_cast<std::size_t>(capacity());
        if (selected_.size() < maximum)
            result.push_back({FavoredClassChoiceReference{}, "Seleziona le classi preferite", maximum - selected_.size(), true});
        for (const auto &[type, count] : levels_)
        {
            if (!favored(type)) continue;
            for (int level = 1; level <= count; ++level)
            {
                const auto found = selections_.find({type, level});
                if (found == selections_.end() || !available(type, found->second))
                    result.push_back({FavoredClassBonusChoiceReference{type, level},
                        "Seleziona il bonus di classe preferita", 1, false});
            }
        }
        return result;
    }

}
