#pragma once

#include "golarion/character/granted_spell.hpp"
#include "golarion/character/spell_cast_result.hpp"
#include "golarion/data/granted_spells_data.hpp"
#include <map>
#include <string_view>

namespace golarion
{
    class SpellCatalog;
    class ResourceManager;
    class GrantedSpells final
    {
    public:
        static constexpr std::string_view Resource = "spells.granted";
        explicit GrantedSpells(ResourceManager &resources, const SpellCatalog *catalog = nullptr);
        ~GrantedSpells();
        GrantedSpells(const GrantedSpells &) = delete;
        GrantedSpells &operator=(const GrantedSpells &) = delete;
        GrantedSpells(GrantedSpells &&) = delete;
        GrantedSpells &operator=(GrantedSpells &&) = delete;
        SpellCastResult consume(std::string_view id);
        void refill(std::string_view id);
        void restore(std::string_view id, int amount);
        std::vector<GrantedSpellView> toView() const;
        GrantedSpellsData toData() const;
        void load(const GrantedSpellsData &data);
    private:
        GrantedSpell &get(std::string_view id);
        const SpellCatalog *catalog_;
        ResourceManager &resources_;
        std::map<std::string, GrantedSpell> spells_;
    };
}
