#include "golarion/resource/modifier_group.hpp"
#include "golarion/data/modifier_group_save_data.hpp"
#include "golarion/view/modifier_group_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

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

    Modifier modifier(ModifierType::Bonus, "Cintura", "Bonus alla Forza", BonusType::Enhancement, "2");
    ModifierGroup group(std::vector<TargetedModifier>{TargetedModifier{" str ", modifier}});
    assert(group.modifiers().size() == 1);
    assert(group.modifiers()[0].resourceName == "str");
    assert(group.modifiers()[0].modifier.id() == modifier.id());

    ResourceManager manager;
    ModifierGroupView view = group.toView(manager);
    assert(view.modifiers.size() == 1);
    assert(view.modifiers[0].resourceName == "str");
    assert(view.modifiers[0].modifier.id == modifier.id());
    assert(view.modifiers[0].modifier.resolvedValue == 2);

    ModifierGroupSaveData saveData = group.toSaveData();
    assert(saveData.modifiers.size() == 1);
    assert(saveData.modifiers[0].resourceName == "str");
    assert(saveData.modifiers[0].modifier.id == modifier.id());
    assert(saveData.modifiers[0].modifier.expression == "2");

    assert(throwsInvalidArgument([]
    {
        ModifierGroup empty(std::vector<TargetedModifier>{});
    }));
    assert(throwsInvalidArgument([&]
    {
        ModifierGroup duplicate(std::vector<TargetedModifier>{
            TargetedModifier{"str", modifier},
            TargetedModifier{"dex", modifier}
        });
    }));

    return 0;
}
