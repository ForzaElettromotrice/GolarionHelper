#pragma once
#include "golarion/character/pending_choice.hpp"

#include "golarion/effect/effect_compiler.hpp"
#include "golarion/data/favored_classes_data.hpp"
#include "golarion/view/favored_classes_view.hpp"

#include <map>
#include <set>
#include <string_view>
#include <tuple>

namespace golarion
{
    struct FavoredClassGrant
    {
        std::string id;
        std::string source;
        int additionalChoices = 0;
        std::set<CharacterClassType> classes{};
        bool allClasses = false;
    };

    struct FavoredClassBonusOption
    {
        std::string id;
        std::string name;
        std::string description;
        // Empty means all eligible classes. IDs identify source-owned registrations.
        std::set<CharacterClassType> classes{};
        std::vector<FavoredClassBenefitEffect> effects{};
        std::set<FavoredClassBaseBonus> replaces{};
        // Apply one copy of effects for each complete group of selections, per class.
        int selectionsPerBenefit = 1;
        std::function<std::shared_ptr<UsagePool>(std::string_view)> usagePool{};
    };

    class Classes;

    class FavoredClasses final
    {
    public:
        static constexpr std::string_view GrantsResource = "classes.favored.grants";
        static constexpr std::string_view OptionsResource = "classes.favored.options";
        static constexpr std::string_view HitPointOption = "base.hitPoint";
        static constexpr std::string_view SkillRankOption = "base.skillRank";

        explicit FavoredClasses(ResourceManager &resources);
        ~FavoredClasses();
        FavoredClasses(const FavoredClasses &) = delete;
        FavoredClasses &operator=(const FavoredClasses &) = delete;

        void setClasses(std::set<CharacterClassType> classes);
        void selectBonus(CharacterClassType type, int classLevel, std::string optionId);
        void clearBonus(CharacterClassType type, int classLevel);
        FavoredClassesData toData() const;
        FavoredClassesView toView() const;
        void load(const FavoredClassesData &data);

        std::vector<PendingChoice> pendingChoices() const;

    private:
        friend class Classes;
        using SelectionKey = std::pair<CharacterClassType, int>;
        using EffectKey = std::tuple<CharacterClassType, std::string, int, std::size_t>;
        void addGrant(FavoredClassGrant grant);
        void removeGrant(std::string_view id);
        void addOption(FavoredClassBonusOption option);
        void removeOption(std::string_view id);
        int capacity() const;
        bool favored(CharacterClassType type) const;
        bool available(CharacterClassType type, const std::string &id) const;
        void reconcile();
        void clearState();

        ResourceManager &resources_;
        std::map<CharacterClassType, int> levels_;
        std::set<CharacterClassType> selected_;
        std::map<SelectionKey, std::string> selections_;
        std::map<std::string, FavoredClassGrant> grants_;
        std::map<std::string, FavoredClassBonusOption> options_;
        std::map<EffectKey, EffectCleanup> applied_;
        bool reconciling_ = false;
    };
}
