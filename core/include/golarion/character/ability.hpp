#pragma once

#include <string_view>

namespace golarion
{
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

    std::string_view displayName(AbilityType type);
    std::string_view resourceName(AbilityType type);

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
        AbilityType type_;
        int baseValue_;
    };
}
