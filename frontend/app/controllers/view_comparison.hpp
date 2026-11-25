#pragma once

#include <algorithm>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

// Structural comparison of detached view values, without serializing or changing
// the model's public types. Structured bindings make unsupported layouts a build error.
namespace sheet_view_compare
{
    struct Field { template<class T> operator T() const; }; // unevaluated only

    template<class T, std::size_t... I>
    consteval bool acceptsFields(std::index_sequence<I...>)
    {
        return requires { T{(static_cast<void>(I), Field{})...}; };
    }

    template<class T, std::size_t N = 0>
    consteval std::size_t fieldCount()
    {
        if constexpr (acceptsFields<T>(std::make_index_sequence<N + 1>{})) return fieldCount<T, N + 1>();
        else return N;
    }

    template<class T>
    auto fields(const T &value)
    {
        constexpr auto n = fieldCount<T>();
        if constexpr (n == 0) { static_assert(std::is_empty_v<T>); return std::tuple{}; }
        // C++20 has no structured-binding packs. Keep each arity in one place.
#define GH_VIEW_BIND(N, ...) else if constexpr (n == N) { const auto &[__VA_ARGS__] = value; return std::tie(__VA_ARGS__); }
        GH_VIEW_BIND(1, a)
        GH_VIEW_BIND(2, a, b)
        GH_VIEW_BIND(3, a, b, c)
        GH_VIEW_BIND(4, a, b, c, d)
        GH_VIEW_BIND(5, a, b, c, d, e)
        GH_VIEW_BIND(6, a, b, c, d, e, f)
        GH_VIEW_BIND(7, a, b, c, d, e, f, g)
        GH_VIEW_BIND(8, a, b, c, d, e, f, g, h)
        GH_VIEW_BIND(9, a, b, c, d, e, f, g, h, i)
        GH_VIEW_BIND(10, a, b, c, d, e, f, g, h, i, j)
        GH_VIEW_BIND(11, a, b, c, d, e, f, g, h, i, j, k)
        GH_VIEW_BIND(12, a, b, c, d, e, f, g, h, i, j, k, l)
        GH_VIEW_BIND(13, a, b, c, d, e, f, g, h, i, j, k, l, m)
        GH_VIEW_BIND(14, a, b, c, d, e, f, g, h, i, j, k, l, m, n)
        GH_VIEW_BIND(15, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o)
        GH_VIEW_BIND(16, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p)
        GH_VIEW_BIND(17, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q)
        GH_VIEW_BIND(18, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r)
        GH_VIEW_BIND(19, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s)
        GH_VIEW_BIND(20, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t)
        GH_VIEW_BIND(21, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u)
        GH_VIEW_BIND(22, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v)
        GH_VIEW_BIND(23, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w)
        GH_VIEW_BIND(24, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x)
        GH_VIEW_BIND(25, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y)
        GH_VIEW_BIND(26, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z)
        GH_VIEW_BIND(27, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a2)
        GH_VIEW_BIND(28, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a2, b2)
        GH_VIEW_BIND(29, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a2, b2, c2)
        GH_VIEW_BIND(30, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a2, b2, c2, d2)
        GH_VIEW_BIND(31, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a2, b2, c2, d2, e2)
        GH_VIEW_BIND(32, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, a2, b2, c2, d2, e2, f2)
#undef GH_VIEW_BIND
        else static_assert(n <= 32, "Add a decomposition for the new view layout");
    }

    template<class T>
    bool equal(const T &a, const T &b)
    {
        if constexpr (std::is_same_v<T, std::string> || std::is_scalar_v<T>) return a == b;
        else if constexpr (requires { a.has_value(); })
            return bool(a) == bool(b) && (!a || equal(*a, *b));
        else if constexpr (requires { a.index(); })
            return a.index() == b.index() && std::visit([&](const auto &v) {
                return equal(v, std::get<std::decay_t<decltype(v)>>(b));
            }, a);
        else if constexpr (std::ranges::range<T>)
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
                [](const auto &x, const auto &y) { return equal(x, y); });
        else if constexpr (requires { a.first; a.second; })
            return equal(a.first, b.first) && equal(a.second, b.second);
        else if constexpr (std::is_aggregate_v<T>)
        {
            const auto left = fields(a), right = fields(b);
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return (equal(std::get<I>(left), std::get<I>(right)) && ...);
            }(std::make_index_sequence<std::tuple_size_v<decltype(left)>>{});
        }
        else return a == b;
    }
}
