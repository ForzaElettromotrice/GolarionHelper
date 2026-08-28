#include "golarion/character/character_identity.hpp"
#include "golarion/data/character_identity_save_data.hpp"
#include "golarion/view/character_identity_view.hpp"

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

    constexpr std::array AlignmentNames{
        std::pair{Alignment::LawfulGood, std::string_view("Legale Buono")},
        std::pair{Alignment::NeutralGood, std::string_view("Neutrale Buono")},
        std::pair{Alignment::ChaoticGood, std::string_view("Caotico Buono")},
        std::pair{Alignment::LawfulNeutral, std::string_view("Legale Neutrale")},
        std::pair{Alignment::Neutral, std::string_view("Neutrale")},
        std::pair{Alignment::ChaoticNeutral, std::string_view("Caotico Neutrale")},
        std::pair{Alignment::LawfulEvil, std::string_view("Legale Malvagio")},
        std::pair{Alignment::NeutralEvil, std::string_view("Neutrale Malvagio")},
        std::pair{Alignment::ChaoticEvil, std::string_view("Caotico Malvagio")}
    };
    for (const auto &[alignment, name] : AlignmentNames)
    {
        assert(displayName(alignment) == name);
    }

    CharacterIdentity identity;
    CharacterIdentityView view = identity.toView();
    assert(view.name.empty());
    assert(view.playerName.empty());
    assert(!view.alignment.has_value());
    assert(!view.deity.has_value());
    assert(!view.homeland.has_value());
    assert(!view.gender.has_value());
    assert(!view.age.has_value());
    assert(!view.heightCentimeters.has_value());
    assert(!view.weightGrams.has_value());
    assert(!view.hair.has_value());
    assert(!view.eyes.has_value());
    assert(!view.appearance.has_value());

    identity.setName("  Merisiel  ");
    identity.setPlayerName("  Giocatrice  ");
    identity.setAlignment(Alignment::ChaoticNeutral);
    identity.setDeity("  Calistria  ");
    identity.setHomeland("  Varisia  ");
    identity.setGender("  Donna  ");
    identity.setAge(25);
    identity.setHeightCentimeters(173);
    identity.setWeightGrams(62000);
    identity.setHair("  Nero  ");
    identity.setEyes("  Verdi  ");
    identity.setAppearance("  Mantello scuro  ");

    view = identity.toView();
    assert(view.name == "Merisiel");
    assert(view.playerName == "Giocatrice");
    assert(view.alignment == Alignment::ChaoticNeutral);
    assert(view.deity == "Calistria");
    assert(view.homeland == "Varisia");
    assert(view.gender == "Donna");
    assert(view.age == 25);
    assert(view.heightCentimeters == 173);
    assert(view.weightGrams == 62000);
    assert(view.hair == "Nero");
    assert(view.eyes == "Verdi");
    assert(view.appearance == "Mantello scuro");

    const CharacterIdentitySaveData data = identity.toSaveData();
    const CharacterIdentity restored(data);
    const CharacterIdentityView restoredView = restored.toView();
    assert(restoredView.name == view.name);
    assert(restoredView.playerName == view.playerName);
    assert(restoredView.alignment == view.alignment);
    assert(restoredView.deity == view.deity);
    assert(restoredView.homeland == view.homeland);
    assert(restoredView.gender == view.gender);
    assert(restoredView.age == view.age);
    assert(restoredView.heightCentimeters == view.heightCentimeters);
    assert(restoredView.weightGrams == view.weightGrams);
    assert(restoredView.hair == view.hair);
    assert(restoredView.eyes == view.eyes);
    assert(restoredView.appearance == view.appearance);

    identity.setDeity(std::nullopt);
    identity.setAppearance(std::nullopt);
    assert(!identity.toView().deity.has_value());
    assert(!identity.toView().appearance.has_value());
    assert(throwsInvalidArgument([&identity]
    {
        identity.setAge(-1);
    }));
    assert(throwsInvalidArgument([&identity]
    {
        identity.setHeightCentimeters(0);
    }));
    assert(throwsInvalidArgument([&identity]
    {
        identity.setWeightGrams(0);
    }));
    assert(throwsInvalidArgument([&identity]
    {
        identity.setHair("   ");
    }));
    return 0;
}
