#pragma once
#include "golarion/character/pending_choice.hpp"

#include "golarion/character/feat_prerequisite_registry.hpp"
#include "golarion/character/feat_slot_filter.hpp"
#include "golarion/data/feats_data.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/view/feats_view.hpp"

#include <map>
#include <variant>

namespace golarion
{
    class FeatCatalog;
    inline constexpr std::string_view FeatSlotsResource = "feat.slots";

    using FeatChoices = std::map<std::string, std::vector<std::string>>;

    struct FeatSlot
    {
        std::string id;
        std::string source;
        FeatSlotFilter filter = AnyFeat{};
        // An empty callback grants no exemption. It never overrides slot eligibility.
        std::function<bool(const FeatRequirementDefinition &, const FeatPrerequisiteContext &)> exempts{};
    };

    class Feats final
    {
    public:
        explicit Feats(ResourceManager &resourceManager, const FeatCatalog *catalog = nullptr);
        ~Feats();
        Feats(const Feats &) = delete;
        Feats &operator=(const Feats &) = delete;
        Feats(Feats &&) = delete;
        Feats &operator=(Feats &&) = delete;

        void addExtraSlot(std::string_view id);
        void removeExtraSlot(std::string_view id);
        void setLevel(int level);
        void selectFeat(std::string_view slotId, std::string_view featId, FeatChoices choices = {});
        void removeFeat(std::string_view slotId);
        void recheckPrerequisites();
        bool qualifies(std::string_view slotId, const FeatDefinition &definition, const FeatChoices &choices = {}) const;
        FeatsView toView() const;
        FeatsData toData() const;
        // Restore selections after sources; missing slots remain unavailable until granted again.
        void load(const FeatsData &data);

        std::vector<PendingChoice> pendingChoices() const;

    private:
        struct SlotState
        {
            FeatSlot slot;
            FeatSlotOrigin origin = FeatSlotOrigin::Source;
            bool available = true;
            const FeatDefinition *selected = nullptr;
            bool active = false;
            FeatChoices choices;
            std::vector<EffectCleanup> cleanups;
        };
        void addSlot(FeatSlot slot);
        void removeSlot(std::string_view id);
        void addNormalizedSlot(FeatSlot slot);
        bool requirementSatisfied(const FeatRequirementDefinition &requirement, const SlotState &slot, const FeatPrerequisiteContext &context) const;
        void clear(SlotState &slot);
        void deactivate(SlotState &slot);
        void activate(SlotState &slot);
        void select(std::string_view slotId, std::string_view featId, FeatChoices choices, bool restoring);

        const FeatCatalog *catalog_;
        ResourceManager &resourceManager_;
        FeatPrerequisiteRegistry prerequisites_;
        std::map<std::string, SlotState> slots_;
        int level_ = 0;
        bool rechecking_ = false;
        bool destroying_ = false;
        int effectMutationDepth_ = 0;
    };
}
