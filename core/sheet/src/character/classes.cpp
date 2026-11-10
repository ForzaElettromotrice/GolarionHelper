#include "golarion/character/classes.hpp"

#include <limits>
#include <stdexcept>

namespace golarion
{
    Classes::Classes(ResourceManager &resourceManager, FavoredClasses &favoredClasses, const ClassDefinitionManager &definitions)
        : resources_(resourceManager), favoredClasses_(favoredClasses), definitions_(definitions)
    {
    }

    Classes::~Classes()
    {
        // Remove bonus effects before their consumers, and keep endpoints alive for source cleanup.
        favoredClasses_.clearState();
    }

    void Classes::setFavoredClass(CharacterClassType type) { setFavoredClasses({type}); }
    void Classes::setFavoredClasses(std::set<CharacterClassType> types) { favoredClasses_.setClasses(std::move(types)); }
    void Classes::selectFavoredClassBonus(CharacterClassType type, int classLevel, std::string optionId) { favoredClasses_.selectBonus(type, classLevel, std::move(optionId)); }
    void Classes::clearFavoredClassBonus(CharacterClassType type, int classLevel) { favoredClasses_.clearBonus(type, classLevel); }
    void Classes::loadFavoredClasses(const FavoredClassesData &data) { favoredClasses_.load(data); }

    void Classes::addLevel(CharacterClassType type, std::optional<int> hitDieResult)
    {
        if (level() == std::numeric_limits<int>::max()) throw std::overflow_error("total class level exceeds integer range");
        const auto found = classes_.find(type);
        if (found != classes_.end())
        {
            found->second->setLevel(found->second->level() + 1, hitDieResult);
            favoredClasses_.levels_.at(type) = found->second->level();
            return;
        }

        const auto &definition = definitions_.get(type);
        // Reserve the map node before applying effects; erase it on construction failure.
        const auto entry = classes_.emplace(type, nullptr).first;
        try
        {
            favoredClasses_.levels_.emplace(type, 0);
            const auto id = std::string(characterClassId(type));
            auto instance = std::make_unique<CharacterClass>(resources_, id, definition, 0);
            instance->setLevel(1, hitDieResult);
            entry->second = std::move(instance);
            favoredClasses_.levels_.at(type) = 1;
        }
        catch (...)
        {
            classes_.erase(entry);
            favoredClasses_.levels_.erase(type);
            throw;
        }
    }

    void Classes::addArchetype(CharacterClassType type, const std::string &archetypeId)
    {
        characterClass(type).addArchetype(definitions_.getArchetype(archetypeId));
    }

    void Classes::setHitDieResult(CharacterClassType type, int classLevel, int result)
    {
        characterClass(type).setHitDieResult(classLevel, result);
    }

    void Classes::removeArchetype(CharacterClassType type, const std::string &archetypeId)
    {
        characterClass(type).removeArchetype(archetypeId);
    }

    int Classes::level() const
    {
        int total = 0;
        for (const auto &[type, instance] : classes_)
        {
            if (instance) total += instance->level();
        }
        return total;
    }

    ClassesData Classes::toData() const
    {
        ClassesData data;
        data.favoredClasses = favoredClasses_.toData();
        for (const auto &[type, instance] : classes_) data.classes.push_back(instance->toData());
        return data;
    }

    ClassesView Classes::toView() const
    {
        ClassesView view{level(), {}};
        view.favoredClasses = favoredClasses_.toView();
        for (const auto &[type, instance] : classes_) view.classes.push_back(instance->toView());
        return view;
    }

    void Classes::load(const ClassesData &data)
    {
        if (!classes_.empty()) throw std::logic_error("class restoration requires an empty container");
        std::map<CharacterClassType, std::unique_ptr<CharacterClass>> restored;
        long long total = 0;
        for (const auto &entry : data.classes)
        {
            if (entry.level <= 0 || (total += entry.level) > std::numeric_limits<int>::max() || restored.contains(entry.type))
                throw std::invalid_argument("invalid saved class level or duplicate class");
            auto instance = std::make_unique<CharacterClass>(resources_, std::string(characterClassId(entry.type)), definitions_.get(entry.type), 0);
            for (const auto &id : entry.archetypeIds) instance->addArchetype(definitions_.getArchetype(id));
            instance->load(entry);
            restored.emplace(entry.type, std::move(instance));
        }
        std::map<CharacterClassType, int> levels;
        for (const auto &[type, instance] : restored) levels.emplace(type, instance->level());
        classes_ = std::move(restored);
        favoredClasses_.levels_ = std::move(levels);
    }

    CharacterClass &Classes::characterClass(CharacterClassType type)
    {
        const auto found = classes_.find(type);
        if (found == classes_.end() || !found->second) throw std::invalid_argument("character does not have this class");
        return *found->second;
    }

    void Classes::selectOption(CharacterClassType type, const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId)
    {
        characterClass(type).selectOption(acquisitionId, choiceId, optionId);
    }

    void Classes::unselectOption(CharacterClassType type, const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId)
    {
        characterClass(type).unselectOption(acquisitionId, choiceId, optionId);
    }
}
