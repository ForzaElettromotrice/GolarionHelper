#include "golarion/play/resource_manager.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }

    void registrationAndInvocation()
    {
        golarion::play::ResourceManager resources;
        resources.registerFeature<int(int, int)>(" math.sum ", [](int left, int right) { return left + right; });
        assert(resources.invoke<int(int, int)>("math.sum", 2, 3) == 5);
        assert(resources.invoke<int(int, int)>("\tmath.sum\n", 4, 5) == 9);

        resources.registerFeature<std::string(std::string_view)>("greeting", [](std::string_view name) { return "Hello " + std::string(name); });
        assert(resources.invoke<std::string(std::string_view)>("greeting", "Alice") == "Hello Alice");

        resources.registerFeature<int()>("counter", [value = 0]() mutable { return ++value; });
        assert(resources.invoke<int()>("counter") == 1);
        assert(resources.invoke<int()>("counter") == 2);

        resources.registerFeature<int()>("Counter", [] { return 10; });
        assert(resources.invoke<int()>("Counter") == 10);

        int value = 4;
        resources.registerFeature<void(int &)>("increment", [](int &target) { ++target; });
        resources.invoke<void(int &)>("increment", value);
        assert(value == 5);

        resources.registerFeature<int &()>("reference", [&value]() -> int & { return value; });
        resources.invoke<int &()>("reference") = 8;
        assert(value == 8);

        resources.registerFeature<int(std::unique_ptr<int>)>("consume", [](std::unique_ptr<int> input) { return *input; });
        auto input = std::make_unique<int>(12);
        assert(resources.invoke<int(std::unique_ptr<int>)>("consume", std::move(input)) == 12);
        assert(!input);
    }

    void invalidOperations()
    {
        golarion::play::ResourceManager resources;
        for (const std::string_view name : {"", " \t\r\n\f\v"})
        {
            assert(rejects([&] { resources.registerFeature<int()>(name, [] { return 1; }); }));
            assert(rejects([&] { resources.invoke<int()>(name); }));
            assert(rejects([&] { resources.unregisterFeature(name); }));
        }

        assert(rejects([&] { resources.registerFeature<int()>("empty", std::function<int()>{}); }));
        int (*nullCallback)() = nullptr;
        assert(rejects([&] { resources.registerFeature<int()>("null", nullCallback); }));
        assert(rejects([&] { resources.invoke<int()>("missing"); }));
        assert(rejects([&] { resources.unregisterFeature("missing"); }));

        int calls = 0;
        resources.registerFeature<int(int)>("value", [&calls](int input) { ++calls; return input; });
        assert(rejects([&] { resources.registerFeature<int(int)>(" value ", [](int) { return 99; }); }));
        assert(rejects([&] { resources.registerFeature<void()>("value", [] {}); }));
        assert(rejects([&] { resources.invoke<int()>("value"); }));
        assert(rejects([&] { resources.invoke<void(int)>("value", 1); }));
        assert(rejects([&] { resources.invoke<int(const int &)>("value", 1); }));
        assert(calls == 0);
        assert(resources.invoke<int(int)>("value", 7) == 7);
        assert(calls == 1);
    }

    void removalAndInstanceIsolation()
    {
        golarion::play::ResourceManager first;
        golarion::play::ResourceManager second;
        const auto publish = [](golarion::play::ResourceManager &resources, int value)
        {
            resources.registerFeature<int()>("value", [value] { return value; });
        };
        publish(first, 3);
        publish(second, 9);
        assert(first.invoke<int()>("value") == 3);
        assert(second.invoke<int()>("value") == 9);

        first.unregisterFeature(" value ");
        assert(rejects([&] { first.invoke<int()>("value"); }));
        assert(rejects([&] { first.unregisterFeature("value"); }));
        assert(second.invoke<int()>("value") == 9);
        first.registerFeature<std::string()>("value", [] { return "replacement"; });
        assert(first.invoke<std::string()>("value") == "replacement");
        assert(rejects([&] { first.invoke<int()>("value"); }));
    }

    void nestedCallsAndRemovalDuringInvocation()
    {
        golarion::play::ResourceManager resources;
        resources.registerFeature<int(int)>("double", [](int value) { return value * 2; });
        resources.registerFeature<int(int)>("quadruple", [&resources](int value)
        {
            return resources.invoke<int(int)>("double", resources.invoke<int(int)>("double", value));
        });
        assert(resources.invoke<int(int)>("quadruple", 3) == 12);

        auto lifetime = std::make_shared<int>(17);
        const std::weak_ptr<int> observer = lifetime;
        resources.registerFeature<int()>("once", [&resources, lifetime]
        {
            resources.unregisterFeature("once");
            resources.registerFeature<int()>("once", [] { return 23; });
            return *lifetime;
        });
        lifetime.reset();
        assert(!observer.expired());
        assert(resources.invoke<int()>("once") == 17);
        assert(observer.expired());
        assert(resources.invoke<int()>("once") == 23);
    }

    void callbackExceptions()
    {
        golarion::play::ResourceManager resources;
        resources.registerFeature<void()>("failure", [] { throw std::runtime_error("callback failed"); });
        bool propagated = false;
        try { resources.invoke<void()>("failure"); }
        catch (const std::runtime_error &error) { propagated = std::string(error.what()) == "callback failed"; }
        assert(propagated);
        resources.unregisterFeature("failure");
        resources.registerFeature<int()>("failure", [] { return 1; });
        assert(resources.invoke<int()>("failure") == 1);
    }
}

int main()
{
    registrationAndInvocation();
    invalidOperations();
    removalAndInstanceIsolation();
    nestedCallsAndRemovalDuringInvocation();
    callbackExceptions();
}
