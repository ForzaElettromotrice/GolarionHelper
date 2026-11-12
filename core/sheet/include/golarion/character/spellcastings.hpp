#pragma once
#include "golarion/character/pending_choice.hpp"

#include "golarion/character/spellcasting.hpp"
#include "golarion/data/spellcastings_data.hpp"
#include "golarion/view/spellcastings_view.hpp"

#include <memory>

namespace golarion
{
    class SpellCatalog;
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
        // Sources register pools here when they must survive even without active casting grants.
        static constexpr std::string_view UsagePoolsResource = "spellcastings.metamagicUsagePools";

        explicit Spellcastings(ResourceManager &resources, const SpellCatalog *catalog = nullptr);
        ~Spellcastings();
        Spellcastings(const Spellcastings &) = delete;
        Spellcastings &operator=(const Spellcastings &) = delete;
        Spellcastings(Spellcastings &&) = delete;
        Spellcastings &operator=(Spellcastings &&) = delete;

        // The reference remains valid until its grant is removed or this owner is destroyed.
        Spellcasting &get(std::string_view id);
        SpellcastingsData toData() const;
        SpellcastingsView toView() const;
        void load(const SpellcastingsData &data);

        std::vector<PendingChoice> pendingChoices() const;

    private:
        void grant(SpellcastingGrant grant);
        void revoke(std::string_view id);
        MetamagicUsagePools usagePools() const;

        struct Entry
        {
            SpellcastingGrant grant;
            std::unique_ptr<Spellcasting> casting;
        };
        const SpellCatalog *catalog_;
        ResourceManager &resources_;
        std::map<std::string, Entry> entries_;
        MetamagicUsagePools sourcePools_;
    };
}
