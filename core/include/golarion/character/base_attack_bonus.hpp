#pragma once

#include <string_view>

namespace golarion
{
    inline constexpr std::string_view BaseAttackBonusResource = "baseAttackBonus";
    inline constexpr std::string_view BaseAttackBonusTarget = "bab";

    class ResourceManager;
    struct BaseAttackBonusView;

    class BaseAttackBonus final
    {
    public:
        explicit BaseAttackBonus(ResourceManager &resourceManager);

        BaseAttackBonusView toView();

    private:
        int total();

        ResourceManager &resourceManager_;
    };
}
