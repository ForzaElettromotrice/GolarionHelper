#include "golarion/resource/requirement.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/requirement_view.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;

    ResourceManager manager;
    manager.registerTarget("armor.worn", []
    {
        return 0;
    });

    Requirement requirement("  @armor.worn == 0  ", "  Non applicabile mentre indossi un'armatura  ");
    assert(requirement.expression() == "@armor.worn == 0");
    assert(requirement.failureReason() == "Non applicabile mentre indossi un'armatura");
    assert(requirement.isSatisfied(manager));

    const RequirementView view = requirement.toView(manager);
    assert(view.expression == "@armor.worn == 0");
    assert(view.failureReason == "Non applicabile mentre indossi un'armatura");
    assert(view.satisfied);

    assert(throwsInvalidArgument([]
    {
        Requirement invalid(" ", "Motivazione");
    }));
    assert(throwsInvalidArgument([]
    {
        Requirement invalid("1", " ");
    }));

    return 0;
}
