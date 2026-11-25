#include "sheet_controller.hpp"

#include "golarion/character/character_sheet.hpp"
#include "golarion/character/spell_definition_manager.hpp"
#include "golarion/effect/effect_json.hpp"

#include <QJsonDocument>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>

namespace
{
    using namespace golarion;
    QString text(const std::string &value) { return QString::fromStdString(value); }
    int number(auto value) { return static_cast<int>(value); }
    QVariant optional(const std::optional<std::string> &value) { return value ? QVariant(text(*value)) : QVariant{}; }
    QVariant optional(const std::optional<int> &value) { return value ? QVariant(*value) : QVariant{}; }
    std::optional<std::string> optionalText(const QVariant &value)
    {
        return value.toString().isEmpty() ? std::nullopt : std::optional(value.toString().toStdString());
    }
    template<class Range, class Convert>
    QVariantList list(const Range &range, Convert convert)
    {
        QVariantList result;
        result.reserve(static_cast<qsizetype>(range.size()));
        for (const auto &entry : range) result.push_back(convert(entry));
        return result;
    }
    QVariant optionalIds(const auto &ids) { return ids ? QVariant(list(*ids, text)) : QVariant{}; }
    QVariantMap slotView(const SpellSlotId &slot)
    {
        return {{"groupId", text(slot.groupId)}, {"spellLevel", slot.spellLevel}, {"index", static_cast<qulonglong>(slot.index)}};
    }
    SpellSlotId slotId(const QVariantMap &slot)
    {
        bool levelOk = false, indexOk = false;
        const int level = slot.value("spellLevel").toInt(&levelOk);
        const auto index = slot.value("index").toLongLong(&indexOk);
        const auto group = slot.value("groupId").toString().toStdString();
        if (group.empty() || !levelOk || !indexOk || level < 0 || level > 9 || index < 0)
            throw std::invalid_argument("invalid spell slot");
        return {group, level, static_cast<std::size_t>(index)};
    }
    QVariantMap grantView(const SpellSlotGrant &grant)
    {
        return {{"id", text(grant.id)}, {"source", text(grant.source)}, {"allowedSpellIds", optionalIds(grant.allowedSpellIds)}};
    }
    QVariantMap applicationView(const MetamagicApplication &application)
    {
        QVariantMap options;
        if (const auto *value = std::get_if<HeightenMetamagicOptions>(&application.options))
            options.insert("level", value->effectiveSpellLevel);
        if (const auto *value = std::get_if<ReachMetamagicOptions>(&application.options))
            options.insert("steps", value->rangeSteps);
        if (const auto *value = std::get_if<ElementalMetamagicOptions>(&application.options))
            options = {{"energy", number(value->energy)}, {"mode", number(value->mode)}};
        return {{"type", number(application.type)}, {"grantId", text(application.grantId)}, {"options", options}};
    }
    std::vector<MetamagicApplication> applications(const QVariantList &entries)
    {
        std::vector<MetamagicApplication> result;
        for (const auto &entry : entries)
        {
            const auto data = entry.toMap();
            const auto type = static_cast<MetamagicType>(data.value("type").toInt());
            const auto options = data.value("options").toMap();
            MetamagicOptions value;
            if (type == MetamagicType::Heighten) value = HeightenMetamagicOptions{options.value("level").toInt()};
            if (type == MetamagicType::Reach) value = ReachMetamagicOptions{options.value("steps").toInt()};
            if (type == MetamagicType::Elemental)
                value = ElementalMetamagicOptions{static_cast<DamageType>(options.value("energy").toInt()),
                    static_cast<ElementalDamageMode>(options.value("mode").toInt())};
            result.push_back({type, data.value("grantId").toString().toStdString(), value});
        }
        return result;
    }
    QVariantMap repertoireView(const SpellRepertoireView &repertoire)
    {
        return {{"manualAdditionAllowed", repertoire.manualAdditionAllowed}, {"manualSpellIds", list(repertoire.manualSpellIds, text)},
            {"listGrants", list(repertoire.listGrants, [](const auto &grant) { return text(grant.id); })},
            {"quotas", list(repertoire.quotas, [](const auto &quota) {
                QVariantMap remaining;
                for (const auto &[level, count] : quota.remainingByLevel) remaining.insert(QString::number(level), count);
                return QVariantMap{{"grant", QVariantMap{{"id", text(quota.grant.id)}, {"source", text(quota.grant.source)},
                    {"allowedSpellIds", optionalIds(quota.grant.allowedSpellIds)}}}, {"remainingByLevel", remaining}};
            })}, {"selections", list(repertoire.selections, [](const auto &entry) {
                const auto &selection = entry.selection;
                return QVariantMap{{"selection", QVariantMap{{"spellId", text(selection.spellId)}, {"quotaId", text(selection.quotaId)},
                    {"spellLevel", selection.spellLevel}}}, {"active", entry.active}};
            })}};
    }
    QVariantMap castingView(const SpellcastingView &casting)
    {
        const auto state = std::visit([](const auto &value) {
            using T = std::decay_t<decltype(value)>;
            QVariantMap result{{"spellListClass", number(value.spellListClass)}, {"metamagic", list(value.metamagic, [](const auto &grant) {
                QVariantMap entry{{"id", text(grant.id)}, {"source", text(grant.source)}, {"type", number(grant.type)},
                    {"phase", number(grant.phase)}, {"energy", grant.elementalEnergy ? QVariant(number(*grant.elementalEnergy)) : QVariant{}}};
                if (grant.slotCost == MetamagicSlotCostPolicy::Waived) entry.insert("levelAdjustment", 0);
                if (grant.usageCost) entry.insert("remaining", optional(grant.usageCost->remaining));
                return entry;
            })}};
            const auto slots = [](const auto &groups) {
                return list(groups, [](const auto &group) {
                    return QVariantMap{{"grant", grantView(group.grant)}, {"slots", list(group.slots, [](const auto &entry) {
                        QVariantMap slot{{"slot", slotView(entry.slot)}, {"state", number(entry.state)}};
                        if constexpr (requires { entry.spellId; })
                        {
                            slot.insert("spellId", optional(entry.spellId));
                            slot.insert("preparationSlot", entry.preparationSlot ? QVariant(slotView(*entry.preparationSlot)) : QVariant{});
                            slot.insert("metamagic", list(entry.metamagic, applicationView));
                        }
                        return slot;
                    })}};
                });
            };
            if constexpr (std::is_same_v<T, FlexibleSpellcastingView>)
            {
                result.insert("slotGroups", slots(value.slotGroups));
                result.insert("mixingPermissions", list(value.mixingPermissions, [](const auto &entry) {
                    return QVariantMap{{"id", text(entry.id)}, {"source", text(entry.source)}};
                }));
                result.insert("preparationGroups", list(value.preparationGroups, [](const auto &group) {
                    return QVariantMap{{"grant", grantView(group.grant)}, {"preparations", list(group.preparations, [](const auto &entry) {
                        return QVariantMap{{"preparationId", slotView(entry.preparationId)}, {"spellId", optional(entry.spellId)},
                            {"metamagic", list(entry.metamagic, applicationView)}};
                    })}};
                }));
            }
            else result.insert("groups", slots(value.groups));
            return result;
        }, casting.state);
        return {{"id", text(casting.id)}, {"source", text(casting.source)}, {"mode", number(casting.mode)},
            {"repertoire", list(casting.repertoire, text)}, {"repertoireDetails", repertoireView(casting.repertoireDetails)}, {"state", state}};
    }
    struct SpellCatalogSnapshot
    {
        QVariantList entries;
        QVariantMap names;
        std::map<int, QVariantMap> levels;

        void add(const std::string &id, const std::string &name, const std::map<CharacterClassType, int> &classLevels)
        {
            entries.push_back(QVariantMap{{"id", text(id)}, {"name", text(name)}});
            names.insert(text(id), text(name));
            for (const auto &[type, level] : classLevels)
                levels[number(type)].insert(text(id), level);
        }

        QVariantMap levelsView() const
        {
            QVariantMap result;
            for (const auto &[type, spells] : levels) result.insert(QString::number(type), spells);
            return result;
        }
    };
    const SpellCatalogSnapshot &standardCatalog()
    {
        static const auto catalog = [] {
            SpellCatalogSnapshot result;
            for (const auto &entry : SpellDefinitionManager::instance().catalogEntries())
                result.add(entry.id, entry.name, entry.levels);
            return result;
        }();
        return catalog;
    }

    using Command = SheetController::SpellCommand;
    using Consumption = std::variant<PreparedSpellConsumptionDefinition, SpontaneousSpellConsumptionDefinition, FlexibleSpellConsumptionDefinition>;
    Consumption consumption(Command command, const QVariantMap &input, std::vector<MetamagicApplication> metamagic)
    {
        switch (command)
        {
        case SheetController::ConsumePrepared:
            return PreparedSpellConsumptionDefinition{optionalText(input.value("replacement")), std::move(metamagic)};
        case SheetController::ConsumeSpontaneous:
            return SpontaneousSpellConsumptionDefinition{input.value("spellId").toString().toStdString(), std::move(metamagic)};
        case SheetController::ConsumeFlexible:
            return FlexibleSpellConsumptionDefinition{slotId(input.value("preparation").toMap()), std::move(metamagic),
                optionalText(input.value("mixingPermissionId"))};
        default: throw std::invalid_argument("invalid spell consumption command");
        }
    }
}

void SheetController::prepareStandardSpellCatalog()
{
    (void)standardCatalog();
}

QVariantMap SheetController::spellCatalogSnapshot(const golarion::CharacterSheetView &view)
{
    SpellCatalogSnapshot catalog;
    if (!view.spellcastings.castings.empty() || !view.grantedSpells.empty())
    {
        catalog = standardCatalog();
        for (const auto &definition : view.customSpellDefinitions)
            catalog.add(definition.id, definition.name, definition.spellLevelByClass);
    }
    return {{"catalog", catalog.entries}, {"names", catalog.names}, {"levels", catalog.levelsView()}};
}

QVariantMap SheetController::spellcastingSnapshot(const golarion::CharacterSheetView &view)
{
    return {{"castings", list(view.spellcastings.castings, castingView)}};
}

QVariantMap SheetController::grantedSpellSnapshot(const golarion::CharacterSheetView &view)
{
    return {{"granted", list(view.grantedSpells, [](const auto &grant) {
            return QVariantMap{{"id", text(grant.id)}, {"source", text(grant.source)}, {"spellId", text(grant.spellId)},
                {"usage", number(grant.usage)}, {"capacity", optional(grant.capacity)}, {"remaining", optional(grant.remaining)}, {"cost", grant.cost}};
        })}};
}

void SheetController::spellCommand(SpellCommand command, const QString &castingId, const QVariantMap &input)
{
    if (!sheet_) return;
    QString error;
    lastSpellCast_.clear();
    try
    {
        const auto id = castingId.toStdString();
        const auto spellId = input.value("spellId").toString().toStdString();
        const auto slot = [&] { return slotId(input.value("slot").toMap()); };
        auto metamagic = applications(input.value("metamagic").toList());
        std::optional<golarion::SpellCastResult> cast;
        switch (command)
        {
        case Prepare: {
            std::vector<golarion::SpellSlotId> slots;
            for (const auto &entry : input.value("slots").toList()) slots.push_back(slotId(entry.toMap()));
            sheet_->prepareSpell(id, slots, {spellId, std::move(metamagic)});
            break;
        }
        case PrepareFlexible: sheet_->prepareFlexibleSpell(id, slot(), {spellId, std::move(metamagic)}); break;
        case ConsumePrepared: cast = sheet_->consumePreparedSpell(id, slot(), std::get<golarion::PreparedSpellConsumptionDefinition>(consumption(command, input, std::move(metamagic)))); break;
        case ConsumeSpontaneous: cast = sheet_->consumeSpontaneousSpell(id, slot(), std::get<golarion::SpontaneousSpellConsumptionDefinition>(consumption(command, input, std::move(metamagic)))); break;
        case ConsumeFlexible: cast = sheet_->consumeFlexibleSpell(id, slot(), std::get<golarion::FlexibleSpellConsumptionDefinition>(consumption(command, input, std::move(metamagic)))); break;
        case ClearSlot: sheet_->clearSpellSlot(id, slot()); break;
        case ClearPreparation: sheet_->clearFlexiblePreparation(id, slot()); break;
        case ResetSlots: sheet_->resetSpellSlots(id); break;
        case AddToRepertoire: sheet_->addSpellToRepertoire(id, spellId); break;
        case SelectRepertoire:
            sheet_->selectRepertoireSpell(id, {input.value("quotaId").toString().toStdString(), input.value("spellLevel").toInt(), spellId});
            break;
        case ConsumeGranted: cast = sheet_->consumeGrantedSpell(id); break;
        default: throw std::invalid_argument("unknown spell command");
        }
        if (cast)
        {
            nlohmann::json effects = nlohmann::json::array();
            for (const auto &effect : cast->effects) effects.push_back(golarion::effectDefinitionToJson(effect));
            lastSpellCast_ = {{"maxTargets", cast->maxTargets},
                {"effects", QJsonDocument::fromJson(QByteArray::fromStdString(effects.dump())).toVariant()}};
        }
    }
    catch (const std::exception &)
    {
        error = tr("Impossibile eseguire l’operazione. Controlla gli slot, il repertorio e gli utilizzi disponibili.");
    }
    // An export error can occur after spending a slot or charges.
    publish(true, std::move(error));
}

int SheetController::spellPreparationCount(const QString &castingId, const QVariantMap &slot, const QString &spellId, const QVariantList &metamagic)
{
    try
    {
        return sheet_ ? sheet_->spellPreparationSlotCount(castingId.toStdString(), slotId(slot),
            {spellId.toStdString(), applications(metamagic)}) : 0;
    }
    catch (const std::exception &) { return 0; }
}

bool SheetController::canConsumeSpell(SpellCommand command, const QString &castingId, const QVariantMap &slot, const QVariantMap &input)
{
    if (!sheet_) return false;
    try
    {
        sheet_->validateSpellConsumption(castingId.toStdString(), slotId(slot),
            consumption(command, input, applications(input.value("metamagic").toList())));
        return true;
    }
    catch (const std::exception &) { return false; }
}

QVariantList SheetController::spellConversionOptions(const QString &castingId, const QVariantMap &slot)
{
    QVariantList result;
    try
    {
        if (!sheet_ || !sheet_->hasSpellConversions(castingId.toStdString())) return result;
        for (const auto &entry : spellCatalogData_.value("catalog").toList())
        {
            const auto id = entry.toMap().value("id");
            if (canConsumeSpell(ConsumePrepared, castingId, slot, {{"replacement", id}})) result.push_back(id);
        }
    }
    catch (const std::exception &) {}
    return result;
}

QVariantList SheetController::spellMetamagicOptions(SpellCommand command, const QVariantMap &casting, const QVariantMap &slot, const QVariantMap &input)
{
    QVariantList result;
    const bool preparing = command == Prepare || command == PrepareFlexible;
    const auto id = casting.value("id").toString();
    const auto state = casting.value("state").toMap();
    QMap<int, QVariantList> byType;
    for (const auto &value : state.value("metamagic").toList())
    {
        const auto grant = value.toMap();
        if (grant.value("phase").toInt() != number(preparing ? golarion::MetamagicPhase::Preparation : golarion::MetamagicPhase::Casting)
            || (!grant.value("remaining").isNull() && grant.value("remaining").toInt() <= 0)) continue;
        const int type = grant.value("type").toInt();
        const auto add = [&](const QVariantMap &options) {
            byType[type].push_back(QVariantMap{{"type", type}, {"grantId", grant.value("id")}, {"source", grant.value("source")}, {"options", options}});
        };
        if (type == number(golarion::MetamagicType::Heighten))
            for (int level = 1; level <= std::min(9, slot.value("spellLevel").toInt()); ++level) add({{"level", level}});
        else if (type == number(golarion::MetamagicType::Reach))
            for (int steps = 1; steps <= 3; ++steps) add({{"steps", steps}});
        else if (type == number(golarion::MetamagicType::Elemental))
        {
            for (const auto energy : {golarion::DamageType::Acid, golarion::DamageType::Cold, golarion::DamageType::Electricity, golarion::DamageType::Fire})
                if (grant.value("energy").isNull() || grant.value("energy").toInt() == number(energy))
                    for (const auto mode : {golarion::ElementalDamageMode::Replace, golarion::ElementalDamageMode::Split})
                        add({{"energy", number(energy)}, {"mode", number(mode)}});
        }
        else add({});
    }

    const auto validate = [&](const QVariantList &selected) {
        if (preparing)
        {
            if (spellPreparationCount(id, slot, input.value("spellId").toString(), selected) == 0) return false;
            result.push_back(QVariantMap{{"applications", selected}});
            return true;
        }
        auto request = input;
        request.insert("metamagic", selected);
        bool valid = canConsumeSpell(command, id, slot, request);
        if (valid) result.push_back(QVariantMap{{"applications", selected}});
        if (command == ConsumeFlexible)
            for (const auto &permission : state.value("mixingPermissions").toList())
            {
                request.insert("mixingPermissionId", permission.toMap().value("id"));
                if (!canConsumeSpell(command, id, slot, request)) continue;
                valid = true;
                result.push_back(QVariantMap{{"applications", selected}, {"mixingPermissionId", request.value("mixingPermissionId")},
                    {"mixingLabel", permission.toMap().value("source")}});
            }
        return valid;
    };
    QVariantList selected;
    const auto combinations = [&](auto &&self, auto start) -> void {
        for (auto it = start; it != byType.cend(); ++it)
            for (const auto &candidate : it.value())
            {
                selected.push_back(candidate);
                if (validate(selected)) self(self, std::next(it));
                selected.pop_back();
            }
    };
    combinations(combinations, byType.cbegin());
    return result;
}
