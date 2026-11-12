#pragma once
#include "golarion/character/pending_choice.hpp"
#include "golarion/character/class_option_definition.hpp"

#include "golarion/character/class_definition.hpp"
#include "golarion/character/archetype_definition.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/data/classes_data.hpp"
#include "golarion/view/classes_view.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace golarion
{
    class ResourceManager;

    class CharacterClass final
    {
    public:
        CharacterClass(ResourceManager &resourceManager, std::string id, ClassDefinition definition, int level = 1);
        ~CharacterClass();
        CharacterClass(const CharacterClass &) = delete;
        CharacterClass &operator=(const CharacterClass &) = delete;
        CharacterClass(CharacterClass &&) = delete;
        CharacterClass &operator=(CharacterClass &&) = delete;

        void setLevel(int level, std::optional<int> hitDieResult = std::nullopt);
        void setHitDieResult(int classLevel, int result);
        void addArchetype(ArchetypeDefinition archetype);
        void removeArchetype(const std::string &archetypeId);
        int level() const;
        CharacterClassData toData() const;
        void load(const CharacterClassData &data);
        CharacterClassView toView() const;
        void selectOption(const ClassOptionDefinition &definition);
        void unselectOption(const ClassOptionDefinition &definition);

        std::vector<PendingChoice> pendingChoices() const;

    private:
        using ChoiceKey = std::pair<std::string, std::string>;
        using Selections = std::map<ChoiceKey, std::set<std::string>>;
        struct Acquisition
        {
            const ClassFeatureProgressionDefinition *definition;
            std::string parent;
            std::string choice;
            std::string option;
            std::vector<std::string> effectIds;
        };

        void index(const std::vector<ClassFeatureProgressionDefinition> &progression, const std::string &parent, const std::string &choice, const std::string &option);
        std::set<std::string> activeAcquisitions(int level, const Selections &selections) const;
        void update(int level, Selections selections);
        void changeOption(const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId, bool selected);
        const ClassLevelDefinition &levelDefinition(int level) const;
        void rebuildReplacers();

        ResourceManager &resources_;
        std::string id_;
        ClassDefinition definition_;
        std::map<std::string, ArchetypeDefinition> archetypes_;
        std::set<std::string> disabledAcquisitions_;
        int level_ = 0;
        std::map<int, int> hitDieResults_;
        Selections selections_;
        std::map<std::string, Acquisition> acquisitions_;
        std::vector<std::string> acquisitionOrder_;
        std::map<std::string, std::set<std::string>> replacers_;
        std::map<std::string, EffectApply> effects_;
        std::vector<std::string> baseEffectIds_;
        std::vector<std::string> appliedIds_;
        std::map<std::string, EffectCleanup> cleanups_;
        std::vector<std::string> targets_;
    };
}
