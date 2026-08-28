#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
    struct CharacterIdentitySaveData;
    struct CharacterIdentityView;

    enum class Alignment
    {
        LawfulGood,
        NeutralGood,
        ChaoticGood,
        LawfulNeutral,
        Neutral,
        ChaoticNeutral,
        LawfulEvil,
        NeutralEvil,
        ChaoticEvil
    };

    std::string_view displayName(Alignment alignment);

    class CharacterIdentity final
    {
    public:
        CharacterIdentity() = default;
        explicit CharacterIdentity(const CharacterIdentitySaveData &data);

        void setName(std::string_view name);
        void setPlayerName(std::string_view playerName);
        void setAlignment(std::optional<Alignment> alignment);
        void setDeity(std::optional<std::string> deity);
        void setHomeland(std::optional<std::string> homeland);
        void setGender(std::optional<std::string> gender);
        void setAge(std::optional<int> age);
        void setHeightCentimeters(std::optional<int> heightCentimeters);
        void setWeightGrams(std::optional<std::int64_t> weightGrams);
        void setHair(std::optional<std::string> hair);
        void setEyes(std::optional<std::string> eyes);
        void setAppearance(std::optional<std::string> appearance);
        CharacterIdentityView toView() const;
        CharacterIdentitySaveData toSaveData() const;

    private:
        std::string name_;
        std::string playerName_;
        std::optional<Alignment> alignment_;
        std::optional<std::string> deity_;
        std::optional<std::string> homeland_;
        std::optional<std::string> gender_;
        std::optional<int> age_;
        std::optional<int> heightCentimeters_;
        std::optional<std::int64_t> weightGrams_;
        std::optional<std::string> hair_;
        std::optional<std::string> eyes_;
        std::optional<std::string> appearance_;
    };
}
