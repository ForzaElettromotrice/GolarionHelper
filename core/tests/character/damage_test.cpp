#include "golarion/character/damage.hpp"
#include "golarion/view/damage_view.hpp"

#include <array>
#include <cassert>
#include <stdexcept>
#include <string_view>

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

    const DamageDice damageDice(DamageDiceDefinition{
        .diceCount = 2,
        .dieSize = 6
    });
    assert(damageDice.toString() == "2d6");
    assert(damageDice.toView().diceCount == 2);
    assert(damageDice.toView().dieSize == 6);
    assert(damageDice.toView().expression == "2d6");
    assert(DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 8}).adjustedByProgression(1).toString() == "2d6");
    assert(DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 10}).adjustedByProgression(1).toString() == "2d8");
    assert(DamageDice(DamageDiceDefinition{.diceCount = 2, .dieSize = 6}).adjustedByProgression(-1).toString() == "1d8");
    assert(DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 3}).adjustedByProgression(-1).toString() == "1");
    assert(DamageDice(DamageDiceDefinition{.diceCount = 2, .dieSize = 6}).multiplied(2).toString() == "4d6");
    assert(throwsInvalidArgument([]
    {
        DamageDice(DamageDiceDefinition{.diceCount = 0, .dieSize = 6});
    }));
    assert(throwsInvalidArgument([]
    {
        DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = -6});
    }));

    constexpr std::array damageTypes{
        DamageType::Bludgeoning,
        DamageType::Piercing,
        DamageType::Slashing,
        DamageType::Acid,
        DamageType::Cold,
        DamageType::Electricity,
        DamageType::Fire,
        DamageType::Sonic,
        DamageType::Force,
        DamageType::PositiveEnergy,
        DamageType::NegativeEnergy,
        DamageType::Untyped
    };
    constexpr std::array<std::string_view, damageTypes.size()> damageTypeNames{
        "Contundente",
        "Perforante",
        "Tagliente",
        "Acido",
        "Freddo",
        "Elettricità",
        "Fuoco",
        "Sonoro",
        "Forza",
        "Energia positiva",
        "Energia negativa",
        "Senza tipo"
    };
    for (std::size_t index = 0; index < damageTypes.size(); ++index)
    {
        assert(displayName(damageTypes[index]) == damageTypeNames[index]);
    }
    assert(displayName(DamageTypeMode::All) == "Tutti");
    assert(displayName(DamageTypeMode::Choice) == "A scelta");
    assert(displayName(DamageCriticalRule::Multiplied) == "Moltiplicato nel critico");
    assert(displayName(DamageCriticalRule::NotMultiplied) == "Non moltiplicato nel critico");
    assert(displayName(DamageCriticalRule::CriticalOnly) == "Solo nel critico");
    assert(displayName(DamageTrait::Precision) == "Precisione");
    assert(displayName(DamageTrait::Bleed) == "Sanguinamento");
    assert(displayName(DamageTrait::NonLethal) == "Non letale");
    assert(displayName(DamageComponentRole::Base) == "Base");
    assert(displayName(DamageComponentRole::Additional) == "Aggiuntivo");
    assert(displayName(DamageDiceAdjustmentType::ProgressionSteps) == "Passi nella progressione dei dadi");
    assert(displayName(DamageDiceAdjustmentType::DiceCountMultiplier) == "Moltiplicatore del numero di dadi");
    assert(displayName(DamageDiceAdjustmentType::Set) == "Dadi impostati");

    const DamageComponent weaponDamage(DamageComponentDefinition{
        .id = "weapon",
        .source = "Spada lunga",
        .role = DamageComponentRole::Base,
        .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 8}),
        .types = {DamageType::Slashing},
        .typeMode = DamageTypeMode::All,
        .criticalRule = DamageCriticalRule::Multiplied,
        .traits = {}
    });
    const DamageComponent versatileDamage(DamageComponentDefinition{
        .id = "versatile",
        .source = "Arma versatile",
        .role = DamageComponentRole::Base,
        .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
        .types = {DamageType::Bludgeoning, DamageType::Piercing},
        .typeMode = DamageTypeMode::Choice,
        .criticalRule = DamageCriticalRule::Multiplied,
        .traits = {DamageTrait::NonLethal}
    });
    static_cast<void>(weaponDamage);
    static_cast<void>(versatileDamage);

    assert(throwsInvalidArgument([]
    {
        DamageComponent(DamageComponentDefinition{
            .id = "missingType",
            .source = "Test",
            .role = DamageComponentRole::Additional,
            .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
            .types = {},
            .typeMode = DamageTypeMode::All,
            .criticalRule = DamageCriticalRule::NotMultiplied,
            .traits = {}
        });
    }));
    assert(throwsInvalidArgument([]
    {
        DamageComponent(DamageComponentDefinition{
            .id = "invalidChoice",
            .source = "Test",
            .role = DamageComponentRole::Additional,
            .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
            .types = {DamageType::Fire},
            .typeMode = DamageTypeMode::Choice,
            .criticalRule = DamageCriticalRule::NotMultiplied,
            .traits = {}
        });
    }));
    assert(throwsInvalidArgument([]
    {
        DamageComponent(DamageComponentDefinition{
            .id = "invalidPrecision",
            .source = "Attacco furtivo",
            .role = DamageComponentRole::Additional,
            .dice = DamageDice(DamageDiceDefinition{.diceCount = 1, .dieSize = 6}),
            .types = {DamageType::Piercing},
            .typeMode = DamageTypeMode::All,
            .criticalRule = DamageCriticalRule::Multiplied,
            .traits = {DamageTrait::Precision}
        });
    }));

    return 0;
}
