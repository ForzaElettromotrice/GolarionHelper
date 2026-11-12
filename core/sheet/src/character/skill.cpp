#include "golarion/character/skill.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/data/skill_save_data.hpp"
#include "golarion/view/skill_view.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    struct SkillDefinition
    {
        std::string_view displayName;
        std::string_view resourceName;
        golarion::AbilityType abilityType;
        bool trainedOnly;
        bool requiresSpecialization;
        bool appliesArmorCheckPenalty;
    };

    constexpr std::array SkillDefinitions{
        SkillDefinition{.displayName = "Acrobazia", .resourceName = "skill.acrobatics", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Addestrare Animali", .resourceName = "skill.handleAnimal", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Artigianato", .resourceName = "skill.craft", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = false, .requiresSpecialization = true, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Artista della Fuga", .resourceName = "skill.escapeArtist", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Camuffare", .resourceName = "skill.disguise", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Cavalcare", .resourceName = "skill.ride", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Conoscenze (arcane)", .resourceName = "skill.knowledge.arcana", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (dungeon)", .resourceName = "skill.knowledge.dungeoneering", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (ingegneria)", .resourceName = "skill.knowledge.engineering", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (geografia)", .resourceName = "skill.knowledge.geography", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (storia)", .resourceName = "skill.knowledge.history", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (locali)", .resourceName = "skill.knowledge.local", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (natura)", .resourceName = "skill.knowledge.nature", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (nobiltà)", .resourceName = "skill.knowledge.nobility", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (piani)", .resourceName = "skill.knowledge.planes", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Conoscenze (religioni)", .resourceName = "skill.knowledge.religion", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Diplomazia", .resourceName = "skill.diplomacy", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Disattivare Congegni", .resourceName = "skill.disableDevice", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Furtività", .resourceName = "skill.stealth", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Guarire", .resourceName = "skill.heal", .abilityType = golarion::AbilityType::Wisdom, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Intimidire", .resourceName = "skill.intimidate", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Intrattenere", .resourceName = "skill.perform", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = false, .requiresSpecialization = true, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Intuizione", .resourceName = "skill.senseMotive", .abilityType = golarion::AbilityType::Wisdom, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Linguistica", .resourceName = "skill.linguistics", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Nuotare", .resourceName = "skill.swim", .abilityType = golarion::AbilityType::Strength, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Percezione", .resourceName = "skill.perception", .abilityType = golarion::AbilityType::Wisdom, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Professione", .resourceName = "skill.profession", .abilityType = golarion::AbilityType::Wisdom, .trainedOnly = true, .requiresSpecialization = true, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Raggirare", .resourceName = "skill.bluff", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Rapidità di Mano", .resourceName = "skill.sleightOfHand", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Sapienza Magica", .resourceName = "skill.spellcraft", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Scalare", .resourceName = "skill.climb", .abilityType = golarion::AbilityType::Strength, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true},
        SkillDefinition{.displayName = "Sopravvivenza", .resourceName = "skill.survival", .abilityType = golarion::AbilityType::Wisdom, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Utilizzare Congegni Magici", .resourceName = "skill.useMagicDevice", .abilityType = golarion::AbilityType::Charisma, .trainedOnly = true, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Valutare", .resourceName = "skill.appraise", .abilityType = golarion::AbilityType::Intelligence, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = false},
        SkillDefinition{.displayName = "Volare", .resourceName = "skill.fly", .abilityType = golarion::AbilityType::Dexterity, .trainedOnly = false, .requiresSpecialization = false, .appliesArmorCheckPenalty = true}
    };

    const SkillDefinition &definition(golarion::SkillType type)
    {
        const auto index = static_cast<std::size_t>(type);
        if (index >= SkillDefinitions.size())
        {
            throw std::invalid_argument("unknown skill type");
        }
        return SkillDefinitions[index];
    }

    std::string normalizedSpecializationId(std::string_view value)
    {
        const std::string id = golarion::normalize(value);
        const bool valid = std::ranges::all_of(id, [](unsigned char character)
        {
            return std::isalnum(character) != 0 || character == '_';
        });
        if (!valid)
        {
            throw std::invalid_argument("specialization id must contain only letters, digits, and underscores");
        }
        return id;
    }

    int checkedSkillValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("skill value is out of range");
        }
        return static_cast<int>(value);
    }
}

namespace golarion
{
    std::string_view displayName(SkillType type)
    {
        return definition(type).displayName;
    }

    std::string_view resourceName(SkillType type)
    {
        return definition(type).resourceName;
    }

    std::string skillCheckResourceName(AbilityType abilityType)
    {
        return "skillCheck." + std::string(resourceName(abilityType));
    }

    AbilityType defaultAbility(SkillType type)
    {
        return definition(type).abilityType;
    }

    bool trainedOnly(SkillType type)
    {
        return definition(type).trainedOnly;
    }

    bool requiresSpecialization(SkillType type)
    {
        return definition(type).requiresSpecialization;
    }

    bool appliesArmorCheckPenalty(SkillType type)
    {
        return definition(type).appliesArmorCheckPenalty;
    }

    std::string skillRankGrantsResource(SkillType type, std::optional<std::string> specializationId)
    {
        if (requiresSpecialization(type) != specializationId.has_value())
            throw std::invalid_argument("skill rank grant specialization does not match skill type");
        return std::string(resourceName(type)) + (specializationId ? "." + normalizedSpecializationId(*specializationId) : "") + ".rankGrants";
    }

    ArmorCheckPenalty::ArmorCheckPenalty(ArmorCheckPenaltyDefinition definition)
        : id_(normalize(definition.id)), source_(normalize(definition.source)), expression_(normalize(definition.expression))
    {
    }

    Skill::Skill(SkillType type)
        : type_(type),
          resourceName_(resourceName(type)),
          ranks_(0)
    {
        if (requiresSpecialization(type))
        {
            throw std::invalid_argument("skill requires a specialization");
        }
    }

    Skill::Skill(SkillType type, const std::string &specializationId, const std::string &specialization)
        : type_(type),
          specializationId_(normalizedSpecializationId(specializationId)),
          specialization_(normalize(specialization)),
          resourceName_(std::string(resourceName(type)) + "." + *specializationId_),
          ranks_(0)
    {
        if (!requiresSpecialization(type))
        {
            throw std::invalid_argument("skill does not support specializations");
        }
    }

    SkillAbilityReplacement::SkillAbilityReplacement(SkillAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          abilityType_(definition.abilityType)
    {
    }

    SkillClassSkillGrant::SkillClassSkillGrant(SkillClassSkillGrantDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName))
    {
    }

    void Skill::setRanks(int ranks)
    {
        if (ranks < 0)
        {
            throw std::invalid_argument("skill ranks must not be negative");
        }
        ranks_ = ranks;
    }

    void Skill::registerResources(ResourceManager &resourceManager) const
    {
        registerResources(resourceManager, {});
    }

    void Skill::registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const
    {
        resourceManager.registerEnhanceableResource(resourceName_, std::move(parentResources));
        try
        {
            resourceManager.registerCollectionResource<SkillRankGrant>(skillRankGrantsResource(type_, specializationId_),
                [this](SkillRankGrant grant)
                {
                    grant.id = normalize(grant.id);
                    grant.source = normalize(grant.source);
                    grant.expression = normalize(grant.expression);
                    const auto id = grant.id;
                    if (!rankGrants_.emplace(id, std::move(grant)).second) throw std::invalid_argument("skill rank grant is already registered");
                },
                [this](std::string_view id)
                {
                    if (!rankGrants_.erase(normalize(id))) throw std::invalid_argument("skill rank grant is not registered");
                });
        }
        catch (...) { resourceManager.unregisterEnhanceableResource(resourceName_); throw; }
    }

    SkillView Skill::toView(ResourceManager &resourceManager, const std::map<std::string, SkillAbilityReplacement> &abilityReplacements, const std::map<std::string, SkillClassSkillGrant> &classSkillGrants, int armorCheckPenalty) const
    {
        if (armorCheckPenalty < 0)
        {
            throw std::invalid_argument("armor check penalty must not be negative");
        }
        const int appliedArmorCheckPenalty = appliesArmorCheckPenalty(type_) ? armorCheckPenalty : 0;
        ModifierSetView modifierView = resourceManager.modifierSetView(resourceName_);
        std::vector<SkillClassSkillGrantView> applicableClassSkillGrants;
        for (const auto &[id, grant] : classSkillGrants)
        {
            if (resourceManager.enhanceableResourceIsOrInheritsFrom(resourceName_, grant.targetResourceName_))
            {
                applicableClassSkillGrants.push_back(SkillClassSkillGrantView{
                    .id = id,
                    .source = grant.source_,
                    .targetResourceName = grant.targetResourceName_
                });
            }
        }
        const bool classSkill = !applicableClassSkillGrants.empty();
        const int effective = effectiveRanks(resourceManager);
        const int classSkillBonus = classSkill && effective > 0 ? 3 : 0;
        std::vector<SkillRankGrantView> rankViews;
        for (const auto &[id, grant] : rankGrants_)
            rankViews.push_back({id, grant.source, grant.expression, resourceManager.evaluateExpression(grant.expression)});
        std::vector<SkillAbilityOptionView> abilityOptions;
        const AbilityType baseAbilityType = defaultAbility(type_);
        const int baseAbilityModifier = resourceManager.targetValue(std::string(resourceName(baseAbilityType)) + "Mod");
        ModifierSetView baseOptionModifiers = resourceManager.modifierSetView(std::vector<std::string>{resourceName_, skillCheckResourceName(baseAbilityType)});
        abilityOptions.push_back(SkillAbilityOptionView{
            .replacementId = std::nullopt,
            .source = "Base",
            .abilityType = baseAbilityType,
            .abilityModifier = baseAbilityModifier,
            .totalValue = checkedSkillValue(static_cast<long long>(baseAbilityModifier) + effective + classSkillBonus + baseOptionModifiers.total - appliedArmorCheckPenalty),
            .modifiers = std::move(baseOptionModifiers)
        });
        for (const auto &[id, replacement] : abilityReplacements)
        {
            if (!resourceManager.enhanceableResourceIsOrInheritsFrom(resourceName_, replacement.targetResourceName_))
            {
                continue;
            }
            const int abilityModifier = resourceManager.targetValue(std::string(resourceName(replacement.abilityType_)) + "Mod");
            ModifierSetView optionModifiers = resourceManager.modifierSetView(std::vector<std::string>{resourceName_, skillCheckResourceName(replacement.abilityType_)});
            abilityOptions.push_back(SkillAbilityOptionView{
                .replacementId = id,
                .source = replacement.source_,
                .abilityType = replacement.abilityType_,
                .abilityModifier = abilityModifier,
                .totalValue = checkedSkillValue(static_cast<long long>(abilityModifier) + effective + classSkillBonus + optionModifiers.total - appliedArmorCheckPenalty),
                .modifiers = std::move(optionModifiers)
            });
        }

        return SkillView{
            .type = type_,
            .specializationId = specializationId_,
            .name = specialization_.value_or(std::string(displayName(type_))),
            .resourceName = resourceName_,
            .ranks = ranks_,
            .classSkill = classSkill,
            .classSkillBonus = classSkillBonus,
            .trainedOnly = trainedOnly(type_),
            .usable = !trainedOnly(type_) || effective > 0,
            .custom = specializationId_.has_value(),
            .appliedArmorCheckPenalty = appliedArmorCheckPenalty,
            .classSkillGrants = std::move(applicableClassSkillGrants),
            .abilityOptions = std::move(abilityOptions),
            .modifiers = std::move(modifierView),
            .effectiveRanks = effective,
            .rankGrants = std::move(rankViews)
        };
    }

    SkillSaveData Skill::toSaveData() const
    {
        return SkillSaveData{
            .type = type_,
            .specializationId = specializationId_,
            .specialization = specialization_,
            .ranks = ranks_,
            .custom = specializationId_.has_value()
        };
    }

    int Skill::totalValue(ResourceManager &resourceManager, int armorCheckPenalty) const
    {
        if (armorCheckPenalty < 0)
        {
            throw std::invalid_argument("armor check penalty must not be negative");
        }
        const std::string abilityModifierTarget = std::string(resourceName(defaultAbility(type_))) + "Mod";
        const std::vector<std::string> modifierResources{resourceName_, skillCheckResourceName(defaultAbility(type_))};
        const long long total = static_cast<long long>(resourceManager.targetValue(abilityModifierTarget))
                                + effectiveRanks(resourceManager)
                                + resourceManager.modifierTotal(modifierResources)
                                - (appliesArmorCheckPenalty(type_) ? armorCheckPenalty : 0);
        return checkedSkillValue(total);
    }

    int Skill::effectiveRanks(ResourceManager &resourceManager) const
    {
        int result = ranks_;
        for (const auto &[id, grant] : rankGrants_)
        {
            const int granted = resourceManager.evaluateExpression(grant.expression);
            if (granted < 0) throw std::invalid_argument("granted skill ranks must not be negative");
            result = std::max(result, granted);
        }
        return result;
    }

    bool Skill::usable(ResourceManager &resourceManager) const
    {
        return !trainedOnly(type_) || effectiveRanks(resourceManager) > 0;
    }
}
