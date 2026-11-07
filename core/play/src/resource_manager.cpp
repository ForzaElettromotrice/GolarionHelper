#include "golarion/play/resource_manager.hpp"

namespace golarion::play
{
    std::string ResourceManager::normalizedName(std::string_view name)
    {
        constexpr std::string_view whitespace = " \t\r\n\f\v";
        const auto begin = name.find_first_not_of(whitespace);
        if (begin == std::string_view::npos)
        {
            throw std::invalid_argument("feature name must not be empty");
        }
        const auto end = name.find_last_not_of(whitespace);
        return std::string(name.substr(begin, end - begin + 1));
    }

    void ResourceManager::unregisterFeature(std::string_view name)
    {
        const std::string key = normalizedName(name);
        if (features_.erase(key) == 0)
        {
            throw std::invalid_argument("feature is not registered: " + key);
        }
    }
}
