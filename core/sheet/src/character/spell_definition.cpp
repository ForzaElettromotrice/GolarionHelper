#include "golarion/character/spell_definition.hpp"

#include "golarion/util/string_utils.hpp"

#include <stdexcept>

namespace
{
    void validateEnum(golarion::SpellSchool value)
    {
        switch (value)
        {
            case golarion::SpellSchool::Abjuration:
            case golarion::SpellSchool::Conjuration:
            case golarion::SpellSchool::Divination:
            case golarion::SpellSchool::Enchantment:
            case golarion::SpellSchool::Evocation:
            case golarion::SpellSchool::Illusion:
            case golarion::SpellSchool::Necromancy:
            case golarion::SpellSchool::Transmutation:
            case golarion::SpellSchool::Universal:
                return;
        }
        throw std::invalid_argument("unknown SpellSchool value");
    }

    void validateEnum(golarion::SpellSubschool value)
    {
        switch (value)
        {
            case golarion::SpellSubschool::Calling:
            case golarion::SpellSubschool::Charm:
            case golarion::SpellSubschool::Compulsion:
            case golarion::SpellSubschool::Creation:
            case golarion::SpellSubschool::Figment:
            case golarion::SpellSubschool::Glamer:
            case golarion::SpellSubschool::Healing:
            case golarion::SpellSubschool::Pattern:
            case golarion::SpellSubschool::Phantasm:
            case golarion::SpellSubschool::Polymorph:
            case golarion::SpellSubschool::Scrying:
            case golarion::SpellSubschool::Shadow:
            case golarion::SpellSubschool::Summoning:
            case golarion::SpellSubschool::Teleportation:
                return;
        }
        throw std::invalid_argument("unknown SpellSubschool value");
    }

    void validateEnum(golarion::SpellDescriptor value)
    {
        switch (value)
        {
            case golarion::SpellDescriptor::Acid:
            case golarion::SpellDescriptor::Air:
            case golarion::SpellDescriptor::Chaotic:
            case golarion::SpellDescriptor::Cold:
            case golarion::SpellDescriptor::Curse:
            case golarion::SpellDescriptor::Darkness:
            case golarion::SpellDescriptor::Death:
            case golarion::SpellDescriptor::Disease:
            case golarion::SpellDescriptor::Earth:
            case golarion::SpellDescriptor::Electricity:
            case golarion::SpellDescriptor::Emotion:
            case golarion::SpellDescriptor::Evil:
            case golarion::SpellDescriptor::Fear:
            case golarion::SpellDescriptor::Fire:
            case golarion::SpellDescriptor::Force:
            case golarion::SpellDescriptor::Good:
            case golarion::SpellDescriptor::LanguageDependent:
            case golarion::SpellDescriptor::Lawful:
            case golarion::SpellDescriptor::Light:
            case golarion::SpellDescriptor::MindAffecting:
            case golarion::SpellDescriptor::Pain:
            case golarion::SpellDescriptor::Poison:
            case golarion::SpellDescriptor::Shadow:
            case golarion::SpellDescriptor::Sonic:
            case golarion::SpellDescriptor::Water:
                return;
        }
        throw std::invalid_argument("unknown SpellDescriptor value");
    }

    void validateEnum(golarion::SpellCastingTimeCategory value)
    {
        switch (value)
        {
            case golarion::SpellCastingTimeCategory::SwiftAction:
            case golarion::SpellCastingTimeCategory::ImmediateAction:
            case golarion::SpellCastingTimeCategory::StandardAction:
            case golarion::SpellCastingTimeCategory::FullRoundAction:
            case golarion::SpellCastingTimeCategory::Round:
            case golarion::SpellCastingTimeCategory::Minute:
            case golarion::SpellCastingTimeCategory::Hour:
            case golarion::SpellCastingTimeCategory::Day:
            case golarion::SpellCastingTimeCategory::Special:
                return;
        }
        throw std::invalid_argument("unknown SpellCastingTimeCategory value");
    }

    void validateEnum(golarion::SpellComponentType value)
    {
        switch (value)
        {
            case golarion::SpellComponentType::Verbal:
            case golarion::SpellComponentType::Somatic:
            case golarion::SpellComponentType::Material:
            case golarion::SpellComponentType::Focus:
            case golarion::SpellComponentType::DivineFocus:
            case golarion::SpellComponentType::MaterialOrDivineFocus:
            case golarion::SpellComponentType::FocusOrDivineFocus:
                return;
        }
        throw std::invalid_argument("unknown SpellComponentType value");
    }

    void validateEnum(golarion::SpellRangeType value)
    {
        switch (value)
        {
            case golarion::SpellRangeType::Personal:
            case golarion::SpellRangeType::Touch:
            case golarion::SpellRangeType::Close:
            case golarion::SpellRangeType::Medium:
            case golarion::SpellRangeType::Long:
            case golarion::SpellRangeType::Unlimited:
            case golarion::SpellRangeType::Other:
                return;
        }
        throw std::invalid_argument("unknown SpellRangeType value");
    }

    void validateEnum(golarion::SpellDurationType value)
    {
        switch (value)
        {
            case golarion::SpellDurationType::Instantaneous:
            case golarion::SpellDurationType::Permanent:
            case golarion::SpellDurationType::Concentration:
            case golarion::SpellDurationType::Round:
            case golarion::SpellDurationType::Minute:
            case golarion::SpellDurationType::Hour:
            case golarion::SpellDurationType::Day:
            case golarion::SpellDurationType::Other:
                return;
        }
        throw std::invalid_argument("unknown SpellDurationType value");
    }

    void validateEnum(golarion::SpellResistanceType value)
    {
        switch (value)
        {
            case golarion::SpellResistanceType::Yes:
            case golarion::SpellResistanceType::No:
            case golarion::SpellResistanceType::SeeText:
                return;
        }
        throw std::invalid_argument("unknown SpellResistanceType value");
    }

    void validateEnum(golarion::SpellSavingThrowEffect value)
    {
        switch (value)
        {
            case golarion::SpellSavingThrowEffect::Negates:
            case golarion::SpellSavingThrowEffect::Partial:
            case golarion::SpellSavingThrowEffect::Half:
            case golarion::SpellSavingThrowEffect::SeeText:
            case golarion::SpellSavingThrowEffect::Harmless:
            case golarion::SpellSavingThrowEffect::Object:
            case golarion::SpellSavingThrowEffect::Disbelief:
                return;
        }
        throw std::invalid_argument("unknown SpellSavingThrowEffect value");
    }

    void validateEnum(golarion::DomainType value)
    {
        switch (value)
        {
            case golarion::DomainType::Air:
            case golarion::DomainType::Animal:
            case golarion::DomainType::Artifice:
            case golarion::DomainType::Chaos:
            case golarion::DomainType::Charm:
            case golarion::DomainType::Community:
            case golarion::DomainType::Darkness:
            case golarion::DomainType::Death:
            case golarion::DomainType::Destruction:
            case golarion::DomainType::Earth:
            case golarion::DomainType::Evil:
            case golarion::DomainType::Fire:
            case golarion::DomainType::Glory:
            case golarion::DomainType::Good:
            case golarion::DomainType::Healing:
            case golarion::DomainType::Knowledge:
            case golarion::DomainType::Law:
            case golarion::DomainType::Liberation:
            case golarion::DomainType::Luck:
            case golarion::DomainType::Madness:
            case golarion::DomainType::Magic:
            case golarion::DomainType::Nobility:
            case golarion::DomainType::Plant:
            case golarion::DomainType::Protection:
            case golarion::DomainType::Reptile:
            case golarion::DomainType::Repose:
            case golarion::DomainType::Rune:
            case golarion::DomainType::Strength:
            case golarion::DomainType::Sun:
            case golarion::DomainType::Travel:
            case golarion::DomainType::Trickery:
            case golarion::DomainType::War:
            case golarion::DomainType::Water:
            case golarion::DomainType::Weather:
            case golarion::DomainType::Void:
                return;
        }
        throw std::invalid_argument("unknown DomainType value");
    }

    void validateEnum(golarion::SubdomainType value)
    {
        switch (value)
        {
            case golarion::SubdomainType::Agathion:
            case golarion::SubdomainType::Alienation:
            case golarion::SubdomainType::Ancestors:
            case golarion::SubdomainType::Arcane:
            case golarion::SubdomainType::Archon:
            case golarion::SubdomainType::Ash:
            case golarion::SubdomainType::Assassination:
            case golarion::SubdomainType::Azata:
            case golarion::SubdomainType::Blood:
            case golarion::SubdomainType::Catastrophe:
            case golarion::SubdomainType::Caves:
            case golarion::SubdomainType::Clouds:
            case golarion::SubdomainType::Commerce:
            case golarion::SubdomainType::Construct:
            case golarion::SubdomainType::Curse:
            case golarion::SubdomainType::DarkTapestry:
            case golarion::SubdomainType::Daemon:
            case golarion::SubdomainType::Day:
            case golarion::SubdomainType::Decay:
            case golarion::SubdomainType::Defense:
            case golarion::SubdomainType::Demon:
            case golarion::SubdomainType::Devil:
            case golarion::SubdomainType::Divine:
            case golarion::SubdomainType::Exploration:
            case golarion::SubdomainType::Family:
            case golarion::SubdomainType::Fate:
            case golarion::SubdomainType::Feather:
            case golarion::SubdomainType::Ferocity:
            case golarion::SubdomainType::Fur:
            case golarion::SubdomainType::Growth:
            case golarion::SubdomainType::Healing:
            case golarion::SubdomainType::Heroism:
            case golarion::SubdomainType::Home:
            case golarion::SubdomainType::Honor:
            case golarion::SubdomainType::Ice:
            case golarion::SubdomainType::Inevitables:
            case golarion::SubdomainType::Interdictions:
            case golarion::SubdomainType::Isolation:
            case golarion::SubdomainType::Language:
            case golarion::SubdomainType::Leadership:
            case golarion::SubdomainType::Liberation:
            case golarion::SubdomainType::Light:
            case golarion::SubdomainType::Loss:
            case golarion::SubdomainType::Loyalty:
            case golarion::SubdomainType::Love:
            case golarion::SubdomainType::Lust:
            case golarion::SubdomainType::Martyrdom:
            case golarion::SubdomainType::Memory:
            case golarion::SubdomainType::Metal:
            case golarion::SubdomainType::Moon:
            case golarion::SubdomainType::Night:
            case golarion::SubdomainType::Nightmare:
            case golarion::SubdomainType::Oceans:
            case golarion::SubdomainType::Protean:
            case golarion::SubdomainType::Purity:
            case golarion::SubdomainType::Rage:
            case golarion::SubdomainType::Resolution:
            case golarion::SubdomainType::Resurrection:
            case golarion::SubdomainType::Revolution:
            case golarion::SubdomainType::Rivers:
            case golarion::SubdomainType::Seasons:
            case golarion::SubdomainType::Smoke:
            case golarion::SubdomainType::Souls:
            case golarion::SubdomainType::Stars:
            case golarion::SubdomainType::Storms:
            case golarion::SubdomainType::Subterfuge:
            case golarion::SubdomainType::Tactics:
            case golarion::SubdomainType::Theft:
            case golarion::SubdomainType::Thought:
            case golarion::SubdomainType::Undeath:
            case golarion::SubdomainType::Whimsy:
            case golarion::SubdomainType::Wind:
                return;
        }
        throw std::invalid_argument("unknown SubdomainType value");
    }

    void validateEnum(golarion::BloodlineType value)
    {
        switch (value)
        {
            case golarion::BloodlineType::Aberrant:
            case golarion::BloodlineType::Abyssal:
            case golarion::BloodlineType::Accursed:
            case golarion::BloodlineType::Aquatic:
            case golarion::BloodlineType::Arcane:
            case golarion::BloodlineType::Celestial:
            case golarion::BloodlineType::Destined:
            case golarion::BloodlineType::Draconic:
            case golarion::BloodlineType::Earth:
            case golarion::BloodlineType::Elemental:
            case golarion::BloodlineType::Fey:
            case golarion::BloodlineType::Impossible:
            case golarion::BloodlineType::Infernal:
            case golarion::BloodlineType::Marid:
            case golarion::BloodlineType::Oni:
            case golarion::BloodlineType::Phoenix:
            case golarion::BloodlineType::Pestilence:
            case golarion::BloodlineType::Protean:
            case golarion::BloodlineType::Psychic:
            case golarion::BloodlineType::Serpentine:
            case golarion::BloodlineType::Shadow:
            case golarion::BloodlineType::Starsoul:
            case golarion::BloodlineType::Stormborn:
            case golarion::BloodlineType::Undead:
            case golarion::BloodlineType::Vermin:
                return;
        }
        throw std::invalid_argument("unknown BloodlineType value");
    }

    void validateEnum(golarion::CharacterClassType value)
    {
        switch (value)
        {
            case golarion::CharacterClassType::Barbarian:
            case golarion::CharacterClassType::Adept:
            case golarion::CharacterClassType::Bard:
            case golarion::CharacterClassType::Cleric:
            case golarion::CharacterClassType::Druid:
            case golarion::CharacterClassType::Fighter:
            case golarion::CharacterClassType::Rogue:
            case golarion::CharacterClassType::Wizard:
            case golarion::CharacterClassType::Monk:
            case golarion::CharacterClassType::Paladin:
            case golarion::CharacterClassType::Ranger:
            case golarion::CharacterClassType::Sorcerer:
            case golarion::CharacterClassType::Alchemist:
            case golarion::CharacterClassType::Cavalier:
            case golarion::CharacterClassType::Summoner:
            case golarion::CharacterClassType::Witch:
            case golarion::CharacterClassType::Inquisitor:
            case golarion::CharacterClassType::Magus:
            case golarion::CharacterClassType::Shifter:
            case golarion::CharacterClassType::Oracle:
            case golarion::CharacterClassType::Gunslinger:
            case golarion::CharacterClassType::Vigilante:
            case golarion::CharacterClassType::Antipaladin:
            case golarion::CharacterClassType::Ninja:
            case golarion::CharacterClassType::Samurai:
            case golarion::CharacterClassType::Arcanist:
            case golarion::CharacterClassType::Brawler:
            case golarion::CharacterClassType::Hunter:
            case golarion::CharacterClassType::Swashbuckler:
            case golarion::CharacterClassType::Investigator:
            case golarion::CharacterClassType::Bloodrager:
            case golarion::CharacterClassType::Slayer:
            case golarion::CharacterClassType::Warpriest:
            case golarion::CharacterClassType::Skald:
            case golarion::CharacterClassType::Shaman:
            case golarion::CharacterClassType::Kineticist:
            case golarion::CharacterClassType::Medium:
            case golarion::CharacterClassType::Mesmerist:
            case golarion::CharacterClassType::Occultist:
            case golarion::CharacterClassType::Psychic:
            case golarion::CharacterClassType::Spiritualist:
            case golarion::CharacterClassType::UnchainedBarbarian:
            case golarion::CharacterClassType::UnchainedSummoner:
            case golarion::CharacterClassType::UnchainedRogue:
            case golarion::CharacterClassType::UnchainedMonk:
                return;
        }
        throw std::invalid_argument("unknown CharacterClassType value");
    }

    golarion::SpellSchool schoolOf(golarion::SpellSubschool subschool)
    {
        using namespace golarion;
        switch (subschool)
        {
            case SpellSubschool::Calling: return SpellSchool::Conjuration;
            case SpellSubschool::Charm: return SpellSchool::Enchantment;
            case SpellSubschool::Compulsion: return SpellSchool::Enchantment;
            case SpellSubschool::Creation: return SpellSchool::Conjuration;
            case SpellSubschool::Figment: return SpellSchool::Illusion;
            case SpellSubschool::Glamer: return SpellSchool::Illusion;
            case SpellSubschool::Healing: return SpellSchool::Conjuration;
            case SpellSubschool::Pattern: return SpellSchool::Illusion;
            case SpellSubschool::Phantasm: return SpellSchool::Illusion;
            case SpellSubschool::Polymorph: return SpellSchool::Transmutation;
            case SpellSubschool::Scrying: return SpellSchool::Divination;
            case SpellSubschool::Shadow: return SpellSchool::Illusion;
            case SpellSubschool::Summoning: return SpellSchool::Conjuration;
            case SpellSubschool::Teleportation: return SpellSchool::Conjuration;
        }
        throw std::invalid_argument("unknown spell subschool");
    }
}

namespace golarion
{
    void validateSpellDefinition(const SpellDefinition &definition)
    {
        std::set<std::string> effectIds;
        for (const auto &effect : definition.effects)
            if (!effectIds.insert(normalize(effectId(effect))).second)
                throw std::invalid_argument("spell effects must have unique IDs");
        for (const auto *text : {&definition.id, &definition.name, &definition.castingTimeDescription,
             &definition.rangeDescription, &definition.durationDescription,
             &definition.spellResistanceDescription, &definition.description})
            static_cast<void>(normalize(*text));
        for (const auto *text : {&definition.materialComponentDescription, &definition.focusComponentDescription,
             &definition.target, &definition.effect, &definition.area})
            if (*text) static_cast<void>(normalize(**text));
        validateEnum(definition.school);
        if (definition.subschool) validateEnum(*definition.subschool);
        if (definition.subschool && schoolOf(*definition.subschool) != definition.school)
            throw std::invalid_argument("spell subschool does not match school");
        for (const auto descriptor : definition.descriptors) validateEnum(descriptor);
        validateEnum(definition.castingTimeCategory);
        validateEnum(definition.rangeType);
        validateEnum(definition.durationType);
        validateEnum(definition.spellResistanceType);
        const auto checkLevel = [](int level)
        {
            if (level < 0) throw std::invalid_argument("spell level must not be negative");
        };
        for (const auto &[type, level] : definition.spellLevelByClass) { validateEnum(type); checkLevel(level); }
        for (const auto &[type, level] : definition.spellLevelByDomain)
        {
            std::visit([](auto value) { validateEnum(value); }, type);
            checkLevel(level);
        }
        for (const auto &[type, level] : definition.spellLevelByBloodline) { validateEnum(type); checkLevel(level); }
        const auto &components = definition.components;
        if (components.empty()) throw std::invalid_argument("spell components must not be empty");
        for (const auto component : components) validateEnum(component);
        const bool material = components.contains(SpellComponentType::Material);
        const bool focus = components.contains(SpellComponentType::Focus);
        const bool divineFocus = components.contains(SpellComponentType::DivineFocus);
        const bool materialOrDivine = components.contains(SpellComponentType::MaterialOrDivineFocus);
        const bool focusOrDivine = components.contains(SpellComponentType::FocusOrDivineFocus);
        if (materialOrDivine && (material || divineFocus))
            throw std::invalid_argument("material/divine focus conflicts with explicit components");
        if (focusOrDivine && (focus || divineFocus))
            throw std::invalid_argument("focus/divine focus conflicts with explicit components");
        if ((definition.materialComponentDescription || definition.materialComponentCostInGoldPieces) && !material && !materialOrDivine)
            throw std::invalid_argument("material details require a material component");
        if (definition.materialComponentCostInGoldPieces && *definition.materialComponentCostInGoldPieces < 0)
            throw std::invalid_argument("material component cost must not be negative");
        if (definition.focusComponentDescription && !focus && !focusOrDivine)
            throw std::invalid_argument("focus details require a focus component");
        if (definition.durationType == SpellDurationType::Concentration && !definition.dismissible)
            throw std::invalid_argument("concentration spells must be dismissible");
        for (const auto &savingThrow : definition.savingThrows)
        {
            if (savingThrow.type != SavingThrowType::Fortitude && savingThrow.type != SavingThrowType::Reflex && savingThrow.type != SavingThrowType::Will)
                throw std::invalid_argument("unknown saving throw type");
            validateEnum(savingThrow.effect);
            static_cast<void>(normalize(savingThrow.description));
        }
    }
}
