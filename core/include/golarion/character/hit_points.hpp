#pragma once

#include "golarion/character/temporary_hit_points.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
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
        void advanceTime(GameDuration duration);
        void heal(int amount);
        void damage(int amount, DamageLethality lethality);
        HitPointsView toView();
        HitPointsSaveData toSaveData() const;

    private:
        friend class CharacterSheet;

        int maxValue();
        void load(const HitPointsSaveData &data);

        ResourceManager &resourceManager_;
        int baseMax_;
        int damageTaken_;
        TemporaryHitPoints temporary_;
        int nonLethal_;
    };
}
