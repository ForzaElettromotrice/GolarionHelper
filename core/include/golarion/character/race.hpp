#pragma once

#include "golarion/character/race_definition.hpp"
#include "golarion/data/race_save_data.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/view/race_view.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace golarion
{
    class ResourceManager;

    class Race final
    {
    public:
        explicit Race(ResourceManager &resourceManager);
        ~Race();

        Race(const Race &) = delete;
        Race &operator=(const Race &) = delete;
        Race(Race &&) = delete;
        Race &operator=(Race &&) = delete;

        void setRace(std::string_view raceDefinitionId);
        void clearRace();
        void selectAlternateFeature(std::string_view featureId);
        void removeAlternateFeature(std::string_view featureId);
        void setChoice(std::string_view elementId, std::string_view choiceId, std::vector<std::string> optionIds);
        RaceView toView() const;
        RaceSaveData toSaveData() const;
        void load(const RaceSaveData &data);

    private:
        struct CompiledEffect
        {
            std::string id;
            EffectApply apply;
        };

        using ChoiceKey = std::pair<std::string, std::string>;
        using ChoiceSelections = std::map<ChoiceKey, std::vector<std::string>>;
        using CompiledEffects = std::map<std::string, CompiledEffect>;

        std::vector<const RacialElementDefinition *> activeElements(const RaceDefinition &definition, const std::vector<std::string> &alternateFeatureIds) const;
        CompiledEffects compileEffects(const RaceDefinition &definition) const;
        std::vector<std::string> desiredEffectIds(const RaceDefinition &definition, const std::vector<std::string> &alternateFeatureIds, const ChoiceSelections &choices) const;
        void transitionEffects(const std::vector<std::string> &desiredIds, const CompiledEffects &desiredCompiledEffects);

        ResourceManager &resourceManager_;
        std::optional<std::reference_wrapper<const RaceDefinition>> definition_;
        std::vector<std::string> alternateFeatureIds_;
        ChoiceSelections choices_;
        CompiledEffects compiledEffects_;
        std::map<std::string, EffectCleanup> appliedCleanups_;
        std::vector<std::string> appliedEffectIds_;
    };
}
