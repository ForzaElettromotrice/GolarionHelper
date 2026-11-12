#include "golarion/character/spell_catalog.hpp"
#include "golarion/character/granted_spells.hpp"
#include "golarion/character/spell_definition_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include <stdexcept>
#include <utility>

namespace golarion
{
    GrantedSpell::GrantedSpell(GrantedSpellDefinition definition, const SpellCatalog *catalog) : definition_(std::move(definition))
    {
        auto &d = definition_;
        d.id = normalize(d.id);
        d.source = normalize(d.source);
        d.spellId = normalize(d.spellId);
        if (catalog) catalog->get(d.spellId);
        else SpellDefinitionManager::instance().get(d.spellId);
        switch (d.usage)
        {
        case GrantedSpellUsage::Limited:
            if (d.pool || d.cost != 1) throw std::invalid_argument("limited spell must own its uses with unit cost");
            d.pool = std::make_shared<UsagePool>(d.capacity);
            break;
        case GrantedSpellUsage::SharedPool:
            if (!d.pool || d.capacity != 0 || d.cost <= 0 || d.cost > d.pool->capacity()) throw std::invalid_argument("invalid shared spell usage cost");
            break;
        case GrantedSpellUsage::AtWill:
        case GrantedSpellUsage::Constant:
            if (d.pool || d.capacity != 0 || d.cost != 1) throw std::invalid_argument("unlimited spell cannot declare a usage pool");
            break;
        default: throw std::invalid_argument("unknown granted spell usage");
        }
    }

    void GrantedSpell::consume()
    {
        if (definition_.usage == GrantedSpellUsage::Constant) throw std::invalid_argument("constant spell is not an activation");
        if (definition_.pool) definition_.pool->consume(definition_.cost);
    }
    void GrantedSpell::refill()
    {
        if (definition_.usage != GrantedSpellUsage::Limited) throw std::invalid_argument("spell does not own a usage pool");
        definition_.pool->refill();
    }
    void GrantedSpell::restore(int amount)
    {
        if (definition_.usage != GrantedSpellUsage::Limited) throw std::invalid_argument("spell does not own a usage pool");
        definition_.pool->restore(amount);
    }
    GrantedSpellView GrantedSpell::toView() const
    {
        const auto &d = definition_;
        return {d.id, d.source, d.spellId, d.usage,
            d.pool ? std::optional<int>(d.pool->capacity()) : std::nullopt,
            d.pool ? std::optional<int>(d.pool->remaining()) : std::nullopt, d.pool ? d.cost : 0};
    }

    GrantedSpells::GrantedSpells(ResourceManager &resources, const SpellCatalog *catalog) : catalog_(catalog), resources_(resources)
    {
        resources_.registerCollectionResource<GrantedSpellDefinition>(Resource,
            [this](GrantedSpellDefinition definition)
            {
                const auto id = normalize(definition.id);
                if (spells_.contains(id)) throw std::invalid_argument("spell grant already exists");
                spells_.emplace(id, GrantedSpell(std::move(definition), catalog_));
            },
            [this](std::string_view id)
            {
                if (!spells_.erase(normalize(id))) throw std::invalid_argument("spell grant does not exist");
            });
    }
    GrantedSpells::~GrantedSpells() { resources_.unregisterCollectionResource(Resource); }
    GrantedSpell &GrantedSpells::get(std::string_view id)
    {
        auto found = spells_.find(normalize(id));
        if (found == spells_.end()) throw std::invalid_argument("spell grant does not exist");
        return found->second;
    }
    SpellCastResult GrantedSpells::consume(std::string_view id)
    {
        auto &grant = get(id);
        grant.consume();
        const auto &spell = catalog_ ? catalog_->get(grant.definition_.spellId) : SpellDefinitionManager::instance().get(grant.definition_.spellId);
        return exportSpellCast(spell, resources_);
    }
    void GrantedSpells::refill(std::string_view id) { get(id).refill(); }
    void GrantedSpells::restore(std::string_view id, int amount) { get(id).restore(amount); }
    std::vector<GrantedSpellView> GrantedSpells::toView() const
    {
        std::vector<GrantedSpellView> view;
        for (const auto &[id, spell] : spells_) view.push_back(spell.toView());
        return view;
    }
    GrantedSpellsData GrantedSpells::toData() const
    {
        GrantedSpellsData data;
        for (const auto &[id, spell] : spells_)
            if (spell.definition_.usage == GrantedSpellUsage::Limited) data.spells.push_back({id, spell.definition_.pool->remaining()});
        return data;
    }
    void GrantedSpells::load(const GrantedSpellsData &data)
    {
        std::map<std::string, std::shared_ptr<UsagePool>> loaded;
        for (const auto &state : data.spells)
        {
            const auto id = normalize(state.id);
            const auto &definition = get(id).definition_;
            if (definition.usage != GrantedSpellUsage::Limited) throw std::invalid_argument("only owned spell uses may be restored here");
            if (!loaded.emplace(id, std::make_shared<UsagePool>(definition.capacity, state.remaining)).second) throw std::invalid_argument("duplicate spell usage state");
        }
        for (const auto &[id, spell] : spells_)
            if (spell.definition_.usage == GrantedSpellUsage::Limited && !loaded.contains(id)) throw std::invalid_argument("missing spell usage state");
        for (auto &[id, pool] : loaded) spells_.at(id).definition_.pool.swap(pool);
    }
}
