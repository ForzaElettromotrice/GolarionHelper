#include "golarion/equipment/item_definition_manager.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Exception, typename Function>
    bool throws(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const Exception &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;

    ItemDefinitionManager &manager = ItemDefinitionManager::instance();
    assert(&manager == &ItemDefinitionManager::instance());
    const ItemDefinition &rope = manager.get(" hempRope15m ");
    const ItemDefinition &sameRope = manager.get("hempRope15m");
    assert(&rope == &sameRope);
    assert(rope.id == "hempRope15m");
    assert(rope.name == "Corda di canapa (15 m)");
    assert(rope.weightGrams == 5000);
    assert(throws<std::invalid_argument>([&manager]
    {
        static_cast<void>(manager.get("missing"));
    }));

    return 0;
}
