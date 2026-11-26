#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace golarion::play::detail
{
    template<typename Owner> class DispatchScope final
    {
    public:
        explicit DispatchScope(const Owner &owner) : owner_(owner), previous_(current_) { current_ = this; }
        ~DispatchScope() { current_ = previous_; }
        DispatchScope(const DispatchScope &) = delete;
        DispatchScope &operator=(const DispatchScope &) = delete;

        static bool contains(const Owner &owner) noexcept
        {
            for (auto scope = current_; scope; scope = scope->previous_)
            {
                if (&scope->owner_ == &owner) return true;
            }
            return false;
        }

    private:
        const Owner &owner_;
        const DispatchScope *previous_;
        inline static thread_local const DispatchScope *current_ = nullptr;
    };

    template<typename Owner> void requireOutsideDispatch(const Owner &owner)
    {
        // Lifecycle operations may join the worker; never wait for it from a callback.
        if (DispatchScope<Owner>::contains(owner)) throw std::logic_error("connection lifecycle and identity restoration are not allowed from a command callback");
    }

    inline std::string normalizedName(std::string_view name)
    {
        constexpr std::string_view whitespace = " \t\r\n\f\v";
        const auto begin = name.find_first_not_of(whitespace);
        if (begin == std::string_view::npos)
        {
            throw std::invalid_argument("command name must not be empty");
        }
        const auto end = name.find_last_not_of(whitespace);
        return std::string(name.substr(begin, end - begin + 1));
    }
}
