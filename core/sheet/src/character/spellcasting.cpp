#include "golarion/character/spell_catalog.hpp"
#include "golarion/character/spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    MetamagicUsagePools Spellcasting::usagePools() const
    {
        return metamagic_.usagePools();
    }

    std::string spellSlotsResource(std::string_view castingId)
    {
        return "spellcasting." + normalize(castingId) + ".slots";
    }

    const SpellDefinition &Spellcasting::spellDefinition(std::string_view id) const
    {
        return catalog_ ? catalog_->get(id) : SpellDefinitionManager::instance().get(id);
    }

    Spellcasting::~Spellcasting()
    {
        resources_.unregisterCollectionResource(slotsResource_);
    }

    Spellcasting::Spellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass, const SpellCatalog *catalog, bool allowManualRepertoire)
        : catalog_(catalog), resources_(resources), slotsResource_(spellSlotsResource(castingId)), spellListClass_(spellListClass), repertoire_(resources, castingId, spellListClass, allowManualRepertoire, [this](std::string_view id) -> const SpellDefinition & { return spellDefinition(id); }), metamagic_(resources, castingId)
    {
        switch (spellListClass_)
        {
        case CharacterClassType::Adept:
        case CharacterClassType::Alchemist:
        case CharacterClassType::Antipaladin:
        case CharacterClassType::Arcanist:
        case CharacterClassType::Sorcerer:
        case CharacterClassType::Skald:
        case CharacterClassType::Oracle:
        case CharacterClassType::Warpriest:
        case CharacterClassType::Investigator:
        case CharacterClassType::Wizard:
        case CharacterClassType::Bard:
        case CharacterClassType::Hunter:
        case CharacterClassType::Cleric:
        case CharacterClassType::Summoner:
        case CharacterClassType::UnchainedSummoner:
        case CharacterClassType::Druid:
        case CharacterClassType::Witch:
        case CharacterClassType::Inquisitor:
        case CharacterClassType::Bloodrager:
        case CharacterClassType::Magus:
        case CharacterClassType::Medium:
        case CharacterClassType::Mesmerist:
        case CharacterClassType::Occultist:
        case CharacterClassType::Paladin:
        case CharacterClassType::Psychic:
        case CharacterClassType::Ranger:
        case CharacterClassType::Shaman:
        case CharacterClassType::Spiritualist:
            break;
        default:
            throw std::invalid_argument("invalid spell list");
        }
        resources_.registerCollectionResource<SpellSlotGrant>(slotsResource_,
            [this](SpellSlotGrant grant) { grantSlots(std::move(grant)); },
            [this](std::string_view id) { revokeSlots(id); });
    }

    const std::map<std::string, SpellSlotGrant> &Spellcasting::slotPools() const
    {
        return slotPools_;
    }

    SpellSlotGrant normalizeSpellSlotGrant(SpellSlotGrant grant)
    {
        grant.id = normalize(grant.id);
        grant.source = normalize(grant.source);
        if (grant.dailyCapacityByLevel.empty())
            throw std::invalid_argument("spell slot grant must contain at least one level");
        for (const auto &[level, capacity] : grant.dailyCapacityByLevel)
        {
            if (level < 0 || level > 9)
                throw std::invalid_argument("spell slot level must be between 0 and 9");
            if (capacity <= 0)
                throw std::invalid_argument("spell slot capacity must be positive");
        }
        if (grant.allowedSpellIds)
        {
            std::set<std::string> ids;
            for (const auto &id : *grant.allowedSpellIds)
                if (!ids.insert(normalize(id)).second)
                    throw std::invalid_argument("duplicate normalized spell ID in slot filter");
            grant.allowedSpellIds = std::move(ids);
        }
        return grant;
    }

    void Spellcasting::grantSlots(SpellSlotGrant grant)
    {
        grant = normalizeSpellSlotGrant(std::move(grant));
        const auto id = grant.id;
        if (!slotPools_.emplace(id, std::move(grant)).second)
            throw std::invalid_argument("spell slot grant is already registered: " + id);
    }

    void Spellcasting::revokeSlots(std::string_view id)
    {
        const auto normalizedId = normalize(id);
        if (!slotPools_.contains(normalizedId))
            throw std::invalid_argument("spell slot grant is not registered");
        onSlotGroupRemoved(normalizedId);
        slotPools_.erase(normalizedId);
    }

    void Spellcasting::addToRepertoire(std::string_view spellId)
    {
        repertoire_.addManual(spellId);
    }

    void Spellcasting::removeFromRepertoire(std::string_view spellId) { repertoire_.removeManual(spellId); }
    void Spellcasting::selectRepertoireSpell(RepertoireSelectionDefinition selection) { repertoire_.select(std::move(selection)); }
    void Spellcasting::removeRepertoireSelection(RepertoireSelectionDefinition selection) { repertoire_.unselect(std::move(selection)); }

    bool Spellcasting::knowsSpell(std::string_view spellId) const
    {
        return repertoire_.knows(spellId);
    }

    CharacterClassType Spellcasting::spellListClass() const
    {
        return spellListClass_;
    }

    void Spellcasting::onSlotGroupRemoved(std::string_view) noexcept {}
}
