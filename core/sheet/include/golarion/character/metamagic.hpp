#pragma once
#include "golarion/character/metamagic_types.hpp"

#include "golarion/character/damage.hpp"
#include "golarion/character/spell_definition.hpp"
#include "golarion/data/prepared_metamagic_data.hpp"
#include "golarion/resource/usage_pool.hpp"

#include <array>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct MetamagicGrantView;
    struct MetamagicUsageCostView;

    struct HeightenMetamagicOptions { int effectiveSpellLevel; };
    struct ReachMetamagicOptions { int rangeSteps; };
    struct ElementalMetamagicOptions
    {
        DamageType energy;
        ElementalDamageMode mode;
    };
    using MetamagicOptions = std::variant<std::monostate, HeightenMetamagicOptions, ReachMetamagicOptions, ElementalMetamagicOptions>;

    struct MetamagicApplication
    {
        MetamagicType type;
        std::string grantId;
        MetamagicOptions options;
    };

    // Chosen at casting time, even when Selective was applied during preparation.
    // Target existence, area membership and the ability-based limit belong to casting.
    struct SelectiveMetamagicTargets { std::vector<std::string> excludedTargetIds; };

    class MetamagicUsagePool final
    {
    public:
        explicit MetamagicUsagePool(std::string_view id, int remaining);
        // The source owns recovery and persistence; this adapter never copies its balance.
        MetamagicUsagePool(std::string_view id, std::shared_ptr<UsagePool> pool);
        MetamagicUsagePool(const MetamagicUsagePool &) = delete;
        MetamagicUsagePool &operator=(const MetamagicUsagePool &) = delete;
        int remaining() const noexcept { return linkedPool_ ? linkedPool_->remaining() : remaining_; }
        bool isExternallyOwned() const noexcept { return static_cast<bool>(linkedPool_); }
        const std::string &id() const noexcept { return id_; }
        void reset(int remaining);
    private:
        friend class MetamagicRegistry;
        std::string id_;
        int remaining_;
        std::shared_ptr<UsagePool> linkedPool_;
    };

    struct MetamagicUsageCost
    {
        std::string poolId;
        int uses = 1;
        std::shared_ptr<MetamagicUsagePool> pool;
    };

    struct MetamagicGrant
    {
        std::string id;
        std::string source;
        MetamagicType type;
        std::optional<DamageType> elementalEnergy;
        MetamagicSlotCostPolicy slotCost = MetamagicSlotCostPolicy::Standard;
        std::optional<MetamagicRodGrade> rodGrade;
        // Reference to the source's pool, not a copied counter. Registration never refills it.
        std::optional<MetamagicUsageCost> usageCost;
        CastingMetamagicScope castingScope = CastingMetamagicScope::AnyCast;
        // Required for applicability that the current spell schema cannot express.
        // Pure query; must not mutate grants, usage pools or casting state.
        std::function<bool(const SpellDefinition &, const MetamagicApplication &)> isApplicable;
    };

    struct MetamagicEvaluation
    {
        std::string spellId;
        int baseSpellLevel = 0;
        int effectiveSpellLevel = 0;
        int requiredSlotLevel = 0;
        int nominalSlotLevel = 0;
        std::vector<MetamagicApplication> applications;
        // Only costs incurred by this operation, never costs of an earlier preparation.
        std::vector<MetamagicUsageCost> usageCosts;
    };

    PreparedMetamagicData preparationSnapshot(const MetamagicEvaluation &evaluation, CharacterClassType spellListClass, int paidSlots = 1);
    MetamagicEvaluation restorePreparationSnapshot(std::string_view spellId, const std::vector<MetamagicApplication> &applications, const PreparedMetamagicData &snapshot, CharacterClassType spellListClass);
    using MetamagicUsagePools = std::map<std::string, std::shared_ptr<MetamagicUsagePool>>;
    void collectMetamagicPool(MetamagicUsagePools &pools, const MetamagicUsageCost &cost);
    MetamagicUsageCostView metamagicUsageCostView(const MetamagicUsageCost &cost);

    struct MetamagicLevelAdjustment
    {
        int slotLevelIncrease;
        int effectiveLevelIncrease;
    };

    // Structural validation and ordinary costs only, not spell applicability or reductions.
    void validateMetamagicApplication(const MetamagicApplication &application);
    MetamagicLevelAdjustment standardMetamagicAdjustment(const MetamagicApplication &application, int baseSpellLevel);
    int metamagicRodMaximumSpellLevel(MetamagicRodGrade grade);

    std::string preparationMetamagicGrantsResource(std::string_view castingId);
    std::string castingMetamagicGrantsResource(std::string_view castingId);

    class MetamagicRegistry final
    {
    public:
        explicit MetamagicRegistry(ResourceManager &resources, std::string_view castingId);
        ~MetamagicRegistry();
        MetamagicRegistry(const MetamagicRegistry &) = delete;
        MetamagicRegistry &operator=(const MetamagicRegistry &) = delete;
        MetamagicRegistry(MetamagicRegistry &&) = delete;
        MetamagicRegistry &operator=(MetamagicRegistry &&) = delete;

        // Resolve a grant snapshot in exactly one phase; does not spend source resources.
        MetamagicGrant resolve(MetamagicPhase phase, const MetamagicApplication &application) const;
        MetamagicEvaluation evaluate(MetamagicPhase phase, const SpellDefinition &spell, int baseSpellLevel, const std::vector<MetamagicApplication> &applications, bool conversion = false, const MetamagicEvaluation *prepared = nullptr) const;
        static void pay(const std::vector<MetamagicUsageCost> &costs);
        std::vector<MetamagicGrantView> toView() const;
        MetamagicUsagePools usagePools() const;

    private:
        void add(MetamagicPhase phase, MetamagicGrant grant);
        void remove(MetamagicPhase phase, std::string_view id);
        ResourceManager &resources_;
        std::array<std::string, 2> endpoints_;
        std::array<std::map<std::string, MetamagicGrant>, 2> grants_;
    };
}
