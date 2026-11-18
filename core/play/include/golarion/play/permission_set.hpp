#pragma once

#include <cstdint>
#include <initializer_list>
#include <stdexcept>

namespace golarion::play
{
    enum class Permission : std::uint64_t
    {
        Any = 0,
        Permission1 = 1ULL << 0,
        Permission2 = 1ULL << 1,
        Permission3 = 1ULL << 2,
        Permission4 = 1ULL << 3,
        Permission5 = 1ULL << 4,
        ChangePermissions = 1ULL << 5
    };

    enum class PermissionPreset
    {
        Master,
        Player,
        Spectator
    };

    class PermissionSet final
    {
    public:
        constexpr PermissionSet() noexcept = default;
        constexpr PermissionSet(Permission permission) : bits_(validatedBit(permission)) {}
        constexpr PermissionSet(std::initializer_list<Permission> permissions)
        {
            for (const auto permission : permissions) bits_ |= validatedBit(permission);
        }

        static constexpr PermissionSet all() noexcept
        {
            return {Permission::Permission1, Permission::Permission2, Permission::Permission3, Permission::Permission4, Permission::Permission5, Permission::ChangePermissions};
        }

        static constexpr PermissionSet fromMask(std::uint64_t mask)
        {
            if ((mask & ~all().bits_) != 0) throw std::invalid_argument("permission mask contains unknown bits");
            PermissionSet result;
            result.bits_ = mask;
            return result;
        }

        constexpr std::uint64_t toMask() const noexcept { return bits_; }

        constexpr bool hasAll(PermissionSet required) const noexcept { return (bits_ & required.bits_) == required.bits_; }
        constexpr bool hasAny(PermissionSet candidates) const noexcept { return (bits_ & candidates.bits_) != 0; }
        constexpr void add(PermissionSet permissions) noexcept { bits_ |= permissions.bits_; }
        constexpr void remove(PermissionSet permissions) noexcept { bits_ &= ~permissions.bits_; }
        constexpr bool operator==(const PermissionSet &) const noexcept = default;

    private:
        static constexpr std::uint64_t validatedBit(Permission permission)
        {
            switch (permission)
            {
                case Permission::Any: throw std::invalid_argument("Any is a command access requirement, not an assignable permission");
                case Permission::Permission1:
                case Permission::Permission2:
                case Permission::Permission3:
                case Permission::Permission4:
                case Permission::Permission5:
                case Permission::ChangePermissions:
                    return static_cast<std::uint64_t>(permission);
            }
            throw std::invalid_argument("unknown permission flag");
        }

        std::uint64_t bits_ = 0;
    };

    constexpr PermissionSet operator|(PermissionSet left, PermissionSet right) noexcept
    {
        left.add(right);
        return left;
    }

    constexpr PermissionSet operator|(Permission left, Permission right)
    {
        return PermissionSet(left) | PermissionSet(right);
    }

    constexpr PermissionSet permissionsForPreset(PermissionPreset preset)
    {
        switch (preset)
        {
            case PermissionPreset::Master: return {Permission::Permission1, Permission::Permission2, Permission::Permission3, Permission::Permission4, Permission::Permission5};
            case PermissionPreset::Player: return {Permission::Permission1, Permission::Permission2, Permission::Permission3};
            case PermissionPreset::Spectator: return {Permission::Permission1};
        }
        throw std::invalid_argument("unknown permission preset");
    }
}
