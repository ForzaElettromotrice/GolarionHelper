#pragma once

#include "golarion/character/temporary_hit_points.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view MaxHitPointsResource = "hp.max";

    class ResourceManager;
    struct HitPointsSaveData;
    struct HitPointsView;

    enum class DamageLethality
    {
        Lethal,
        NonLethal
    };

    std::string_view displayName(DamageLethality lethality);

    class HitPoints final
    {
    public:
        explicit HitPoints(ResourceManager &resourceManager);

        void setMax(int value);
        void setCurrent(int value);
        void setNonLethal(int value);
        void addTemporary(std::string id, int amount, std::optional<GameDuration> duration);
        void removeTemporary(std::string_view id);
        void heal(int amount);
        void damage(int amount, DamageLethality lethality);
        HitPointsView toView();
        HitPointsSaveData toSaveData() const;

    private:
        friend class CharacterSheet;

        void connectConditionEntries();
        void initializeConditionEntries();
        void reconcileConditionEntries();
        void setConditionEntryActive(bool shouldBeActive, std::string_view entryId, std::string_view conditionId, std::string_view source, bool &isActive);
        int maxValue();
        void load(const HitPointsSaveData &data);

        ResourceManager &resourceManager_;
        int baseMax_;
        int damageTaken_;
        TemporaryHitPoints temporary_;
        int nonLethal_;
        bool dead_;
        bool initialized_;
        bool conditionEntriesConnected_;
        bool disabledEntryActive_;
        bool staggeredEntryActive_;
        bool unconsciousEntryActive_;
        bool deadEntryActive_;
    };
}
