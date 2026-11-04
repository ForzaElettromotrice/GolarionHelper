#pragma once

#include "golarion/resource/requirement.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view AbilityCheckRootResource = "abilityCheck.all";

    struct AbilitySaveData;
    struct AbilityView;
    class ResourceManager;

    enum class AbilityType
    {
        Strength,
        Dexterity,
        Constitution,
        Intelligence,
        Wisdom,
        Charisma
    };

    enum class AbilityReplacementStage
    {
        Base,
        Final
    };

    std::string_view displayName(AbilityType type);
    std::string_view displayName(AbilityReplacementStage stage);
    std::string_view resourceName(AbilityType type);
    std::string abilityCheckResourceName(AbilityType type);
    std::string abilityReplacementsResourceName(AbilityType type);

    struct AbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string expression;
        AbilityReplacementStage stage;
        std::vector<Requirement> requirements;
    };

    class AbilityReplacement final
    {
    public:
        explicit AbilityReplacement(AbilityReplacementDefinition definition);

    private:
        friend class AbilityScore;

        std::string id_;
        std::string source_;
        std::string expression_;
        AbilityReplacementStage stage_;
        std::vector<Requirement> requirements_;
    };

    class AbilityChecks final
    {
    public:
        explicit AbilityChecks(ResourceManager &resourceManager);
    };

    class AbilityScore final
    {
    public:
        explicit AbilityScore(AbilityType type, int baseValue = 10);

        void setBaseValue(int baseValue);
        void registerResources(ResourceManager &resourceManager) const;
        AbilityView toView(ResourceManager &resourceManager) const;
        AbilitySaveData toSaveData() const;

        int baseValue() const;
        int totalValue(ResourceManager &resourceManager) const;
        int modifier(ResourceManager &resourceManager) const;

    private:
        std::optional<int> replacementValue(AbilityReplacementStage stage, ResourceManager &resourceManager) const;
        void addReplacement(AbilityReplacement replacement) const;
        void removeReplacement(std::string_view replacementId) const;

        AbilityType type_;
        int baseValue_;
        mutable std::map<std::string, AbilityReplacement> replacements_;
    };
}
