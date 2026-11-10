#pragma once

#include "golarion/character/character_class.hpp"
#include "golarion/character/class_definition_manager.hpp"
#include "golarion/character/favored_classes.hpp"

#include <map>
#include <memory>

namespace golarion
{
    class Classes final
    {
    public:
        Classes(ResourceManager &resourceManager, FavoredClasses &favoredClasses, const ClassDefinitionManager &definitions = ClassDefinitionManager::instance());
        ~Classes();
        Classes(const Classes &) = delete;
        Classes &operator=(const Classes &) = delete;
        Classes(Classes &&) = delete;
        Classes &operator=(Classes &&) = delete;

        void addLevel(CharacterClassType type, std::optional<int> hitDieResult = std::nullopt);
        void setHitDieResult(CharacterClassType type, int classLevel, int result);
        void setFavoredClass(CharacterClassType type);
        void setFavoredClasses(std::set<CharacterClassType> types);
        void selectFavoredClassBonus(CharacterClassType type, int classLevel, std::string optionId);
        void clearFavoredClassBonus(CharacterClassType type, int classLevel);
        void loadFavoredClasses(const FavoredClassesData &data);
        void addArchetype(CharacterClassType type, const std::string &archetypeId);
        void removeArchetype(CharacterClassType type, const std::string &archetypeId);
        int level() const;
        ClassesData toData() const;
        ClassesView toView() const;
        // Restore class sources only; call loadFavoredClasses after reconstructing other providers.
        void load(const ClassesData &data);
        void selectOption(CharacterClassType type, const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId);
        void unselectOption(CharacterClassType type, const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId);

    private:
        CharacterClass &characterClass(CharacterClassType type);

        ResourceManager &resources_;
        FavoredClasses &favoredClasses_;
        const ClassDefinitionManager &definitions_;
        std::map<CharacterClassType, std::unique_ptr<CharacterClass>> classes_;
    };
}
