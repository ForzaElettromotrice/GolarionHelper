#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/character/spell_slot_grant.hpp"
#include "golarion/character/metamagic.hpp"

#include <map>
#include <set>
#include <string>
#include <string_view>

namespace golarion
{
    class ResourceManager;

    std::string spellSlotsResource(std::string_view castingId);

    class Spellcasting
    {
    public:
        virtual ~Spellcasting() = 0;
        Spellcasting(const Spellcasting &) = delete;
        Spellcasting &operator=(const Spellcasting &) = delete;
        Spellcasting(Spellcasting &&) = delete;
        Spellcasting &operator=(Spellcasting &&) = delete;

        const std::map<std::string, SpellSlotGrant> &slotPools() const;
        void addToRepertoire(std::string_view spellId);

    protected:
        explicit Spellcasting(ResourceManager &resources, std::string_view castingId, CharacterClassType spellListClass);
        bool knowsSpell(std::string_view spellId) const;
        CharacterClassType spellListClass() const;
        const MetamagicRegistry &metamagic() const { return metamagic_; }
        virtual void onSlotGroupRemoved(std::string_view id) noexcept;

    private:
        friend class Spellcastings;
        void grantSlots(SpellSlotGrant grant);
        void revokeSlots(std::string_view id);

        ResourceManager &resources_;
        std::string slotsResource_;
        CharacterClassType spellListClass_;
        std::set<std::string> repertoire_{};
        std::map<std::string, SpellSlotGrant> slotPools_;
        MetamagicRegistry metamagic_;
    };
}
