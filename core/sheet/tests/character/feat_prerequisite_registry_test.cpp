#include "golarion/character/feat_prerequisite_registry.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Function>
    void expectInvalid(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    int bab = 3;
    int classLevel = 6;
    resources.registerTarget("bab", [&] { return bab; });
    resources.registerTarget("class.test.level", [&] { return classLevel; });
    resources.registerTarget("int", [] { return 10; });
    const FeatDefinition combat{.id = "test", .name = "Prova", .description = "Prova.", .types = {"combat"}};
    const FeatDefinition general{.id = "other", .name = "Altro", .description = "Altro.", .types = {"general"}};
    const FeatPrerequisiteContext bonusContext{combat, "class.bonus1", "Classe di prova", {{"weapon", {"dagger"}}}};
    const FeatPrerequisiteContext ordinaryContext{combat, "base.level1", "Regole base"};
    const FeatPrerequisiteContext otherContext{general, "class.bonus1", "Classe di prova"};

    {
        FeatPrerequisiteRegistry registry(resources);
        assert(registry.values("bab", bonusContext).size() == 1);
        assert(!registry.satisfies("bab", bonusContext, [](int value) { return value >= 6; }));
        FeatPrerequisiteAlternative alternative{
            .id = " class.bab ",
            .source = " Classe di prova ",
            .target = " bab ",
            .applies = [](const auto &context)
            {
                return context.slotId == "class.bonus1" &&
                    std::find(context.feat.types.begin(), context.feat.types.end(), "combat") != context.feat.types.end();
            },
            .calculate = [](const auto &, const FeatTargetResolver &read)
            {
                return read("class.test.level");
            }
        };
        resources.addToCollection(FeatPrerequisiteAlternativesResource, alternative);
        assert(registry.satisfies("bab", bonusContext, [](int value) { return value >= 6; }));
        assert(!registry.satisfies("bab", ordinaryContext, [](int value) { return value >= 6; }));
        assert(!registry.satisfies("bab", otherContext, [](int value) { return value >= 6; }));
        assert(resources.targetValue("bab") == 3);
        const auto candidates = registry.values(" bab ", bonusContext);
        assert(!candidates[0].alternativeId && !candidates[0].source);
        assert(candidates[1].alternativeId == "class.bab");
        assert(candidates[1].source == "Classe di prova");
        expectInvalid([&] { resources.addToCollection(FeatPrerequisiteAlternativesResource, alternative); });
        assert(registry.values("bab", bonusContext).size() == 2);

        auto second = alternative;
        second.id = "second";
        second.calculate = [](const auto &, const FeatTargetResolver &read) { return read("bab") + 1; };
        resources.addToCollection(FeatPrerequisiteAlternativesResource, second);
        assert(registry.values("bab", bonusContext)[2].value == 4);
        assert(!registry.satisfies("bab", bonusContext, [](int value) { return value >= 7; }));
        // Each candidate is tested, rather than retaining only the maximum.
        assert(registry.satisfies("bab", bonusContext, [](int value) { return value == 4; }));
        bab = 8;
        classLevel = 2;
        assert(registry.satisfies("bab", bonusContext, [](int value) { return value == 8; }));
        assert(registry.values("bab", bonusContext)[1].value == 2);
        resources.removeFromCollection(FeatPrerequisiteAlternativesResource, " class.bab ");
        assert(registry.values("bab", bonusContext).size() == 2);
        resources.removeFromCollection(FeatPrerequisiteAlternativesResource, "second");
        assert(registry.values("bab", bonusContext).size() == 1);
        expectInvalid([&] { resources.removeFromCollection(FeatPrerequisiteAlternativesResource, "second"); });

        auto specific = alternative;
        specific.id = "choice";
        specific.target = "int";
        specific.applies = [](const auto &context)
        {
            return context.feat.id == "test" && context.choices.contains("weapon") &&
                context.choices.at("weapon") == std::vector<std::string>{"dagger"};
        };
        specific.calculate = [](const auto &, const FeatTargetResolver &) { return 13; };
        resources.addToCollection(FeatPrerequisiteAlternativesResource, specific);
        assert(registry.satisfies("int", bonusContext, [](int value) { return value >= 13; }));
        assert(!registry.satisfies("int", ordinaryContext, [](int value) { return value >= 13; }));

        for (int field = 0; field < 5; ++field)
        {
            auto invalid = alternative;
            if (field == 0) invalid.id = " ";
            if (field == 1) invalid.source = " ";
            if (field == 2) invalid.target = " ";
            if (field == 3) invalid.applies = {};
            if (field == 4) invalid.calculate = {};
            expectInvalid([&] { resources.addToCollection(FeatPrerequisiteAlternativesResource, invalid); });
        }
        expectInvalid([&] { registry.values("missing", bonusContext); });
        expectInvalid([&] { registry.satisfies("bab", bonusContext, {}); });
        auto failing = alternative;
        failing.id = "failing";
        failing.calculate = [](const auto &, const FeatTargetResolver &read) { return read("missing"); };
        resources.addToCollection(FeatPrerequisiteAlternativesResource, failing);
        expectInvalid([&] { registry.values("bab", bonusContext); });
        resources.removeFromCollection(FeatPrerequisiteAlternativesResource, "failing");
        assert(registry.values("bab", bonusContext).size() == 1);
    }
    // Destruction removes callbacks capturing the old registry.
    FeatPrerequisiteRegistry replacement(resources);
    assert(replacement.values("bab", ordinaryContext).size() == 1);
}
