#pragma once

#include "golarion/character/feat_definition.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;

    inline constexpr std::string_view FeatPrerequisiteAlternativesResource = "feat.prerequisiteAlternatives";

    struct FeatPrerequisiteContext
    {
        const FeatDefinition &feat;
        std::string_view slotId;
        std::string_view slotSource;
        std::map<std::string, std::vector<std::string>> choices{};
    };

    using FeatTargetResolver = std::function<int(std::string_view)>;

    struct FeatPrerequisiteAlternative
    {
        std::string id;
        std::string source;
        std::string target;
        std::function<bool(const FeatPrerequisiteContext &)> applies;
        std::function<int(const FeatPrerequisiteContext &, const FeatTargetResolver &)> calculate;
    };

    struct FeatPrerequisiteValue
    {
        int value;
        std::optional<std::string> alternativeId;
        std::optional<std::string> source;
    };

    // Resolves one target's permitted values; exemptions and complete requirements
    // belong to the caller. Callbacks must be read-only and must not reenter the registry.
    class FeatPrerequisiteRegistry final
    {
    public:
        explicit FeatPrerequisiteRegistry(ResourceManager &resourceManager);
        ~FeatPrerequisiteRegistry();
        FeatPrerequisiteRegistry(const FeatPrerequisiteRegistry &) = delete;
        FeatPrerequisiteRegistry &operator=(const FeatPrerequisiteRegistry &) = delete;
        FeatPrerequisiteRegistry(FeatPrerequisiteRegistry &&) = delete;
        FeatPrerequisiteRegistry &operator=(FeatPrerequisiteRegistry &&) = delete;

        std::vector<FeatPrerequisiteValue> values(std::string_view target, const FeatPrerequisiteContext &context) const;
        bool satisfies(std::string_view target, const FeatPrerequisiteContext &context, const std::function<bool(int)> &requirement) const;

    private:
        void add(FeatPrerequisiteAlternative alternative);
        void remove(std::string_view id);

        ResourceManager &resourceManager_;
        std::map<std::string, FeatPrerequisiteAlternative> alternatives_;
    };
}
