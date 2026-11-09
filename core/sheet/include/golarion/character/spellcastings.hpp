#pragma once

#include "golarion/character/spellcasting.hpp"
#include "golarion/data/spellcastings_data.hpp"

#include <memory>

namespace golarion
{
    enum class SpellcastingMode { Prepared, Spontaneous, Flexible };

    struct SpellcastingGrant
    {
        std::string id;
        std::string source;
        SpellcastingMode mode;
        CharacterClassType spellListClass;
    };

    class Spellcastings final
    {
    public:
        static constexpr std::string_view Resource = "spellcastings";

        explicit Spellcastings(ResourceManager &resources);
        ~Spellcastings();
        Spellcastings(const Spellcastings &) = delete;
        Spellcastings &operator=(const Spellcastings &) = delete;
        Spellcastings(Spellcastings &&) = delete;
        Spellcastings &operator=(Spellcastings &&) = delete;

        // The reference remains valid until its grant is removed or this owner is destroyed.
        Spellcasting &get(std::string_view id);
        SpellcastingsData toData() const;
        void load(const SpellcastingsData &data);

    private:
        void grant(SpellcastingGrant grant);
        void revoke(std::string_view id);

        struct Entry
        {
            SpellcastingGrant grant;
            std::unique_ptr<Spellcasting> casting;
        };
        ResourceManager &resources_;
        std::map<std::string, Entry> entries_;
    };
}
