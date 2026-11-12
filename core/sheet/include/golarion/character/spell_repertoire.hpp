#pragma once
#include "golarion/character/character_class_type.hpp"
#include "golarion/view/spell_repertoire_view.hpp"
#include <functional>
#include <string_view>

namespace golarion
{
    class ResourceManager;
    struct SpellDefinition;
    class Spellcastings;
    std::string repertoireQuotasResource(std::string_view castingId);
    std::string repertoireGrantsResource(std::string_view castingId);
    std::string repertoireListsResource(std::string_view castingId);

    class SpellRepertoire final
    {
    public:
        SpellRepertoire(ResourceManager &resources, std::string_view castingId, CharacterClassType list, bool allowManual, std::function<const SpellDefinition &(std::string_view)> resolve);
        ~SpellRepertoire();
        SpellRepertoire(const SpellRepertoire &) = delete;
        SpellRepertoire &operator=(const SpellRepertoire &) = delete;
        void addManual(std::string_view id);
        void removeManual(std::string_view id);
        void select(RepertoireSelectionDefinition selection);
        void unselect(RepertoireSelectionDefinition selection);
        bool knows(std::string_view id) const;
        std::vector<std::string> knownIds() const;
        SpellRepertoireView toView() const;
        SpellRepertoireData toData() const;

    private:
        friend class Spellcastings;
        SpellRepertoireData validatedData(SpellRepertoireData data) const;
        void swapData(SpellRepertoireData &data) noexcept;
        bool accepts(const SpellRepertoireQuota &quota, int level, std::string_view id) const;
        bool active(const RepertoireSelectionDefinition &selection) const;
        int selectedCount(std::string_view quotaId, int level) const;
        int listLevel(std::string_view id) const;
        ResourceManager &resources_;
        std::string quotasResource_, grantsResource_, listsResource_;
        CharacterClassType list_;
        bool allowManual_;
        std::function<const SpellDefinition &(std::string_view)> resolve_;
        std::map<std::string, SpellRepertoireQuota> quotas_;
        std::map<std::string, SpellRepertoireGrant> grants_;
        std::map<std::string, SpellRepertoireListGrant> lists_;
        SpellRepertoireData data_;
    };
}
