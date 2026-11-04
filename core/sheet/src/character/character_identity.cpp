#include "golarion/character/character_identity.hpp"

#include "golarion/data/character_identity_save_data.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/character_identity_view.hpp"

#include <stdexcept>
#include <utility>

namespace
{
    std::string normalizeRequiredText(std::string_view value)
    {
        if (value.empty())
        {
            return {};
        }
        return golarion::normalize(value);
    }

    std::optional<std::string> normalizeOptionalText(std::optional<std::string> value)
    {
        if (value.has_value())
        {
            value = golarion::normalize(*value);
        }
        return value;
    }
}

namespace golarion
{
    std::string_view displayName(Alignment alignment)
    {
        switch (alignment)
        {
            case Alignment::LawfulGood:
                return "Legale Buono";
            case Alignment::NeutralGood:
                return "Neutrale Buono";
            case Alignment::ChaoticGood:
                return "Caotico Buono";
            case Alignment::LawfulNeutral:
                return "Legale Neutrale";
            case Alignment::Neutral:
                return "Neutrale";
            case Alignment::ChaoticNeutral:
                return "Caotico Neutrale";
            case Alignment::LawfulEvil:
                return "Legale Malvagio";
            case Alignment::NeutralEvil:
                return "Neutrale Malvagio";
            case Alignment::ChaoticEvil:
                return "Caotico Malvagio";
        }

        throw std::invalid_argument("unknown alignment");
    }

    CharacterIdentity::CharacterIdentity(const CharacterIdentitySaveData &data)
    {
        setName(data.name);
        setPlayerName(data.playerName);
        setAlignment(data.alignment);
        setDeity(data.deity);
        setHomeland(data.homeland);
        setGender(data.gender);
        setAge(data.age);
        setHeightCentimeters(data.heightCentimeters);
        setWeightGrams(data.weightGrams);
        setHair(data.hair);
        setEyes(data.eyes);
        setAppearance(data.appearance);
    }

    void CharacterIdentity::setName(std::string_view name)
    {
        name_ = normalizeRequiredText(name);
    }

    void CharacterIdentity::setPlayerName(std::string_view playerName)
    {
        playerName_ = normalizeRequiredText(playerName);
    }

    void CharacterIdentity::setAlignment(std::optional<Alignment> alignment)
    {
        alignment_ = alignment;
    }

    void CharacterIdentity::setDeity(std::optional<std::string> deity)
    {
        deity_ = normalizeOptionalText(std::move(deity));
    }

    void CharacterIdentity::setHomeland(std::optional<std::string> homeland)
    {
        homeland_ = normalizeOptionalText(std::move(homeland));
    }

    void CharacterIdentity::setGender(std::optional<std::string> gender)
    {
        gender_ = normalizeOptionalText(std::move(gender));
    }

    void CharacterIdentity::setAge(std::optional<int> age)
    {
        if (age.has_value() && *age < 0)
        {
            throw std::invalid_argument("character age must not be negative");
        }
        age_ = age;
    }

    void CharacterIdentity::setHeightCentimeters(std::optional<int> heightCentimeters)
    {
        if (heightCentimeters.has_value() && *heightCentimeters <= 0)
        {
            throw std::invalid_argument("character height must be positive");
        }
        heightCentimeters_ = heightCentimeters;
    }

    void CharacterIdentity::setWeightGrams(std::optional<std::int64_t> weightGrams)
    {
        if (weightGrams.has_value() && *weightGrams <= 0)
        {
            throw std::invalid_argument("character weight must be positive");
        }
        weightGrams_ = weightGrams;
    }

    void CharacterIdentity::setHair(std::optional<std::string> hair)
    {
        hair_ = normalizeOptionalText(std::move(hair));
    }

    void CharacterIdentity::setEyes(std::optional<std::string> eyes)
    {
        eyes_ = normalizeOptionalText(std::move(eyes));
    }

    void CharacterIdentity::setAppearance(std::optional<std::string> appearance)
    {
        appearance_ = normalizeOptionalText(std::move(appearance));
    }

    CharacterIdentityView CharacterIdentity::toView() const
    {
        return CharacterIdentityView{
            .name = name_,
            .playerName = playerName_,
            .alignment = alignment_,
            .deity = deity_,
            .homeland = homeland_,
            .gender = gender_,
            .age = age_,
            .heightCentimeters = heightCentimeters_,
            .weightGrams = weightGrams_,
            .hair = hair_,
            .eyes = eyes_,
            .appearance = appearance_
        };
    }

    CharacterIdentitySaveData CharacterIdentity::toSaveData() const
    {
        return CharacterIdentitySaveData{
            .name = name_,
            .playerName = playerName_,
            .alignment = alignment_,
            .deity = deity_,
            .homeland = homeland_,
            .gender = gender_,
            .age = age_,
            .heightCentimeters = heightCentimeters_,
            .weightGrams = weightGrams_,
            .hair = hair_,
            .eyes = eyes_,
            .appearance = appearance_
        };
    }
}
