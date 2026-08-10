#include "golarion/character/skills.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
    struct CanonicalSpecialization
    {
        std::string_view id;
        std::string_view displayName;
    };

    constexpr std::array SpecializedSkillTypes{
        golarion::SkillType::Craft,
        golarion::SkillType::Perform,
        golarion::SkillType::Profession
    };

    constexpr std::array SkillCategories{
        std::string_view("skill.all"),
        std::string_view("skill.craft"),
        std::string_view("skill.perform"),
        std::string_view("skill.profession"),
        std::string_view("skill.knowledge"),
        std::string_view("skill.armorCheckPenalty")
    };

    constexpr std::array CraftSpecializations{
        CanonicalSpecialization{.id = "alchemy", .displayName = "Alchimia"},
        CanonicalSpecialization{.id = "armor", .displayName = "Armature"},
        CanonicalSpecialization{.id = "baskets", .displayName = "Ceste"},
        CanonicalSpecialization{.id = "books", .displayName = "Libri"},
        CanonicalSpecialization{.id = "bows", .displayName = "Archi"},
        CanonicalSpecialization{.id = "calligraphy", .displayName = "Calligrafia"},
        CanonicalSpecialization{.id = "carpentry", .displayName = "Carpenteria"},
        CanonicalSpecialization{.id = "cloth", .displayName = "Tessitura"},
        CanonicalSpecialization{.id = "clothing", .displayName = "Sartoria"},
        CanonicalSpecialization{.id = "glass", .displayName = "Vetreria"},
        CanonicalSpecialization{.id = "jewelry", .displayName = "Oreficeria"},
        CanonicalSpecialization{.id = "leather", .displayName = "Lavorare Pellami"},
        CanonicalSpecialization{.id = "locks", .displayName = "Ferramenta"},
        CanonicalSpecialization{.id = "paintings", .displayName = "Pittura"},
        CanonicalSpecialization{.id = "pottery", .displayName = "Ceramica"},
        CanonicalSpecialization{.id = "sculptures", .displayName = "Scultura"},
        CanonicalSpecialization{.id = "ships", .displayName = "Costruire Navi"},
        CanonicalSpecialization{.id = "shoes", .displayName = "Riparare Scarpe"},
        CanonicalSpecialization{.id = "stonemasonry", .displayName = "Lavori in Muratura"},
        CanonicalSpecialization{.id = "traps", .displayName = "Costruire Trappole"},
        CanonicalSpecialization{.id = "weapons", .displayName = "Armi"}
    };

    constexpr std::array PerformSpecializations{
        CanonicalSpecialization{.id = "singing", .displayName = "Canto"},
        CanonicalSpecialization{.id = "comedy", .displayName = "Commedia"},
        CanonicalSpecialization{.id = "dance", .displayName = "Danza"},
        CanonicalSpecialization{.id = "oratory", .displayName = "Oratoria"},
        CanonicalSpecialization{.id = "acting", .displayName = "Recitazione"},
        CanonicalSpecialization{.id = "stringInstruments", .displayName = "Strumenti a Corda"},
        CanonicalSpecialization{.id = "windInstruments", .displayName = "Strumenti a Fiato"},
        CanonicalSpecialization{.id = "percussionInstruments", .displayName = "Strumenti a Percussione"},
        CanonicalSpecialization{.id = "keyboardInstruments", .displayName = "Strumenti a Tastiera"}
    };

    constexpr std::array ProfessionSpecializations{
        CanonicalSpecialization{.id = "animalBreeder", .displayName = "Allevatore"},
        CanonicalSpecialization{.id = "architect", .displayName = "Architetto"},
        CanonicalSpecialization{.id = "lawyer", .displayName = "Avvocato"},
        CanonicalSpecialization{.id = "boatman", .displayName = "Barcaiolo"},
        CanonicalSpecialization{.id = "librarian", .displayName = "Bibliotecario"},
        CanonicalSpecialization{.id = "brewer", .displayName = "Birraio"},
        CanonicalSpecialization{.id = "forester", .displayName = "Boscaiolo"},
        CanonicalSpecialization{.id = "hunter", .displayName = "Cacciatore"},
        CanonicalSpecialization{.id = "caravaner", .displayName = "Carovaniere"},
        CanonicalSpecialization{.id = "tanner", .displayName = "Conciatore"},
        CanonicalSpecialization{.id = "constable", .displayName = "Conestabile"},
        CanonicalSpecialization{.id = "accountant", .displayName = "Contabile"},
        CanonicalSpecialization{.id = "farmer", .displayName = "Contadino"},
        CanonicalSpecialization{.id = "courtier", .displayName = "Cortigiano"},
        CanonicalSpecialization{.id = "cook", .displayName = "Cuoco"},
        CanonicalSpecialization{.id = "herbalist", .displayName = "Erborista"},
        CanonicalSpecialization{.id = "pharmacist", .displayName = "Farmacista"},
        CanonicalSpecialization{.id = "baker", .displayName = "Fornaio"},
        CanonicalSpecialization{.id = "gardener", .displayName = "Giardiniere"},
        CanonicalSpecialization{.id = "gambler", .displayName = "Giocatore d'Azzardo"},
        CanonicalSpecialization{.id = "guide", .displayName = "Guida"},
        CanonicalSpecialization{.id = "engineer", .displayName = "Ingegnere"},
        CanonicalSpecialization{.id = "midwife", .displayName = "Levatrice"},
        CanonicalSpecialization{.id = "innkeeper", .displayName = "Locandiere"},
        CanonicalSpecialization{.id = "butcher", .displayName = "Macellaio"},
        CanonicalSpecialization{.id = "sailor", .displayName = "Marinaio"},
        CanonicalSpecialization{.id = "merchant", .displayName = "Mercante"},
        CanonicalSpecialization{.id = "miner", .displayName = "Minatore"},
        CanonicalSpecialization{.id = "miller", .displayName = "Mugnaio"},
        CanonicalSpecialization{.id = "shepherd", .displayName = "Pastore"},
        CanonicalSpecialization{.id = "fisherman", .displayName = "Pescatore"},
        CanonicalSpecialization{.id = "scribe", .displayName = "Scrivano"},
        CanonicalSpecialization{.id = "steward", .displayName = "Siniscalco"},
        CanonicalSpecialization{.id = "soldier", .displayName = "Soldato"},
        CanonicalSpecialization{.id = "stableMaster", .displayName = "Stalliere"},
        CanonicalSpecialization{.id = "woodcutter", .displayName = "Taglialegna"}
    };

    bool isKnowledge(golarion::SkillType type)
    {
        return type >= golarion::SkillType::KnowledgeArcana && type <= golarion::SkillType::KnowledgeReligion;
    }

    std::vector<std::string> categoriesFor(golarion::SkillType type)
    {
        std::vector<std::string> categories{"skill.all"};
        if (golarion::requiresSpecialization(type))
        {
            categories.emplace_back(golarion::resourceName(type));
        }
        if (isKnowledge(type))
        {
            categories.emplace_back("skill.knowledge");
        }
        if (golarion::appliesArmorCheckPenalty(type))
        {
            categories.emplace_back("skill.armorCheckPenalty");
        }
        return categories;
    }

    bool isCanonicalSpecialization(golarion::SkillType type, std::string_view specializationId)
    {
        const auto containsId = [specializationId](const auto &specializations)
        {
            return std::ranges::any_of(specializations, [specializationId](const CanonicalSpecialization &specialization)
            {
                return specialization.id == specializationId;
            });
        };

        switch (type)
        {
            case golarion::SkillType::Craft:
                return containsId(CraftSpecializations);
            case golarion::SkillType::Perform:
                return containsId(PerformSpecializations);
            case golarion::SkillType::Profession:
                return containsId(ProfessionSpecializations);
            default:
                return false;
        }
    }
}

namespace golarion
{
    Skills::Skills(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        for (std::string_view category : SkillCategories)
        {
            resourceManager_.registerEnhanceableResource(category);
        }

        for (SkillType type : SpecializedSkillTypes)
        {
            specializations_.try_emplace(type);
            specializationDefaults_.emplace(type, SpecializationDefaults{
                .abilityType = defaultAbility(type),
                .classSkill = false
            });
        }

        const std::size_t skillTypeCount = static_cast<std::size_t>(SkillType::Fly) + 1;
        for (std::size_t index = 0; index < skillTypeCount; ++index)
        {
            const SkillType type = static_cast<SkillType>(index);
            if (requiresSpecialization(type))
            {
                continue;
            }

            auto [entry, inserted] = skills_.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(type));
            if (!inserted)
            {
                throw std::logic_error("standard skill is duplicated");
            }
            entry->second.registerResources(resourceManager_, categoriesFor(type));
        }

        for (const CanonicalSpecialization &specialization : CraftSpecializations)
        {
            addSpecialization(SkillType::Craft, std::string(specialization.id), std::string(specialization.displayName));
        }
        for (const CanonicalSpecialization &specialization : PerformSpecializations)
        {
            addSpecialization(SkillType::Perform, std::string(specialization.id), std::string(specialization.displayName));
        }
        for (const CanonicalSpecialization &specialization : ProfessionSpecializations)
        {
            addSpecialization(SkillType::Profession, std::string(specialization.id), std::string(specialization.displayName));
        }
    }

    void Skills::setRanks(SkillType type, int ranks)
    {
        skill(type).setRanks(ranks);
    }

    void Skills::setClassSkill(SkillType type, bool classSkill)
    {
        if (!requiresSpecialization(type))
        {
            skill(type).setClassSkill(classSkill);
            return;
        }

        specializationDefaults(type).classSkill = classSkill;
        for (auto &[specializationId, specializedSkill] : specializations_.at(type))
        {
            specializedSkill.setClassSkill(classSkill);
        }
    }

    void Skills::setAbilityType(SkillType type, AbilityType abilityType)
    {
        if (!requiresSpecialization(type))
        {
            skill(type).setAbilityType(abilityType);
            return;
        }

        specializationDefaults(type).abilityType = abilityType;
        for (auto &[specializationId, specializedSkill] : specializations_.at(type))
        {
            specializedSkill.setAbilityType(abilityType);
        }
    }

    void Skills::addSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization)
    {
        if (!requiresSpecialization(type))
        {
            throw std::invalid_argument("skill does not support specializations");
        }

        const std::string normalizedId = normalize(specializationId);
        auto &specializedSkills = specializations_.at(type);
        if (specializedSkills.contains(normalizedId))
        {
            throw std::invalid_argument("skill specialization is already registered: " + normalizedId);
        }

        auto [entry, inserted] = specializedSkills.emplace(std::piecewise_construct, std::forward_as_tuple(normalizedId), std::forward_as_tuple(type, normalizedId, specialization));
        if (!inserted)
        {
            throw std::logic_error("skill specialization insertion failed");
        }

        const SpecializationDefaults &defaults = specializationDefaults(type);
        entry->second.setAbilityType(defaults.abilityType);
        entry->second.setClassSkill(defaults.classSkill);
        entry->second.registerResources(resourceManager_, categoriesFor(type));
    }

    void Skills::setSpecializationRanks(SkillType type, const std::string &specializationId, int ranks)
    {
        specialization(type, specializationId).setRanks(ranks);
    }

    SkillsView Skills::toView()
    {
        std::size_t skillCount = skills_.size();
        for (const auto &entry : specializations_)
        {
            skillCount += entry.second.size();
        }

        std::vector<SkillView> skillViews;
        skillViews.reserve(skillCount);

        for (const auto &entry : skills_)
        {
            skillViews.push_back(entry.second.toView(resourceManager_));
        }

        for (const auto &[type, specializedSkills] : specializations_)
        {
            for (const auto &[specializationId, specializedSkill] : specializedSkills)
            {
                SkillView view = specializedSkill.toView(resourceManager_);
                view.custom = !isCanonicalSpecialization(type, specializationId);
                skillViews.push_back(std::move(view));
            }
        }

        std::ranges::sort(skillViews, [](const SkillView &left, const SkillView &right)
        {
            if (left.type != right.type)
            {
                return left.type < right.type;
            }
            return left.specializationId < right.specializationId;
        });

        return SkillsView{.skills = std::move(skillViews)};
    }

    SkillsSaveData Skills::toSaveData() const
    {
        std::size_t skillCount = skills_.size();
        for (const auto &entry : specializations_)
        {
            skillCount += entry.second.size();
        }

        std::vector<SkillSaveData> skillData;
        skillData.reserve(skillCount);

        for (const auto &entry : skills_)
        {
            skillData.push_back(entry.second.toSaveData());
        }

        for (const auto &[type, specializedSkills] : specializations_)
        {
            for (const auto &[specializationId, specializedSkill] : specializedSkills)
            {
                SkillSaveData data = specializedSkill.toSaveData();
                data.custom = !isCanonicalSpecialization(type, specializationId);
                skillData.push_back(std::move(data));
            }
        }

        std::ranges::sort(skillData, [](const SkillSaveData &left, const SkillSaveData &right)
        {
            if (left.type != right.type)
            {
                return left.type < right.type;
            }
            return left.specializationId < right.specializationId;
        });

        return SkillsSaveData{.skills = std::move(skillData)};
    }

    void Skills::load(const SkillsSaveData &data)
    {
        using SkillIdentity = std::pair<SkillType, std::string>;
        std::set<SkillIdentity> identities;
        std::map<SkillType, SpecializationDefaults> loadedSpecializationDefaults;

        for (const SkillSaveData &skillData : data.skills)
        {
            if (skillData.ranks < 0)
            {
                throw std::invalid_argument("skill ranks must not be negative");
            }

            const bool specialized = requiresSpecialization(skillData.type);
            if (!specialized)
            {
                if (skillData.specializationId || skillData.specialization || skillData.custom)
                {
                    throw std::invalid_argument("standard skill save contains specialization data");
                }
                if (!identities.emplace(skillData.type, std::string{}).second)
                {
                    throw std::invalid_argument("skill is duplicated in save data");
                }
                continue;
            }

            if (!skillData.specializationId || !skillData.specialization)
            {
                throw std::invalid_argument("specialized skill save is missing specialization data");
            }

            const std::string normalizedId = normalize(*skillData.specializationId);
            const bool canonical = isCanonicalSpecialization(skillData.type, normalizedId);
            if (skillData.custom == canonical)
            {
                throw std::invalid_argument("skill specialization kind does not match the canonical catalog: " + normalizedId);
            }
            if (!identities.emplace(skillData.type, normalizedId).second)
            {
                throw std::invalid_argument("skill specialization is duplicated in save data: " + normalizedId);
            }

            const SpecializationDefaults defaults{
                .abilityType = skillData.abilityType,
                .classSkill = skillData.classSkill
            };
            auto [entry, inserted] = loadedSpecializationDefaults.emplace(skillData.type, defaults);
            if (!inserted && (entry->second.abilityType != defaults.abilityType || entry->second.classSkill != defaults.classSkill))
            {
                throw std::invalid_argument("specializations of the same skill have inconsistent defaults");
            }
        }

        for (const auto &entry : skills_)
        {
            if (!identities.contains(SkillIdentity{entry.first, std::string{}}))
            {
                throw std::invalid_argument("standard skill is missing from save data");
            }
        }
        for (const auto &[type, specializedSkills] : specializations_)
        {
            for (const auto &entry : specializedSkills)
            {
                if (!identities.contains(SkillIdentity{type, entry.first}))
                {
                    throw std::invalid_argument("canonical skill specialization is missing from save data: " + entry.first);
                }
            }
        }

        for (const SkillSaveData &skillData : data.skills)
        {
            if (skillData.custom)
            {
                addSpecialization(skillData.type, *skillData.specializationId, *skillData.specialization);
            }
        }

        for (const auto &[type, defaults] : loadedSpecializationDefaults)
        {
            setAbilityType(type, defaults.abilityType);
            setClassSkill(type, defaults.classSkill);
        }

        for (const SkillSaveData &skillData : data.skills)
        {
            if (requiresSpecialization(skillData.type))
            {
                setSpecializationRanks(skillData.type, *skillData.specializationId, skillData.ranks);
                continue;
            }

            Skill &standardSkill = skill(skillData.type);
            standardSkill.setAbilityType(skillData.abilityType);
            standardSkill.setClassSkill(skillData.classSkill);
            standardSkill.setRanks(skillData.ranks);
        }
    }

    std::string Skills::removableSpecializationResourceName(SkillType type, const std::string &specializationId) const
    {
        if (!requiresSpecialization(type))
        {
            throw std::invalid_argument("skill does not support specializations");
        }

        const std::string normalizedId = normalize(specializationId);
        const auto &specializedSkills = specializations_.at(type);
        if (!specializedSkills.contains(normalizedId))
        {
            throw std::invalid_argument("skill specialization is not registered: " + normalizedId);
        }
        if (isCanonicalSpecialization(type, normalizedId))
        {
            throw std::invalid_argument("canonical skill specialization cannot be removed: " + normalizedId);
        }

        return std::string(resourceName(type)) + "." + normalizedId;
    }

    void Skills::removeSpecialization(SkillType type, const std::string &specializationId)
    {
        const std::string normalizedId = normalize(specializationId);
        const std::string specializationResource = removableSpecializationResourceName(type, normalizedId);
        resourceManager_.unregisterEnhanceableResource(specializationResource);
        specializations_.at(type).erase(normalizedId);
    }

    Skill &Skills::skill(SkillType type)
    {
        auto entry = skills_.find(type);
        if (entry == skills_.end())
        {
            if (requiresSpecialization(type))
            {
                throw std::invalid_argument("skill requires a specialization");
            }
            throw std::invalid_argument("skill is not registered");
        }
        return entry->second;
    }

    Skill &Skills::specialization(SkillType type, const std::string &specializationId)
    {
        if (!requiresSpecialization(type))
        {
            throw std::invalid_argument("skill does not support specializations");
        }

        const std::string normalizedId = normalize(specializationId);
        auto &specializedSkills = specializations_.at(type);
        auto entry = specializedSkills.find(normalizedId);
        if (entry == specializedSkills.end())
        {
            throw std::invalid_argument("skill specialization is not registered: " + normalizedId);
        }
        return entry->second;
    }

    Skills::SpecializationDefaults &Skills::specializationDefaults(SkillType type)
    {
        auto defaults = specializationDefaults_.find(type);
        if (defaults == specializationDefaults_.end())
        {
            throw std::invalid_argument("skill does not support specializations");
        }
        return defaults->second;
    }
}
