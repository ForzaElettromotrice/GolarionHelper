#pragma once

#include <any>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace golarion::play
{
    class ResourceManager final
    {
    public:
        ResourceManager() = default;
        ResourceManager(const ResourceManager &) = delete;
        ResourceManager &operator=(const ResourceManager &) = delete;
        ResourceManager(ResourceManager &&) = delete;
        ResourceManager &operator=(ResourceManager &&) = delete;

        template<typename Signature> void registerFeature(std::string_view name, std::function<Signature> callback);
        template<typename Signature, typename... Arguments> decltype(auto) invoke(std::string_view name, Arguments &&...arguments) const;
        void unregisterFeature(std::string_view name);

    private:
        static std::string normalizedName(std::string_view name);

        std::unordered_map<std::string, std::any> features_;
    };

    template<typename Signature>
    void ResourceManager::registerFeature(std::string_view name, std::function<Signature> callback)
    {
        static_assert(std::is_function_v<Signature>, "feature signature must be a function type");
        const std::string key = normalizedName(name);
        if (!callback)
        {
            throw std::invalid_argument("feature callback must not be empty: " + key);
        }
        if (features_.contains(key))
        {
            throw std::invalid_argument("feature is already registered: " + key);
        }
        features_.emplace(key, std::make_shared<std::function<Signature>>(std::move(callback)));
    }

    template<typename Signature, typename... Arguments>
    decltype(auto) ResourceManager::invoke(std::string_view name, Arguments &&...arguments) const
    {
        static_assert(std::is_function_v<Signature>, "feature signature must be a function type");
        const std::string key = normalizedName(name);
        const auto feature = features_.find(key);
        if (feature == features_.end())
        {
            throw std::invalid_argument("feature is not registered: " + key);
        }
        const auto storedCallback = std::any_cast<std::shared_ptr<std::function<Signature>>>(&feature->second);
        if (!storedCallback)
        {
            throw std::invalid_argument("feature signature does not match: " + key);
        }

        // Keep the original callable alive if it unregisters itself during execution.
        const auto callback = *storedCallback;
        return (*callback)(std::forward<Arguments>(arguments)...);
    }
}
