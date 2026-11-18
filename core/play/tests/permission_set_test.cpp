#include "golarion/play/permission_set.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace
{
    using namespace golarion::play;

    constexpr std::array<Permission, 6> Flags{Permission::Permission1, Permission::Permission2, Permission::Permission3, Permission::Permission4, Permission::Permission5, Permission::ChangePermissions};

    void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

    template<typename Function> bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    void constructionAndValueSemantics()
    {
        static_assert(std::is_trivially_copyable_v<PermissionSet>);
        static_assert(sizeof(PermissionSet) == sizeof(std::uint64_t));
        constexpr auto selected = Permission::Permission1 | Permission::Permission3 | Permission::Permission5;
        static_assert(selected.hasAll(Permission::Permission1 | Permission::Permission5));
        static_assert(!selected.hasAny(Permission::Permission2 | Permission::ChangePermissions));
        static_assert(PermissionSet::all().hasAll(selected));
        static_assert(PermissionSet{}.hasAll({}));
        static_assert(!PermissionSet{}.hasAny({}));
        static_assert(static_cast<std::uint64_t>(Permission::Any) == 0);
        static_assert(PermissionSet::fromMask(0) == PermissionSet{});
        static_assert(PermissionSet::fromMask(63) == PermissionSet::all());
        static_assert(selected.toMask() == 21);

        auto copy = selected;
        copy.remove(Permission::Permission1 | Permission::Permission3);
        copy.add(Permission::Permission2);
        check(copy == PermissionSet{Permission::Permission2, Permission::Permission5}, "add/remove lost unrelated flags");
        check(selected == PermissionSet{Permission::Permission1, Permission::Permission3, Permission::Permission5}, "copy shared mutable state");
        copy.add(Permission::Permission2);
        copy.remove(Permission::Permission4);
        check(copy == PermissionSet{Permission::Permission2, Permission::Permission5}, "add/remove are not idempotent");
        copy.remove(PermissionSet::all());
        check(copy == PermissionSet{}, "removing all flags left permissions behind");
        check(PermissionSet{Permission::Permission1, Permission::Permission1} == PermissionSet(Permission::Permission1), "duplicate entries changed the set");
        for (std::uint64_t mask = 0; mask < 64; ++mask) check(PermissionSet::fromMask(mask).toMask() == mask, "numeric mask did not round-trip");
        for (const std::uint64_t mask : {64ULL, 127ULL, 1ULL << 63, ~0ULL}) check(rejects([&] { PermissionSet::fromMask(mask); }), "numeric import accepted unknown bits");
        check(rejects([] { PermissionSet{Permission::Any}; }), "command-only Any became assignable");
        check(rejects([] { auto mixed = Permission::Any | Permission::Permission1; static_cast<void>(mixed); }), "Any was combined with assignable flags");
    }

    void membershipForAllCombinations()
    {
        std::array<PermissionSet, 64> sets;
        std::array<std::set<Permission>, 64> expected;
        for (std::size_t index = 0; index < sets.size(); ++index)
        {
            for (std::size_t flag = 0; flag < Flags.size(); ++flag)
            {
                if ((index >> flag) % 2 == 0) continue;
                sets[index].add(Flags[flag]);
                expected[index].insert(Flags[flag]);
            }
        }
        for (std::size_t owned = 0; owned < sets.size(); ++owned)
        {
            for (std::size_t required = 0; required < sets.size(); ++required)
            {
                const bool all = std::includes(expected[owned].begin(), expected[owned].end(), expected[required].begin(), expected[required].end());
                const bool any = std::any_of(expected[required].begin(), expected[required].end(), [&](Permission permission) { return expected[owned].contains(permission); });
                check(sets[owned].hasAll(sets[required]) == all, "hasAll disagrees with set membership");
                check(sets[owned].hasAny(sets[required]) == any, "hasAny disagrees with set membership");
            }
        }
    }

    void invalidFlagsAndPresets()
    {
        for (const std::uint64_t invalid : {0ULL, 3ULL, 33ULL, 64ULL, 1ULL << 63, ~0ULL})
        {
            const auto permission = static_cast<Permission>(invalid);
            check(rejects([&] { PermissionSet set(permission); }), "unknown or combined enum flag accepted");
            check(rejects([&] { PermissionSet set{Permission::Permission1, permission}; }), "invalid initializer-list flag accepted");
            auto set = PermissionSet(Permission::Permission2);
            check(rejects([&] { set.add(permission); }), "invalid flag added");
            check(set == PermissionSet(Permission::Permission2), "failed addition changed the original set");
            check(rejects([&] { set.remove(permission); }), "invalid flag removed");
            check(rejects([&] { set.hasAll(permission); }), "invalid required flag accepted");
        }
        check(permissionsForPreset(PermissionPreset::Spectator) == PermissionSet(Permission::Permission1), "wrong Spectator preset");
        check(permissionsForPreset(PermissionPreset::Player) == PermissionSet{Permission::Permission1, Permission::Permission2, Permission::Permission3}, "wrong Player preset");
        check(permissionsForPreset(PermissionPreset::Master) == PermissionSet{Permission::Permission1, Permission::Permission2, Permission::Permission3, Permission::Permission4, Permission::Permission5}, "wrong Master preset");
        for (const auto preset : {PermissionPreset::Spectator, PermissionPreset::Player, PermissionPreset::Master})
        {
            check(!permissionsForPreset(preset).hasAny(Permission::ChangePermissions), "preset grants ChangePermissions");
        }
        check(rejects([] { permissionsForPreset(static_cast<PermissionPreset>(-1)); }), "unknown negative preset accepted");
        check(rejects([] { permissionsForPreset(static_cast<PermissionPreset>(99)); }), "unknown preset accepted");
    }
}

int main()
{
    try
    {
        constructionAndValueSemantics();
        membershipForAllCombinations();
        invalidFlagsAndPresets();
        std::cout << "PermissionSet: PASS\n";
        return 0;
    }
    catch (const std::exception &error) { std::cerr << "PermissionSet: " << error.what() << '\n'; return 1; }
}
