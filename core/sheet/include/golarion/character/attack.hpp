#pragma once

#include "golarion/data/attacks_data.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class AttackRoutines;
    class Strikes;
    struct StrikeCalculationContext;
    struct AttacksView;

    class Attack final
    {
    public:
        Attack(const Attack &) = default;
        Attack &operator=(const Attack &) = default;
        Attack(Attack &&) noexcept = default;
        Attack &operator=(Attack &&) noexcept = default;

    private:
        friend class Attacks;

        Attack(std::string id, std::string name, std::string routineId);

        std::string id_;
        std::string name_;
        std::string routineId_;
        std::map<std::string, std::string> assignments_;
    };

    class Attacks final
    {
    public:
        Attacks(AttackRoutines &attackRoutines, Strikes &strikes);

        void create(std::string_view id, std::string_view name, std::string_view routineId);
        void remove(std::string_view attackId);
        void assignStrike(std::string_view attackId, std::string_view slotId, std::string_view strikeGrantId);
        void unassignStrike(std::string_view attackId, std::string_view slotId);
        AttacksView toView();
        AttacksView toView(const StrikeCalculationContext &context);
        AttacksData toData() const;
        void load(const AttacksData &data);

    private:
        Attack &attack(std::string_view attackId);

        AttackRoutines &attackRoutines_;
        Strikes &strikes_;
        std::vector<Attack> attacks_;
    };
}
