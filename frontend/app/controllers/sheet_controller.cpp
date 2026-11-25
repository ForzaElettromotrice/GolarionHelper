#include "sheet_controller.hpp"
#include "sheet_view_changes.hpp"

#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/feat_definition_manager.hpp"
#include "golarion/character/race_definition_manager.hpp"
#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/equipment/item_definition_manager.hpp"
#include "golarion/persistence/json.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QtConcurrentRun>
#include <QUuid>
#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <nlohmann/json.hpp>
#include <utility>

namespace
{
    constexpr auto sheetLibraryKey = "sheets/localPaths";

    QString catalogReadMessage(const golarion::catalog::CatalogReadError &error)
    {
        return SheetController::tr("Impossibile leggere un catalogo ufficiale. %1").arg(QString::fromUtf8(error.what()));
    }

    bool replaceIfChanged(auto &current, auto &&next)
    {
        if (current == next) return false;
        current = std::forward<decltype(next)>(next);
        return true;
    }

    QVariant optionalText(const std::optional<std::string> &value)
    {
        return value ? QVariant{QString::fromStdString(*value)} : QVariant{};
    }

    std::optional<std::string> identityText(const QVariant &value)
    {
        if (value.isNull()) return std::nullopt;
        return value.toString().toStdString();
    }

    template<typename Int>
    std::optional<Int> identityNumber(const QVariant &value)
    {
        if (value.isNull()) return std::nullopt;
        bool ok = false;
        const auto number = value.toLongLong(&ok);
        if (!ok || number < std::numeric_limits<Int>::min() || number > std::numeric_limits<Int>::max())
            throw std::invalid_argument("invalid identity number");
        return static_cast<Int>(number);
    }

    QVariantMap identitySnapshot(const golarion::CharacterIdentityView &identity)
    {
        return {
            {"name", QString::fromStdString(identity.name)}, {"playerName", QString::fromStdString(identity.playerName)},
            {"alignment", identity.alignment ? static_cast<int>(*identity.alignment) + 1 : 0},
            {"deity", optionalText(identity.deity)}, {"homeland", optionalText(identity.homeland)}, {"gender", optionalText(identity.gender)},
            {"age", identity.age ? QVariant{*identity.age} : QVariant{}},
            {"heightCentimeters", identity.heightCentimeters ? QVariant{*identity.heightCentimeters} : QVariant{}},
            {"weightGrams", identity.weightGrams ? QVariant::fromValue(static_cast<qlonglong>(*identity.weightGrams)) : QVariant{}},
            {"hair", optionalText(identity.hair)}, {"eyes", optionalText(identity.eyes)}, {"appearance", optionalText(identity.appearance)}
        };
    }

    QVariantMap modifierSnapshot(const golarion::ModifierSetView &set)
    {
        QVariantList modifiers;
        for (const auto &entry : set.modifiers)
            modifiers.push_back(QVariantMap{{"type", static_cast<int>(entry.type)},
                {"source", QString::fromStdString(entry.source)}, {"condition", optionalText(entry.condition)},
                {"bonusType", entry.bonusType ? QVariant{static_cast<int>(*entry.bonusType)} : QVariant{}},
                {"resolvedValue", entry.resolvedValue}, {"active", entry.active}});
        QVariantList conditionalTotals;
        for (const auto &entry : set.conditionalTotals)
            conditionalTotals.push_back(QVariantMap{{"condition", QString::fromStdString(entry.condition)}, {"value", entry.value}});
        return {{"total", set.total}, {"modifiers", modifiers}, {"conditionalTotals", conditionalTotals}};
    }

    template<typename Options>
    QVariantList abilityOptionsSnapshot(const Options &options)
    {
        QVariantList result;
        for (const auto &option : options)
        {
            QVariantMap entry{{"source", QString::fromStdString(option.source)}, {"abilityType", static_cast<int>(option.abilityType)},
                {"abilityModifier", option.abilityModifier}, {"totalValue", option.totalValue}};
            if constexpr (requires { option.modifiers; })
                entry.insert("modifiers", modifierSnapshot(option.modifiers));
            result.push_back(entry);
        }
        return result;
    }

    QVariantMap contributionSnapshot(const golarion::ContributionSetView &set)
    {
        QVariantList contributions;
        for (const auto &entry : set.contributions)
            contributions.push_back(QVariantMap{{"id", QString::fromStdString(entry.id)}, {"resolvedValue", entry.resolvedValue}});
        return {{"contributions", contributions}};
    }

    QVariantMap armorClassSnapshot(const golarion::ArmorClassView &armor)
    {
        QVariantList options, suppressions;
        for (const auto &option : armor.abilityOptions)
        {
            QVariantList values;
            for (const auto &value : option.values)
                values.push_back(QVariantMap{{"type", static_cast<int>(value.type)}, {"totalValue", value.totalValue},
                    {"appliedAbilityModifier", value.appliedAbilityModifier}, {"modifiers", modifierSnapshot(value.modifiers)}});
            options.push_back(QVariantMap{{"source", QString::fromStdString(option.source)},
                {"abilityType", static_cast<int>(option.abilityType)}, {"values", values}});
        }
        for (const auto &entry : armor.abilitySuppressions)
            suppressions.push_back(QVariantMap{{"source", QString::fromStdString(entry.source)}, {"dexterityRetained", entry.dexterityRetained}});
        return {{"abilityOptions", options}, {"abilitySuppressions", suppressions},
            {"maximumDexterityBonus", armor.maximumDexterityBonus ? QVariant{*armor.maximumDexterityBonus} : QVariant{}}};
    }

    QVariantMap hitPointsSnapshot(const golarion::HitPointsView &hitPoints)
    {
        QVariantList pools;
        for (const auto &pool : hitPoints.temporary.pools)
            pools.push_back(QVariantMap{{"id", QString::fromStdString(pool.id)}, {"remaining", pool.remaining}});
        return {{"current", hitPoints.current}, {"max", hitPoints.max}, {"nonLethal", hitPoints.nonLethal},
            {"temporary", QVariantMap{{"total", hitPoints.temporary.total}, {"pools", pools}}},
            {"maxContributions", contributionSnapshot(hitPoints.maxContributions)}};
    }

    template<typename Grants>
    QVariantList applicabilitySnapshots(const Grants &grants)
    {
        QVariantList result;
        for (const auto &grant : grants)
            result.push_back(QVariantMap{{"applicability", optionalText(grant.applicability)}});
        return result;
    }

    QVariantMap defensesSnapshot(const golarion::SpecialDefensesView &defenses)
    {
        QVariantList resistances, vulnerabilities, reductions, immunities;
        for (const auto &entry : defenses.energyResistances)
            resistances.push_back(QVariantMap{{"energy", static_cast<int>(entry.energy)}, {"effectiveValue", entry.effectiveValue}});
        for (const auto &entry : defenses.energyVulnerabilities)
            vulnerabilities.push_back(QVariantMap{{"energy", static_cast<int>(entry.energy)}, {"grants", applicabilitySnapshots(entry.grants)}});
        for (const auto &entry : defenses.damageReductions)
        {
            QVariantList alternatives;
            for (const auto &alternative : entry.bypass.anyOf)
            {
                QVariantList traits;
                for (const auto trait : alternative.allOf)
                    traits.push_back(static_cast<int>(trait));
                alternatives.push_back(QVariantMap{{"allOf", traits}});
            }
            reductions.push_back(QVariantMap{{"effectiveValue", entry.effectiveValue}, {"applicability", optionalText(entry.applicability)},
                {"bypass", QVariantMap{{"anyOf", alternatives}}}});
        }
        for (const auto &entry : defenses.immunities)
            immunities.push_back(QVariantMap{{"name", QString::fromStdString(entry.name)}, {"grants", applicabilitySnapshots(entry.grants)}});
        return {{"energyResistances", resistances}, {"energyVulnerabilities", vulnerabilities},
            {"damageReductions", reductions}, {"immunities", immunities}};
    }

    QVariantMap movementSnapshot(const golarion::MovementView &movement)
    {
        QVariantList grants;
        for (const auto &grant : movement.grants)
        {
            QVariantList adjustments, boundedBonuses, runAdjustments;
            QStringList reasons;
            for (const auto &entry : grant.adjustments)
                adjustments.push_back(QVariantMap{{"source", QString::fromStdString(entry.source)},
                    {"description", QString::fromStdString(entry.description)}, {"condition", optionalText(entry.condition)}});
            for (const auto &entry : grant.boundedBonuses)
                boundedBonuses.push_back(QVariantMap{{"source", QString::fromStdString(entry.source)},
                    {"requestedUnits", entry.requestedUnits}, {"cappedUnits", entry.cappedUnits}});
            for (const auto &entry : grant.run.adjustments)
                runAdjustments.push_back(QVariantMap{{"source", QString::fromStdString(entry.source)},
                    {"description", QString::fromStdString(entry.description)}, {"applied", entry.applied}});
            for (const auto &reason : grant.run.notUsableReasons)
                reasons.push_back(QString::fromStdString(reason));
            const QVariantMap run{{"supported", grant.run.supported}, {"usable", grant.run.usable},
                {"effectiveMultiplier", grant.run.effectiveMultiplier ? QVariant{*grant.run.effectiveMultiplier} : QVariant{}},
                {"distanceUnits", grant.run.distanceUnits ? QVariant{*grant.run.distanceUnits} : QVariant{}},
                {"notUsableReasons", reasons}, {"adjustments", runAdjustments}};
            grants.push_back(QVariantMap{{"id", QString::fromStdString(grant.id)}, {"source", QString::fromStdString(grant.source)},
                {"type", static_cast<int>(grant.type)}, {"baseUnits", grant.baseUnits}, {"effectiveUnits", grant.effectiveUnits},
                {"usable", grant.usable}, {"maneuverability", grant.maneuverability ? QVariant{static_cast<int>(*grant.maneuverability)} : QVariant{}},
                {"run", run}, {"modifiers", modifierSnapshot(grant.modifiers)}, {"adjustments", adjustments}, {"boundedBonuses", boundedBonuses}});
        }
        return {{"grants", grants}};
    }

    QVariantList activeConditionsSnapshot(const golarion::ConditionManagerView &view)
    {
        QVariantList cards;
        for (const auto &condition : view.conditions)
        {
            if (condition.effectiveSeverity <= 0) continue;
            const auto stage = std::ranges::find_if(condition.stages.rbegin(), condition.stages.rend(),
                [](const auto &value) { return value.active; });
            if (stage == condition.stages.rend()) continue;
            QStringList effects;
            for (const auto &active : condition.stages)
            {
                if (!active.active) continue;
                for (const auto &effect : active.effects)
                    effects.push_back(QString::fromStdString(effect.description));
                for (const auto &derived : active.derivedConditions)
                {
                    const auto child = std::ranges::find(view.conditions, derived.conditionId, &golarion::ConditionView::id);
                    if (child == view.conditions.end()) continue;
                    const auto childStage = std::ranges::find(child->stages, derived.severity, &golarion::ConditionStageView::severity);
                    const auto &name = childStage == child->stages.end() ? child->name : childStage->name;
                    effects.push_back(SheetController::tr("Comporta anche: %1").arg(QString::fromStdString(name)));
                }
            }
            effects.removeDuplicates();
            const auto id = QString::fromStdString(condition.id);
            const auto name = QString::fromStdString(stage->name);
            if (condition.id == "bleeding")
            {
                for (const auto &entry : condition.entries)
                {
                    const auto parameter = QString::fromStdString(entry.parameter.value_or(""));
                    QStringList details{SheetController::tr("Fonte: %1").arg(QString::fromStdString(entry.source)),
                        SheetController::tr("Danno per turno: %1").arg(parameter)};
                    details.append(effects);
                    cards.push_back(QVariantMap{{"id", id + "_" + QString::fromStdString(entry.id)}, {"iconId", id},
                        {"name", SheetController::tr("%1 · %2").arg(name, parameter)}, {"effects", details}});
                }
            }
            else if (condition.id == "negativeLevels")
            {
                effects.prepend(SheetController::tr("Effetti per ogni livello negativo:"));
                cards.push_back(QVariantMap{{"id", id}, {"name", SheetController::tr("%1 · %2")
                    .arg(QString::fromStdString(condition.name)).arg(condition.effectiveSeverity)}, {"effects", effects}});
            }
            else cards.push_back(QVariantMap{{"id", id}, {"name", name}, {"effects", effects}});
        }
        return cards;
    }

    template<typename Range, typename Convert>
    QVariantList snapshotList(const Range &range, Convert convert)
    {
        QVariantList result;
        result.reserve(static_cast<qsizetype>(range.size()));
        for (const auto &entry : range) result.push_back(convert(entry));
        return result;
    }

    QVariantMap namedEntry(const auto &entry)
    {
        return {{"id", QString::fromStdString(entry.id)}, {"name", QString::fromStdString(entry.name)}};
    }

    QVariantMap raceSnapshot(const golarion::RaceView &race)
    {
        const auto choiceSnapshot = [](const auto &choice) {
            return QVariantMap{{"id", QString::fromStdString(choice.id)}, {"prompt", QString::fromStdString(choice.prompt)},
                {"selectionCount", static_cast<qulonglong>(choice.selectionCount)},
                {"options", snapshotList(choice.options, namedEntry<golarion::RacialChoiceOptionView>)}};
        };
        QVariantList alternatives;
        if (race.race)
            alternatives = snapshotList(golarion::RaceDefinitionManager::instance().alternateFeaturesView(race.race->id), [](const auto &trait) {
                auto entry = namedEntry(trait);
                entry.insert("description", QString::fromStdString(trait.description));
                entry.insert("replaces", snapshotList(trait.replaces, QString::fromStdString));
                return entry;
            });
        return {{"race", race.race ? QVariant(namedEntry(*race.race)) : QVariant{}},
            {"alternateFeatures", alternatives}, {"activeElements", snapshotList(race.activeElements, [&](const auto &element) {
                auto entry = namedEntry(element);
                entry.insert("description", QString::fromStdString(element.description));
                entry.insert("removable", element.removable);
                entry.insert("choices", snapshotList(element.choices, [&](const auto &choice) {
                    auto result = choiceSnapshot(choice);
                    result.insert("selectedOptions", snapshotList(choice.selectedOptions, namedEntry<golarion::RacialChoiceOptionView>));
                    return result;
                }));
                entry.insert("missingChoices", snapshotList(element.missingChoices, [&](const auto &choice) {
                    auto result = choiceSnapshot(choice);
                    result.insert("blocksAdvancement", choice.blocksAdvancement);
                    return result;
                }));
                return entry;
            })}};
    }

    QVariantMap describedEntry(const auto &entry)
    {
        auto result = namedEntry(entry);
        result.insert("description", QString::fromStdString(entry.description));
        return result;
    }

    int enumValue(auto value) { return static_cast<int>(value); }

    QVariantMap featSummary(const auto &feat)
    {
        auto entry = namedEntry(feat);
        entry.insert("types", snapshotList(feat.types, enumValue<golarion::FeatType>));
        return entry;
    }

    QVariantMap featDetails(const golarion::FeatDefinition &feat)
    {
        auto entry = describedEntry(feat);
        entry.insert("choices", snapshotList(feat.choices, [](const auto &choice) {
            return QVariantMap{{"id", QString::fromStdString(choice.id)}, {"prompt", QString::fromStdString(choice.prompt)},
                {"selectionCount", static_cast<qulonglong>(choice.selectionCount)}, {"options", snapshotList(choice.options, [](const auto &option) {
                    return namedEntry(option);
                })}};
        }));
        return entry;
    }

    const QVariantList &standardFeatCatalog()
    {
        static const auto catalog = snapshotList(golarion::FeatDefinitionManager::instance().catalogEntries(), featSummary<golarion::FeatDefinitionManager::CatalogEntry>);
        return catalog;
    }

    enum class EnhancementInputKind { Contribution, Bonus, Penalty };

    bool automaticCondition(std::string_view id)
    {
        return id == "dead" || id == "disabled";
    }

    QVariantMap conditionsSnapshot(const golarion::ConditionManagerView &view)
    {
        QStringList automaticIds;
        const auto conditions = snapshotList(view.conditions, [&](const auto &condition) {
            if (automaticCondition(condition.id)) automaticIds.push_back(QString::fromStdString(condition.id));
            auto result = namedEntry(condition);
            result.insert("effectiveSeverity", condition.effectiveSeverity);
            result.insert("entryParameterName", optionalText(condition.entryParameterName));
            result.insert("stages", snapshotList(condition.stages, [](const auto &stage) {
                auto data = namedEntry(stage);
                data.insert("severity", stage.severity);
                data.insert("effects", snapshotList(stage.effects, [](const auto &effect) { return QString::fromStdString(effect.description); }));
                data.insert("derivedConditions", snapshotList(stage.derivedConditions, [](const auto &derived) {
                    return QVariantMap{{"id", QString::fromStdString(derived.conditionId)}, {"severity", derived.severity}};
                }));
                return data;
            }));
            result.insert("entries", snapshotList(condition.entries, [](const auto &entry) {
                return QVariantMap{{"id", QString::fromStdString(entry.id)}, {"source", QString::fromStdString(entry.source)},
                    {"severity", entry.severity}, {"origin", enumValue(entry.origin)}, {"parameter", optionalText(entry.parameter)}};
            }));
            return result;
        });
        return {{"conditions", conditions}, {"automaticConditionIds", automaticIds}};
    }

    QVariantMap enhancementsSnapshot(const golarion::EnhancementsView &view)
    {
        return {{"groups", snapshotList(view.groups, [](const auto &group) {
            auto data = namedEntry(group);
            data.insert("description", optionalText(group.description));
            data.insert("active", group.active);
            data.insert("effectsEditable", group.effectsEditable);
            if (group.origin) data.insert("origin", QVariantMap{{"type", enumValue(group.origin->type)}});
            data.insert("enhancements", snapshotList(group.enhancements, [](const auto &effect) {
                const auto definition = std::visit([](const auto &value) {
                    using T = std::decay_t<decltype(value)>;
                    QVariantMap entry{{"id", QString::fromStdString(value.id)}};
                    if constexpr (requires { QString::fromStdString(value.description); })
                        if (!value.description.empty()) entry.insert("description", QString::fromStdString(value.description));
                    if constexpr (std::is_same_v<T, golarion::ContributionEffectDefinition> || std::is_same_v<T, golarion::ModifierEffectDefinition>)
                    {
                        entry.insert("resource", QString::fromStdString(value.resource));
                        entry.insert("expression", QString::fromStdString(value.expression));
                    }
                    if constexpr (std::is_same_v<T, golarion::ModifierEffectDefinition>)
                    {
                        entry.insert("type", enumValue(value.type));
                        entry.insert("condition", optionalText(value.condition));
                    }
                    return entry;
                }, effect.definition);
                return QVariantMap{{"definition", definition}, {"resolvedValue", effect.resolvedValue ? QVariant(*effect.resolvedValue) : QVariant{}}};
            }));
            return data;
        })}};
    }

    QVariantMap featsSnapshot(const golarion::FeatsView &view)
    {
        auto slots = snapshotList(view.slots, [](const auto &slot) {
            return QVariantMap{{"id", QString::fromStdString(slot.id)}, {"source", QString::fromStdString(slot.source)},
                {"featId", optionalText(slot.featId)}, {"available", slot.available}, {"origin", enumValue(slot.origin)},
                {"allowedType", slot.allowedType ? QVariant(enumValue(*slot.allowedType)) : QVariant{}},
                {"allowedFeatIds", snapshotList(slot.allowedFeatIds, QString::fromStdString)}};
        });
        std::stable_partition(slots.begin(), slots.end(), [](const auto &entry) {
            const auto slot = entry.toMap();
            return slot.value("available").toBool() && slot.value("featId").isNull();
        });
        const auto feats = snapshotList(view.feats, [](const auto &feat) {
            auto entry = describedEntry(feat);
            entry.insert("slotId", QString::fromStdString(feat.slotId));
            entry.insert("active", feat.active);
            entry.insert("types", snapshotList(feat.types, enumValue<golarion::FeatType>));
            entry.insert("choices", snapshotList(feat.choices, [](const auto &choice) {
                return QVariantMap{{"id", QString::fromStdString(choice.id)}, {"prompt", QString::fromStdString(choice.prompt)},
                    {"selectedOptions", snapshotList(choice.selectedOptions, namedEntry<golarion::FeatChoiceOptionView>)}};
            }));
            return entry;
        });
        return {{"feats", QVariantMap{{"slots", slots}, {"feats", feats}}}};
    }

    QVariantMap languagesSnapshot(const golarion::LanguagesView &languages)
    {
        const auto spoken = snapshotList(languages.spoken, [](const auto &language) {
            auto sources = snapshotList(language.grantedSources, QString::fromStdString);
            const bool extra = language.learnedFrom == golarion::LanguageOrigin::Extra;
            if (extra) sources.push_back(SheetController::tr("Aggiunta manuale"));
            const auto origin = language.learnedFrom == golarion::LanguageOrigin::CustomLinguistics
                ? std::optional(golarion::LanguageOrigin::Linguistics) : language.learnedFrom;
            return QVariantMap{{"id", QString::fromStdString(language.id)}, {"grantedSources", sources},
                {"learnedFrom", origin && !extra ? QVariant(enumValue(*origin)) : QVariant{}}, {"removable", origin.has_value()}};
        });
        const auto options = snapshotList(languages.options, [](const auto &option) {
            return QVariantMap{{"origin", enumValue(option.origin)}, {"languageIds", snapshotList(option.languageIds, QString::fromStdString)}};
        });
        return {{"languages", QVariantMap{{"spoken", spoken}, {"options", options},
                {"remainingIntelligence", languages.remainingIntelligence}, {"remainingLinguistics", languages.remainingLinguistics}}}};
    }

    QVariantMap remindersSnapshot(const golarion::RemindersView &view, const std::vector<golarion::PendingChoice> &pending)
    {
        return {{"reminders", QVariantMap{{"messages", snapshotList(view.messages, QString::fromStdString)},
                {"choices", snapshotList(pending, [](const auto &choice) {
                    return QVariantMap{{"type", enumValue(choice.type())}, {"description", QString::fromStdString(choice.description)},
                        {"remaining", static_cast<qulonglong>(choice.remaining)}, {"blocksAdvancement", choice.blocksAdvancement}};
                })}}}};
    }

    QVariantMap classesSnapshot(const golarion::ClassesView &classes)
    {
        const auto entries = snapshotList(classes.classes, [](const auto &entry) {
            QVariantMap results;
            for (const auto &[level, result] : entry.hitDieResults) results.insert(QString::number(level), result);
            return QVariantMap{{"type", enumValue(entry.type)}, {"name", QString::fromStdString(entry.name)},
                {"level", entry.level}, {"hitDie", enumValue(entry.hitDie)}, {"hitDieResults", results},
                {"archetypes", snapshotList(entry.archetypes, describedEntry<golarion::ArchetypeView>)},
                {"features", snapshotList(entry.features, [](const auto &feature) {
                    auto result = describedEntry(feature);
                    result.insert("acquisitions", snapshotList(feature.acquisitions, [](const auto &acquisition) {
                        return QVariantMap{{"id", QString::fromStdString(acquisition.id)}, {"level", acquisition.level}, {"active", acquisition.active},
                            {"choices", snapshotList(acquisition.choices, [](const auto &choice) {
                                return QVariantMap{{"id", QString::fromStdString(choice.id)}, {"prompt", QString::fromStdString(choice.prompt)},
                                    {"selectionCount", static_cast<qulonglong>(choice.selectionCount)},
                                    {"remainingSelections", static_cast<qulonglong>(choice.remainingSelections)},
                                    {"blocksAdvancement", choice.blocksAdvancement}, {"options", snapshotList(choice.options, [](const auto &option) {
                                        auto result = describedEntry(option);
                                        result.insert("selected", option.selected);
                                        return result;
                                    })}};
                            })}};
                    }));
                    return result;
                })}};
        });
        const auto &favored = classes.favoredClasses;
        const auto favoredEntries = snapshotList(favored.classes, [](const auto &entry) {
            return QVariantMap{{"type", enumValue(entry.type)}, {"favored", entry.favored},
                {"selections", snapshotList(entry.selections, [](const auto &selection) {
                    return QVariantMap{{"classLevel", selection.classLevel}, {"optionId", QString::fromStdString(selection.optionId)}, {"active", selection.active}};
                })}, {"options", snapshotList(entry.options, [](const auto &option) {
                    auto result = describedEntry(option);
                    result.insert("available", option.available);
                    result.insert("selections", option.selections);
                    result.insert("selectionsPerBenefit", option.selectionsPerBenefit);
                    result.insert("benefitTypes", snapshotList(option.benefitTypes, enumValue<golarion::FavoredClassBenefitType>));
                    return result;
                })}};
        });
        return {{"level", classes.level}, {"classes", entries}, {"favoredClasses", QVariantMap{
            {"selectedClasses", snapshotList(favored.selectedClasses, enumValue<golarion::CharacterClassType>)}, {"classes", favoredEntries}}}};
    }

    golarion::CharacterClassType classTypeFromInt(int type)
    {
        if (type < enumValue(golarion::CharacterClassType::Barbarian) || type > enumValue(golarion::CharacterClassType::Adept))
            throw std::invalid_argument("unknown character class");
        return static_cast<golarion::CharacterClassType>(type);
    }

    QVariant integer(auto value) { return QVariant::fromValue(static_cast<qlonglong>(value)); }
    QVariant optionalInteger(const auto &value) { return value ? integer(*value) : QVariant{}; }

    QVariantMap inventoryItemsSnapshot(const golarion::InventoryView &inventory)
    {
        const auto items = snapshotList(inventory.items, [&](const auto &item) {
            return QVariantMap{{"id", QString::fromStdString(item.id)}, {"name", QString::fromStdString(item.name)},
                {"itemDefinitionId", QString::fromStdString(item.itemDefinitionId)}, {"quantity", item.quantity},
                {"description", QString::fromStdString(item.description)}, {"containerId", QString::fromStdString(item.containerId)},
                {"ownedContainerId", optionalText(item.ownedContainerId)}, {"equipped", item.equipped}, {"canStack", item.canStack},
                {"equipmentSlot", optionalInteger(item.equipmentSlot)}, {"coinDenomination", optionalInteger(item.coinDenomination)},
                {"intrinsicWeightGrams", integer(item.intrinsicWeightGrams)}, {"effectiveWeightGrams", integer(item.effectiveWeightGrams)},
                {"intrinsicVolumeMilliliters", integer(item.intrinsicVolumeMilliliters)},
                {"usagePools", snapshotList(item.usagePools, [](const auto &pool) {
                    return QVariantMap{{"id", QString::fromStdString(pool.id)}, {"capacity", pool.capacity}, {"remaining", pool.remaining}};
                })}, {"choices", snapshotList(item.choices, [](const auto &choice) {
                    return QVariantMap{{"prompt", QString::fromStdString(choice.prompt)}, {"options", snapshotList(choice.options, [](const auto &option) {
                        return QVariantMap{{"id", QString::fromStdString(option.id)}, {"name", QString::fromStdString(option.name)}, {"selected", option.selected}};
                    })}};
                })}};
        });
        return {{"items", items}, {"totalCarriedWeightGrams", integer(inventory.totalCarriedWeightGrams)}};
    }

    QVariantMap inventoryContainersSnapshot(const golarion::InventoryView &inventory)
    {
        const auto containers = snapshotList(inventory.containers, [&](const auto &container) {
            return QVariantMap{{"id", QString::fromStdString(container.id)}, {"name", QString::fromStdString(container.name)},
                {"ownerItemId", optionalText(container.ownerItemId)}, {"itemIds", snapshotList(container.itemIds, QString::fromStdString)},
                {"maximumContentsWeightGrams", optionalInteger(container.maximumContentsWeightGrams)},
                {"maximumContentsVolumeMilliliters", optionalInteger(container.maximumContentsVolumeMilliliters)},
                {"currentContentsWeightGrams", integer(container.currentContentsWeightGrams)},
                {"currentContentsVolumeMilliliters", integer(container.currentContentsVolumeMilliliters)},
                {"ignoresContentsWeight", container.ignoresContentsWeight}, {"contributesToCarriedWeight", container.contributesToCarriedWeight}};
        });
        return {{"containers", containers}};
    }

    QVariantMap inventoryMoneySnapshot(const golarion::InventoryView &inventory)
    {
        const auto amounts = [&](const auto &value) {
            return QVariantMap{{"copper", integer(value.copper)}, {"silver", integer(value.silver)},
                {"gold", integer(value.gold)}, {"platinum", integer(value.platinum)}, {"weightGrams", integer(value.weightGrams)}};
        };
        const auto money = QVariantMap{{"total", amounts(inventory.money.total)}, {"containers", snapshotList(inventory.money.containers, [&](const auto &entry) {
            return QVariantMap{{"containerId", QString::fromStdString(entry.containerId)}, {"amounts", amounts(entry.amounts)}};
        })}};
        return {{"money", money}};
    }

    QVariantMap inventoryEquipmentSnapshot(const golarion::InventoryView &inventory)
    {
        return {{"equipmentSlots", snapshotList(inventory.equipmentSlots, [&](const auto &slot) {
                return QVariantMap{{"slot", static_cast<int>(slot.slot)}, {"capacity", integer(slot.capacity)},
                    {"itemIds", snapshotList(slot.itemIds, QString::fromStdString)}};
            })}};
    }

    QVariantMap encumbranceSnapshot(const golarion::EncumbranceView &load)
    {
        const auto &effects = load.effects;
        return {{"encumbrance", QVariantMap{{"totalWeightGrams", integer(load.totalWeightGrams)}, {"category", static_cast<int>(load.category)},
                {"lightLoadMaxGrams", integer(load.lightLoadMaxGrams)}, {"mediumLoadMaxGrams", integer(load.mediumLoadMaxGrams)},
                {"heavyLoadMaxGrams", integer(load.heavyLoadMaxGrams)}, {"effects", QVariantMap{
                    {"maximumDexterityBonus", optionalInteger(effects.maximumDexterityBonus)}, {"armorCheckPenalty", effects.armorCheckPenalty},
                    {"losesDexterityBonusToArmorClass", effects.losesDexterityBonusToArmorClass}, {"reducesMovement", effects.reducesMovement},
                    {"movementSpeedLimitUnits", optionalInteger(effects.movementSpeedLimitUnits)}, {"runMultiplierPenalty", effects.runMultiplierPenalty},
                    {"preventsRunning", effects.preventsRunning}}}}}};
    }

    QVariantMap diceSnapshot(const golarion::DamageDiceView &dice)
    {
        return {{"diceCount", dice.diceCount}, {"dieSize", dice.dieSize}, {"expression", QString::fromStdString(dice.expression)}};
    }

    QVariantMap damageComponentSnapshot(const golarion::DamageComponentView &component)
    {
        return {{"source", QString::fromStdString(component.source)}, {"effectiveDice", diceSnapshot(component.effectiveDice)},
            {"types", snapshotList(component.types, [](auto type) { return static_cast<int>(type); })},
            {"typeMode", static_cast<int>(component.typeMode)},
            {"conditionalDice", snapshotList(component.conditionalDice, [](const auto &entry) {
                return QVariantMap{{"condition", QString::fromStdString(entry.condition)}, {"dice", diceSnapshot(entry.dice)}};
            })}};
    }

    QVariantMap criticalSnapshot(const golarion::CriticalProfileView &profile)
    {
        return {{"threatMinimum", profile.threatMinimum}, {"multiplier", profile.multiplier}};
    }

    QVariantMap strikeSnapshot(const golarion::StrikeView &strike)
    {
        const auto distance = [](const auto &entry) { return QVariantMap{{"effectiveValue", entry.effectiveValue}}; };
        QVariantMap result{
            {"source", QString::fromStdString(strike.source)}, {"usageAttackPenalty", strike.usageAttackPenalty},
            {"failureReasons", snapshotList(strike.failureReasons, QString::fromStdString)},
            {"attackModifiers", modifierSnapshot(strike.attackModifiers)},
            {"criticalConfirmationModifiers", modifierSnapshot(strike.criticalConfirmationModifiers)},
            {"damageModifiers", modifierSnapshot(strike.damageModifiers)},
            {"criticalProfile", criticalSnapshot(strike.criticalProfile)},
            {"conditionalCriticalProfiles", snapshotList(strike.conditionalCriticalProfiles, [](const auto &entry) {
                return QVariantMap{{"condition", QString::fromStdString(entry.condition)}, {"profile", criticalSnapshot(entry.profile)}};
            })},
            {"damageComponents", snapshotList(strike.damageComponents, damageComponentSnapshot)},
            {"attackAbilityOptions", snapshotList(strike.attackAbilityOptions, [](const auto &option) {
                return QVariantMap{{"source", QString::fromStdString(option.source)}, {"abilityType", static_cast<int>(option.abilityType)},
                    {"abilityModifier", option.abilityModifier}, {"attackBonus", option.attackBonus}, {"criticalConfirmationBonus", option.criticalConfirmationBonus}};
            })},
            {"defenseOptions", snapshotList(strike.defenseOptions, [](const auto &entry) {
                return QVariantMap{{"defenseType", static_cast<int>(entry.defenseType)}, {"condition", optionalText(entry.condition)}};
            })},
            {"requirements", snapshotList(strike.requirements, [](const auto &entry) {
                return QVariantMap{{"requirement", QVariantMap{{"satisfied", entry.requirement.satisfied},
                    {"failureReason", QString::fromStdString(entry.requirement.failureReason)}}}};
            })}
        };
        if (strike.reach)
            result.insert("reach", QVariantMap{{"minimumUnits", distance(strike.reach->minimumUnits)}, {"maximumUnits", distance(strike.reach->maximumUnits)}});
        if (strike.range)
            result.insert("range", QVariantMap{{"incrementUnits", distance(strike.range->incrementUnits)},
                {"maximumIncrements", distance(strike.range->maximumIncrements)}, {"penaltyPerAdditionalIncrement", distance(strike.range->penaltyPerAdditionalIncrement)}});
        return result;
    }

    QVariantMap calculatedDamageSnapshot(const golarion::CalculatedDamageView &damage)
    {
        return {{"abilityOptions", snapshotList(damage.abilityOptions, [](const auto &option) {
                    return QVariantMap{{"source", QString::fromStdString(option.source)}, {"abilityType", static_cast<int>(option.abilityType)},
                        {"abilityContribution", option.abilityContribution}, {"bonus", option.bonus}};
                })},
                {"components", snapshotList(damage.components, [](const auto &entry) {
                    return QVariantMap{{"component", damageComponentSnapshot(entry.component)}, {"occurrences", entry.occurrences}};
                })}};
    }

    QVariantMap attackSnapshot(const golarion::AttackView &attack)
    {
        return {{"id", QString::fromStdString(attack.id)}, {"name", QString::fromStdString(attack.name)}, {"actionId", optionalText(attack.actionId)},
            {"complete", attack.complete}, {"failureReasons", snapshotList(attack.failureReasons, QString::fromStdString)},
            {"slots", snapshotList(attack.slots, [](const auto &slot) {
                return QVariantMap{{"id", QString::fromStdString(slot.id)}, {"name", QString::fromStdString(slot.name)},
                    {"selectionMode", static_cast<int>(slot.selectionMode)}, {"assignedGrantId", optionalText(slot.assignedGrantId)},
                    {"effectiveGrantIds", snapshotList(slot.effectiveGrantIds, QString::fromStdString)},
                    {"failureReasons", snapshotList(slot.failureReasons, QString::fromStdString)},
                    {"candidates", snapshotList(slot.candidates, [](const auto &candidate) {
                        return QVariantMap{{"grantId", QString::fromStdString(candidate.grantId)}, {"name", QString::fromStdString(candidate.name)},
                            {"accepted", candidate.accepted}};
                    })}};
            })},
            {"calculatedAttacks", snapshotList(attack.calculatedAttacks, [](const auto &entry) {
                return QVariantMap{{"strikeName", QString::fromStdString(entry.strikeName)}, {"slotName", QString::fromStdString(entry.slotName)},
                    {"routineAttackBonusAdjustment", entry.routineAttackBonusAdjustment}, {"progressionAttackBonusAdjustment", entry.progressionAttackBonusAdjustment},
                    {"strike", strikeSnapshot(entry.strike)}, {"normalDamage", calculatedDamageSnapshot(entry.normalDamage)},
                    {"criticalDamage", calculatedDamageSnapshot(entry.criticalDamage)}};
            })}};
    }

    QVariantMap attackRoutinesSnapshot(const golarion::CharacterSheetView &sheet)
    {
        const auto routines = snapshotList(sheet.attackRoutines.routines, [](const auto &routine) {
            return QVariantMap{{"id", QString::fromStdString(routine.id)}, {"name", QString::fromStdString(routine.name)},
                {"actionId", QString::fromStdString(routine.actionId)}, {"usable", routine.usable},
                {"failureReasons", snapshotList(routine.failureReasons, QString::fromStdString)}};
        });
        return {{"attackRoutines", QVariantMap{{"routines", routines}}}};
    }

    QVariantMap actionsSnapshot(const golarion::CharacterSheetView &sheet)
    {
        const auto categories = snapshotList(sheet.actions.categories, [](const auto &category) {
            return QVariantMap{{"id", QString::fromStdString(category.id)}, {"name", QString::fromStdString(category.name)},
                {"actions", snapshotList(category.actions, [](const auto &action) {
                    return QVariantMap{{"id", QString::fromStdString(action.id)}, {"name", QString::fromStdString(action.name)},
                        {"source", QString::fromStdString(action.source)}, {"description", QString::fromStdString(action.description)},
                        {"effectiveCost", static_cast<int>(action.effectiveCost)}, {"usable", action.usable},
                        {"inhibitions", snapshotList(action.inhibitions, [](const auto &entry) {
                            return QVariantMap{{"source", QString::fromStdString(entry.source)}, {"reason", QString::fromStdString(entry.reason)}};
                        })},
                        {"notes", snapshotList(action.notes, [](const auto &entry) {
                            return QVariantMap{{"source", QString::fromStdString(entry.source)}, {"text", QString::fromStdString(entry.text)}};
                        })}};
                })}};
        });
        return {{"actions", QVariantMap{{"categories", categories}}}};
    }

    QVariantMap maneuversSnapshot(const golarion::CharacterSheetView &sheet)
    {
        const auto maneuvers = snapshotList(sheet.combatManeuvers.maneuvers, [](const auto &entry) {
            const auto &defense = entry.defense;
            return QVariantMap{{"type", static_cast<int>(entry.type)},
                {"bonus", QVariantMap{{"abilityOptions", abilityOptionsSnapshot(entry.bonus.abilityOptions)},
                    {"modifiers", modifierSnapshot(entry.bonus.modifiers)}}},
                {"defense", QVariantMap{{"strengthModifier", defense.strengthModifier},
                    {"appliedDexterityModifier", defense.appliedDexterityModifier}, {"totalValue", defense.totalValue},
                    {"dexterityBonusSuppressed", defense.dexterityBonusSuppressed}, {"modifiers", modifierSnapshot(defense.modifiers)},
                    {"dexteritySuppressions", snapshotList(defense.dexteritySuppressions, [](const auto &value) {
                        return QVariantMap{{"source", QString::fromStdString(value.source)}};
                    })}}}};
        });
        return {{"combatManeuvers", QVariantMap{{"baseAttackBonus", sheet.combatManeuvers.baseAttackBonus}, {"maneuvers", maneuvers}}}};
    }

    QVariantMap abilitiesSnapshot(const golarion::CharacterSheetView &sheet)
    {
        QVariantList abilities;
        for (const auto &ability : sheet.abilities)
            abilities.push_back(QVariantMap{{"type", static_cast<int>(ability.type)}, {"baseValue", ability.baseValue},
                {"totalValue", ability.totalValue}, {"modifier", ability.modifier}, {"modifiers", modifierSnapshot(ability.modifiers)}});
        return {{"abilities", abilities}};
    }

    QVariantMap skillsSnapshot(const golarion::CharacterSheetView &sheet)
    {
        QVariantList skills;
        for (const auto &skill : sheet.skills.skills)
        {
            QVariantList rankGrants;
            for (const auto &grant : skill.rankGrants)
                rankGrants.push_back(QVariantMap{{"source", QString::fromStdString(grant.source)}, {"ranks", grant.ranks}});
            skills.push_back(QVariantMap{{"type", static_cast<int>(skill.type)}, {"name", QString::fromStdString(skill.name)},
                {"resourceName", QString::fromStdString(skill.resourceName)}, {"specializationId", optionalText(skill.specializationId)},
                {"ranks", skill.ranks}, {"effectiveRanks", skill.effectiveRanks}, {"classSkill", skill.classSkill},
                {"customClassSkill", skill.customClassSkill},
                {"classSkillBonus", skill.classSkillBonus}, {"trainedOnly", skill.trainedOnly}, {"usable", skill.usable},
                {"custom", skill.custom}, {"appliedArmorCheckPenalty", skill.appliedArmorCheckPenalty}, {"rankGrants", rankGrants},
                {"abilityOptions", abilityOptionsSnapshot(skill.abilityOptions)}});
        }
        const auto skillCategories = snapshotList(sheet.skills.categoryClassSkills, [](const auto &category) {
            return QVariantMap{{"type", static_cast<int>(category.type)}, {"classSkill", category.classSkill},
                {"customClassSkill", category.customClassSkill}};
        });
        return {{"skills", skills}, {"skillCategories", skillCategories}, {"spentRanks", sheet.skills.spentRanks}, {"maxRanks", sheet.skills.maxRanks}};
    }

    QVariantMap savesSnapshot(const golarion::CharacterSheetView &sheet)
    {
        QVariantList savingThrows;
        for (const auto &savingThrow : sheet.savingThrows.savingThrows)
            savingThrows.push_back(QVariantMap{{"type", static_cast<int>(savingThrow.type)}, {"baseValue", savingThrow.baseValue},
                {"baseContributions", contributionSnapshot(savingThrow.baseContributions)}, {"modifiers", modifierSnapshot(savingThrow.modifiers)},
                {"abilityOptions", abilityOptionsSnapshot(savingThrow.abilityOptions)}});
        return {{"savingThrows", savingThrows}};
    }
}

struct SheetController::PublishedInputs
{
    golarion::CharacterSheetView view;
    std::vector<golarion::PendingChoice> pending;
    std::vector<std::string> modifiers, contributions, targets;
};

SheetController::SheetController(QObject *parent, QString sheetDirectory) : QObject(parent), sheetDirectory_(std::move(sheetDirectory))
{
    connect(&saveWatcher_, &QFutureWatcherBase::finished, this, &SheetController::finishSave);
    connect(&catalogWatcher_, &QFutureWatcherBase::finished, this, &SheetController::finishCatalogLoad);
    connect(&browsingCatalogWatcher_, &QFutureWatcherBase::finished, this, &SheetController::finishBrowsingCatalogLoad);
}

SheetController::~SheetController()
{
    if (!flushSaves()) qWarning() << "Could not finish saving the sheet:" << currentPath_;
    catalogWatcher_.waitForFinished();
    browsingCatalogWatcher_.waitForFinished();
}

QVariantMap SheetController::identity() const { return identity_; }
QVariantMap SheetController::home() const
{
    auto result = abilityScores_;
    result.insert(skillScores_);
    result.insert(savingThrows_);
    result.insert(armorClass_);
    result.insert(hitPoints_);
    result.insert(defenses_);
    result.insert(movement_);
    result.insert(activeConditions_);
    result.insert(initiative_);
    return result;
}
QVariantMap SheetController::combat() const
{
    auto result = attacks_;
    result.insert(attackRoutines_);
    result.insert(actions_);
    result.insert(baseAttackBonus_);
    result.insert(combatManeuvers_);
    return result;
}
QVariantMap SheetController::talents() const
{
    auto result = feats_;
    result.insert(languages_);
    result.insert(reminders_);
    return result;
}
QVariantMap SheetController::inventory() const
{
    auto result = inventoryItems_;
    result.insert(inventoryContainers_);
    result.insert(inventoryEquipment_);
    result.insert(inventoryMoney_);
    result.insert(encumbrance_);
    return result;
}
QVariantMap SheetController::spells() const
{
    auto result = spellcastings_;
    result.insert(spellCatalogData_);
    result.insert(grantedSpells_);
    result.insert(lastCast_);
    return result;
}
QString SheetController::notes() const { return notes_; }
QString SheetController::errorMessage() const { return errorMessage_; }
bool SheetController::opening() const { return opening_; }

void SheetController::preloadCatalogs()
{
    if (!classCatalog_ && !catalogLoading_)
    {
        catalogLoading_ = true;
        catalogWatcher_.setFuture(QtConcurrent::run([]() -> ClassCatalogResult
        {
            try { return {&golarion::ClassDefinitionManager::instance(), {}}; }
            catch (const golarion::catalog::CatalogReadError &error) { return {nullptr, catalogReadMessage(error)}; }
            catch (...) { return {nullptr, tr("Impossibile caricare il catalogo delle classi. Riprova.")}; }
        }));
    }
    if (!browsingCatalogReady_ && !browsingCatalogLoading_)
    {
        browsingCatalogLoading_ = true;
        browsingCatalogWatcher_.setFuture(QtConcurrent::run([]() -> QString {
            try
            {
                prepareStandardSpellCatalog();
                (void)standardFeatCatalog();
                return {};
            }
            catch (const golarion::catalog::CatalogReadError &error) { return catalogReadMessage(error); }
            catch (...) { return tr("Impossibile caricare i cataloghi di talenti e incantesimi. Riprova."); }
        }));
    }
}

void SheetController::openLocalSheet()
{
    if (sheet_)
    {
        Q_EMIT sheetOpened();
        return;
    }
    requestSheet({}, false);
}

QVariantList SheetController::savedSheets() const
{
    QVariantList rows;
    for (const auto &path : QSettings().value(sheetLibraryKey).toStringList())
    {
        const QFileInfo file(path);
        rows.push_back(QVariantMap{{"name", file.completeBaseName()}, {"path", path},
            {"url", QUrl::fromLocalFile(path)}, {"available", file.isFile()}});
    }
    return rows;
}

static QString unusedSheetPath(const QString &folder, const QString &name)
{
    if (folder.isEmpty() || !QDir().mkpath(folder)) return {};
    const QDir directory(folder);
    auto path = directory.filePath(name + ".json");
    for (int index = 2; QFileInfo::exists(path); ++index)
        path = directory.filePath(QStringLiteral("%1 %2.json").arg(name).arg(index));
    return path;
}

void SheetController::openSavedSheet(const QUrl &file)
{
    if (opening_ || !file.isLocalFile()) return;
    const QFileInfo source(file.toLocalFile());
    if (source.isSymLink() || source.dir() != QDir(sheetDirectory_))
        return;
    importSource_.clear();
    requestSheet(source.absoluteFilePath(), false);
}

void SheetController::createSavedSheet(const QString &name)
{
    if (opening_) return;
    const auto base = name.trimmed();
    const auto path = base.isEmpty() || base.contains('/') || base.contains('\\') || base.contains(':')
        ? QString{} : unusedSheetPath(sheetDirectory_, base);
    if (path.isEmpty())
    {
        setErrorMessage(tr("Impossibile creare la scheda. Verifica il nome e la cartella locale."));
        return;
    }
    importSource_.clear();
    requestSheet(path, true);
}

void SheetController::importSavedSheet(const QUrl &file)
{
    if (opening_ || !file.isLocalFile()) return;
    const QFileInfo source(file.toLocalFile());
    const auto path = unusedSheetPath(sheetDirectory_, source.completeBaseName().isEmpty() ? tr("Scheda") : source.completeBaseName());
    if (path.isEmpty())
    {
        setErrorMessage(tr("La cartella locale delle schede non è disponibile."));
        return;
    }
    importSource_ = source.absoluteFilePath();
    requestSheet(path, false);
}

void SheetController::exportSavedSheet(const QUrl &file, const QUrl &destination)
{
    if (QFileInfo(file.toLocalFile()) == QFileInfo(currentPath_) && !flushSaves()) return;
    QFile source(file.toLocalFile());
    const QFileInfo target(destination.toLocalFile());
    QSaveFile copy(target.absoluteFilePath());
    if (file.isLocalFile() && destination.isLocalFile() && target.dir().canonicalPath() != QDir(sheetDirectory_).canonicalPath()
        && !target.isSymLink() && (!target.exists() || target != QFileInfo(source))
        && source.open(QIODevice::ReadOnly) && copy.open(QIODevice::WriteOnly))
    {
        const auto data = source.readAll();
        if (source.error() == QFileDevice::NoError && copy.write(data) == data.size() && copy.commit())
        {
            setErrorMessage({});
            return;
        }
    }
    setErrorMessage(tr("Impossibile esportare la scheda. Scegli un file esterno alla raccolta locale."));
}

void SheetController::deleteSavedSheet(const QUrl &file)
{
    if (opening_ || !file.isLocalFile()) return;
    QSettings settings;
    auto paths = settings.value(sheetLibraryKey).toStringList();
    const auto path = file.toLocalFile();
    const QFileInfo target(path);
    if (sheetDirectory_.isEmpty() || !paths.contains(path) || target.dir() != QDir(sheetDirectory_)
        || target.isSymLink() || target.suffix().compare("json", Qt::CaseInsensitive) != 0)
    {
        setErrorMessage(tr("Puoi eliminare solo una scheda della raccolta locale."));
        return;
    }
    const bool current = path == currentPath_;
    if (current && !flushSaves()) return;
    if (QFileInfo::exists(path) && (!target.isFile() || !QFile::remove(path)))
    {
        setErrorMessage(tr("Impossibile eliminare il file della scheda."));
        return;
    }
    if (current) currentPath_.clear();
    paths.removeAll(path);
    settings.setValue(sheetLibraryKey, paths);
    refreshSavedSheets();
}

void SheetController::refreshSavedSheets()
{
    setErrorMessage({});
    Q_EMIT savedSheetsChanged();
}

void SheetController::requestSheet(const QString &path, bool create)
{
    if (opening_ || !flushSaves()) return;
    pendingPath_ = path;
    createPendingFile_ = create;
    setErrorMessage({});
    if (classCatalog_ && browsingCatalogReady_)
    {
        createLocalSheet();
        return;
    }
    opening_ = true;
    Q_EMIT openingChanged();
    preloadCatalogs();
}

void SheetController::finishCatalogLoad()
{
    auto [catalog, error] = catalogWatcher_.result();
    classCatalog_ = catalog;
    classCatalogError_ = std::move(error);
    catalogLoading_ = false;
    finishOpeningWhenCatalogsReady();
}

void SheetController::finishBrowsingCatalogLoad()
{
    browsingCatalogError_ = browsingCatalogWatcher_.result();
    browsingCatalogReady_ = browsingCatalogError_.isEmpty();
    browsingCatalogLoading_ = false;
    finishOpeningWhenCatalogsReady();
}

void SheetController::finishOpeningWhenCatalogsReady()
{
    if (!opening_ || catalogLoading_ || browsingCatalogLoading_) return;
    opening_ = false;
    Q_EMIT openingChanged();
    if (classCatalog_ && browsingCatalogReady_) createLocalSheet();
    else setErrorMessage(classCatalog_ ? browsingCatalogError_ : classCatalogError_);
}

void SheetController::createLocalSheet()
{
    const auto importSource = std::exchange(importSource_, QString{});
    try
    {
        QByteArray importedData;
        std::unique_ptr<golarion::CharacterSheet> candidate;
        if (pendingPath_.isEmpty() || createPendingFile_)
            candidate = std::make_unique<golarion::CharacterSheet>(*classCatalog_);
        else
        {
            QFile file(importSource.isEmpty() ? pendingPath_ : importSource);
            if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot open sheet file");
            const auto data = file.readAll();
            if (file.error() != QFileDevice::NoError) throw std::runtime_error("Cannot read sheet file");
            if (!importSource.isEmpty()) importedData = data;
            candidate = std::make_unique<golarion::CharacterSheet>(
                golarion::persistence::fromJson(std::string_view(data.constData(), data.size())), *classCatalog_);
        }
        if (createPendingFile_ || !importSource.isEmpty())
        {
            const auto data = createPendingFile_ ? QByteArray::fromStdString(golarion::persistence::toJson(candidate->toSaveData())) : importedData;
            QFile file(pendingPath_);
            if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly))
                throw std::runtime_error("Cannot create sheet file");
            if (file.write(data.data(), data.size()) != qint64(data.size()) || !file.flush())
            {
                file.remove();
                throw std::runtime_error("Cannot write sheet file");
            }
        }
        sheet_ = std::move(candidate);
        currentPath_ = pendingPath_;
        lastSpellCast_.clear();
        publish(false);
        if (!currentPath_.isEmpty())
        {
            QSettings settings;
            auto paths = settings.value(sheetLibraryKey).toStringList();
            if (!paths.contains(currentPath_))
            {
                paths.append(currentPath_);
                settings.setValue(sheetLibraryKey, paths);
            }
            Q_EMIT savedSheetsChanged();
        }
        Q_EMIT sheetOpened();
    }
    catch (const golarion::catalog::CatalogReadError &error)
    {
        setErrorMessage(catalogReadMessage(error));
    }
    catch (const std::exception &)
    {
        setErrorMessage(createPendingFile_ || pendingPath_.isEmpty()
            ? tr("Impossibile creare la scheda. Scegli un percorso libero e scrivibile.")
            : tr("Impossibile aprire la scheda. Verifica che il file esista e sia valido."));
    }
}

void SheetController::setErrorMessage(QString message)
{
    if (replaceIfChanged(errorMessage_, std::move(message))) Q_EMIT errorMessageChanged();
}

void SheetController::queueCurrentSheetSave()
{
    if (currentPath_.isEmpty()) return;
    pendingSave_ = std::make_shared<const golarion::CharacterSheetSaveData>(sheet_->toSaveData());
    startPendingSave();
}

void SheetController::startPendingSave()
{
    if (activeSave_ || !pendingSave_) return;
    activeSave_ = std::move(pendingSave_);
    saveWatcher_.setFuture(QtConcurrent::run([snapshot = activeSave_, path = currentPath_] {
        try {
            const auto data = golarion::persistence::toJson(*snapshot);
            QSaveFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data.data(), data.size()) == qint64(data.size()) && file.commit();
        }
        catch (const std::exception &) { return false; }
    }));
}

bool SheetController::finishSave()
{
    if (!activeSave_ || !saveWatcher_.isFinished()) return true;
    if (!saveWatcher_.result() && !pendingSave_)
    {
        pendingSave_ = std::move(activeSave_);
        setErrorMessage(tr("Impossibile salvare le modifiche della scheda."));
        return false;
    }
    activeSave_.reset();
    startPendingSave();
    return true;
}

bool SheetController::flushSaves()
{
    startPendingSave();
    while (activeSave_)
    {
        saveWatcher_.waitForFinished();
        if (!finishSave()) return false;
    }
    return true;
}

void SheetController::setNotes(const QString &notes)
{
    if (!sheet_ || notes == notes_) return;
    sheet_->setNotes(notes.toStdString());
    publish();
}

template<typename Change>
void SheetController::applySheetChange(Change change, const QString &failureMessage)
{
    if (!sheet_) return;
    try
    {
        change();
        publish();
    }
    catch (const std::exception &)
    {
        setErrorMessage(failureMessage);
    }
}

void SheetController::addTemporaryHitPoints(const QString &source, int amount)
{
    if (source.trimmed().isEmpty() || amount <= 0) return;
    applySheetChange([&] { sheet_->addTemporaryHitPoints(source.trimmed().toStdString(), amount); },
        tr("Impossibile aggiungere i PF temporanei."));
}

void SheetController::removeTemporaryHitPoints(const QString &source)
{
    applySheetChange([&] { sheet_->removeTemporaryHitPoints(source.toStdString()); },
        tr("Impossibile rimuovere la riserva di PF temporanei."));
}

void SheetController::damage(int amount, bool nonLethal)
{
    if (amount <= 0) return;
    applySheetChange([&] { sheet_->damage(amount, nonLethal ? golarion::DamageLethality::NonLethal : golarion::DamageLethality::Lethal); },
        tr("Impossibile applicare il danno."));
}

void SheetController::heal(int amount)
{
    if (amount <= 0) return;
    applySheetChange([&] { sheet_->heal(amount); }, tr("Impossibile applicare la cura."));
}

void SheetController::setAbilityBaseValue(int abilityType, int value)
{
    if (abilityType < 0 || abilityType > static_cast<int>(golarion::AbilityType::Charisma)) return;
    applySheetChange([&] { sheet_->setAbilityBaseValue(static_cast<golarion::AbilityType>(abilityType), value); },
        tr("Impossibile modificare il punteggio base."));
}

void SheetController::setSkillRanks(const QString &resourceName, int ranks)
{
    applySheetChange([&] {
        const auto skills = skillScores_.value(QStringLiteral("skills")).toList();
        for (const auto &entry : skills)
        {
            const auto skill = entry.toMap();
            if (skill.value(QStringLiteral("resourceName")).toString() != resourceName) continue;
            const auto type = static_cast<golarion::SkillType>(skill.value(QStringLiteral("type")).toInt());
            const auto specializationId = identityText(skill.value(QStringLiteral("specializationId")));
            if (specializationId) sheet_->setSkillSpecializationRanks(type, *specializationId, ranks);
            else sheet_->setSkillRanks(type, ranks);
            return;
        }
        throw std::invalid_argument("unknown skill resource");
    }, tr("Impossibile modificare i gradi dell’abilità."));
}

void SheetController::addSkillSpecialization(int skillType, const QString &name)
{
    if (skillType < 0 || skillType > static_cast<int>(golarion::SkillType::Fly) || name.trimmed().isEmpty()) return;
    applySheetChange([&] {
        sheet_->addSkillSpecialization(static_cast<golarion::SkillType>(skillType),
            {"custom_" + QUuid::createUuid().toString(QUuid::Id128).toStdString(), name.trimmed().toStdString()});
    }, tr("Impossibile aggiungere la specializzazione."));
}

void SheetController::removeSkillSpecialization(int skillType, const QString &id)
{
    if (skillType < 0 || skillType > static_cast<int>(golarion::SkillType::Fly)) return;
    applySheetChange([&] { sheet_->removeSkillSpecialization(static_cast<golarion::SkillType>(skillType), id.toStdString()); },
        tr("Impossibile rimuovere la specializzazione."));
}

void SheetController::setCustomClassSkill(int skillType, bool enabled)
{
    if (skillType < 0 || skillType > static_cast<int>(golarion::SkillType::Fly)) return;
    applySheetChange([&] {
        const auto type = static_cast<golarion::SkillType>(skillType);
        if (enabled) sheet_->addCustomClassSkill(type);
        else sheet_->removeCustomClassSkill(type);
    }, tr("Impossibile modificare l’abilità di classe."));
}

void SheetController::setIdentityField(const QString &field, const QVariant &value)
{
    if (!sheet_) return;
    try
    {
        if (field == QStringLiteral("name")) sheet_->setName(value.toString().toStdString());
        else if (field == QStringLiteral("playerName")) sheet_->setPlayerName(value.toString().toStdString());
        else if (field == QStringLiteral("homeland")) sheet_->setHomeland(identityText(value));
        else if (field == QStringLiteral("gender")) sheet_->setGender(identityText(value));
        else if (field == QStringLiteral("hair")) sheet_->setHair(identityText(value));
        else if (field == QStringLiteral("eyes")) sheet_->setEyes(identityText(value));
        else if (field == QStringLiteral("appearance")) sheet_->setAppearance(identityText(value));
        else if (field == QStringLiteral("age")) sheet_->setAge(identityNumber<int>(value));
        else if (field == QStringLiteral("heightCentimeters")) sheet_->setHeightCentimeters(identityNumber<int>(value));
        else if (field == QStringLiteral("weightGrams")) sheet_->setWeightGrams(identityNumber<std::int64_t>(value));
        else if (field == QStringLiteral("alignment"))
        {
            const auto alignment = identityNumber<int>(value).value_or(0);
            if (alignment < 0 || alignment > static_cast<int>(golarion::Alignment::ChaoticEvil) + 1)
                throw std::invalid_argument("invalid identity alignment");
            sheet_->setAlignment(alignment ? std::optional{static_cast<golarion::Alignment>(alignment - 1)} : std::nullopt);
        }
        else throw std::invalid_argument("unknown identity field");
        publish();
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile aggiornare il campo dell’identità."));
    }
}

void SheetController::addAttackRoutine(const QString &routineId)
{
    applySheetChange([&] {
        const auto available = attackRoutines_.value(QStringLiteral("attackRoutines")).toMap().value(QStringLiteral("routines")).toList();
        for (const auto &entry : available)
        {
            const auto routine = entry.toMap();
            if (routine.value(QStringLiteral("id")).toString() != routineId) continue;
            if (!routine.value(QStringLiteral("usable")).toBool()) break;
            sheet_->createAttack({"attack_" + QUuid::createUuid().toString(QUuid::Id128).toStdString(),
                routine.value(QStringLiteral("name")).toString().toStdString(), routineId.toStdString()});
            return;
        }
        throw std::invalid_argument("attack routine is unavailable");
    }, tr("Impossibile aggiungere la routine."));
}

void SheetController::removeAttackRoutine(const QString &attackId)
{
    applySheetChange([&] { sheet_->removeAttack(attackId.toStdString()); }, tr("Impossibile rimuovere la routine."));
}

void SheetController::renameAttackRoutine(const QString &attackId, const QString &name)
{
    applySheetChange([&] { sheet_->renameAttack(attackId.toStdString(), name.trimmed().toStdString()); },
        tr("Impossibile rinominare la routine."));
}

void SheetController::reorderAttackRoutine(const QString &attackId, const QVariant &beforeAttackId)
{
    applySheetChange([&] {
        const auto before = identityText(beforeAttackId);
        sheet_->reorderAttack(attackId.toStdString(), before ? std::optional<std::string_view>{*before} : std::nullopt);
    }, tr("Impossibile riordinare la routine."));
}

void SheetController::assignAttackStrike(const QString &attackId, const QString &slotId, const QString &strikeGrantId)
{
    applySheetChange([&] {
        sheet_->assignStrikeToAttack(attackId.toStdString(), slotId.toStdString(), strikeGrantId.toStdString());
    }, tr("Impossibile assegnare l’attacco allo slot."));
}

namespace
{
    std::string inventoryId(const char *prefix)
    {
        return prefix + QUuid::createUuid().toString(QUuid::Id128).toStdString();
    }

    const golarion::InventoryItemView &findInventoryItem(const golarion::InventoryView &inventory, const QString &id)
    {
        const auto found = std::ranges::find(inventory.items, id.toStdString(), &golarion::InventoryItemView::id);
        if (found == inventory.items.end()) throw std::invalid_argument("unknown inventory item");
        return *found;
    }

    golarion::CoinDenomination denominationFromInt(int value)
    {
        if (value < 0 || value > static_cast<int>(golarion::CoinDenomination::Platinum))
            throw std::invalid_argument("unknown coin denomination");
        return static_cast<golarion::CoinDenomination>(value);
    }
}

template<typename Change>
void SheetController::applySheetTransaction(Change change, const QString &failureMessage)
{
    applySheetChange([&] {
        auto candidate = std::make_unique<golarion::CharacterSheet>(sheet_->toSaveData(), *classCatalog_);
        change(*candidate);
        sheet_ = std::move(candidate);
    }, failureMessage);
}

QVariantList SheetController::itemCatalog()
{
    try
    {
        static const QVariantList catalog = [] {
            auto rows = snapshotList(golarion::ItemDefinitionManager::instance().catalogEntries(), [](const auto &entry) {
                return QVariantMap{{"id", QString::fromStdString(entry.first)}, {"name", QString::fromStdString(entry.second)}};
            });
            std::stable_sort(rows.begin(), rows.end(), [](const QVariant &a, const QVariant &b) {
                return QString::localeAwareCompare(a.toMap().value("name").toString(), b.toMap().value("name").toString()) < 0;
            });
            return rows;
        }();
        return catalog;
    }
    catch (const golarion::catalog::CatalogReadError &error)
    {
        setErrorMessage(catalogReadMessage(error));
        return {};
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile caricare il catalogo degli oggetti."));
        return {};
    }
}

void SheetController::addCatalogItem(const QString &definitionId)
{
    applySheetChange([&] {
        const auto &definition = golarion::ItemDefinitionManager::instance().get(definitionId.toStdString());
        sheet_->addItem({.id = inventoryId("item."), .itemDefinitionId = definition.id});
    }, tr("Impossibile aggiungere l’oggetto dal catalogo."));
}

void SheetController::addCustomItem(const QVariantMap &definition)
{
    applySheetTransaction([&](auto &candidate) {
        auto fields = definition;
        const auto slot = fields.take("slot");
        if (slot.isValid() && !slot.isNull())
        {
            constexpr std::array slots{"ring", "armor", "belt", "neck", "body", "headband", "hands",
                "eyes", "feet", "wrists", "shield", "shoulders", "head", "chest"};
            bool valid = false;
            const int index = slot.toInt(&valid);
            if (!valid || index < 0 || static_cast<std::size_t>(index) >= slots.size())
                throw std::invalid_argument("unknown equipment slot");
            fields.insert("slot", QString::fromLatin1(slots[index]));
        }
        const auto id = inventoryId("custom.item.");
        fields.insert("id", QString::fromStdString(id));
        if (!fields.contains("tags")) fields.insert("tags", QVariantList{});
        const auto json = QJsonDocument::fromVariant(fields).toJson(QJsonDocument::Compact);
        candidate.addCustomItemDefinition(golarion::itemDefinitionFromJson(nlohmann::json::parse(json.constBegin(), json.constEnd())));
        candidate.addItem({.id = inventoryId("item."), .itemDefinitionId = id});
    }, tr("Impossibile creare l’oggetto. Controlla i dati inseriti."));
}

void SheetController::addContainer(const QVariantMap &definition)
{
    applySheetChange([&] {
        sheet_->addContainer({.id = inventoryId("container."), .name = definition.value("name").toString().trimmed().toStdString(),
            .allowsPossessionEffects = definition.value("allowsPossessionEffects", true).toBool(),
            .contributesToCarriedWeight = definition.value("contributesToCarriedWeight").toBool()});
    }, tr("Impossibile aggiungere il contenitore."));
}

void SheetController::moveItem(const QString &itemId, const QString &containerId)
{
    applySheetChange([&] { sheet_->moveItem(itemId.toStdString(), containerId.toStdString()); }, tr("Impossibile spostare l’oggetto."));
}

void SheetController::setItemQuantity(const QString &itemId, int quantity)
{
    applySheetChange([&] { sheet_->setItemQuantity(itemId.toStdString(), quantity); }, tr("Impossibile modificare la quantità."));
}

void SheetController::mergeCoinStacks(const QString &sourceId, const QString &targetId)
{
    applySheetTransaction([&](auto &candidate) {
        const auto inventory = sheet_->toView().inventory;
        const auto &source = findInventoryItem(inventory, sourceId);
        const auto &target = findInventoryItem(inventory, targetId);
        if (source.id == target.id || !source.coinDenomination || source.itemDefinitionId != target.itemDefinitionId
            || source.containerId != target.containerId || source.quantity > std::numeric_limits<int>::max() - target.quantity)
            throw std::invalid_argument("incompatible coin stacks");
        candidate.removeItem(source.id);
        candidate.setItemQuantity(target.id, target.quantity + source.quantity);
    }, tr("Impossibile unire le pile di monete."));
}

void SheetController::splitCoinStack(const QString &itemId, int quantity)
{
    applySheetTransaction([&](auto &candidate) {
        const auto inventory = sheet_->toView().inventory;
        const auto &item = findInventoryItem(inventory, itemId);
        if (!item.coinDenomination || quantity <= 0 || quantity >= item.quantity)
            throw std::invalid_argument("invalid coin split");
        const auto id = inventoryId("coins.");
        candidate.setItemQuantity(item.id, item.quantity - quantity);
        candidate.addItem({.id = id, .itemDefinitionId = item.itemDefinitionId, .quantity = quantity}, item.containerId);
        const auto container = std::ranges::find(inventory.containers, item.containerId, &golarion::InventoryContainerView::id);
        if (container != inventory.containers.end())
        {
            const auto position = std::ranges::find(container->itemIds, item.id);
            if (position != container->itemIds.end() && position + 1 != container->itemIds.end())
                candidate.reorderItem(id, *(position + 1));
        }
    }, tr("Impossibile dividere la pila di monete."));
}

void SheetController::restoreItemUsagePool(const QString &itemId, const QString &poolId, int amount)
{
    applySheetChange([&] { sheet_->restoreItemUsagePool(itemId.toStdString(), poolId.toStdString(), amount); },
        tr("Impossibile ripristinare le cariche."));
}

void SheetController::refillItemUsagePool(const QString &itemId, const QString &poolId)
{
    applySheetChange([&] { sheet_->refillItemUsagePool(itemId.toStdString(), poolId.toStdString()); },
        tr("Impossibile ricaricare l’oggetto."));
}

void SheetController::reorderItem(const QString &itemId, const QVariant &beforeId)
{
    applySheetChange([&] {
        const auto before = identityText(beforeId);
        sheet_->reorderItem(itemId.toStdString(), before ? std::optional<std::string_view>{*before} : std::nullopt);
    }, tr("Impossibile riordinare l’oggetto."));
}

void SheetController::moveAndReorderItem(const QString &itemId, const QString &containerId, const QVariant &beforeId)
{
    applySheetTransaction([&](auto &candidate) {
        const auto before = identityText(beforeId);
        candidate.moveItem(itemId.toStdString(), containerId.toStdString());
        candidate.reorderItem(itemId.toStdString(), before ? std::optional<std::string_view>{*before} : std::nullopt);
    }, tr("Impossibile spostare e riordinare l’oggetto."));
}

void SheetController::unequipItem(const QString &itemId, const QString &containerId)
{
    applySheetChange([&] { sheet_->unequipItem(itemId.toStdString(), containerId.toStdString()); },
        tr("Impossibile togliere l’oggetto dall’equipaggiamento."));
}

void SheetController::equipItem(const QString &itemId, const QString &replacedId)
{
    const auto failure = tr("Impossibile equipaggiare l’oggetto.");
    if (replacedId.isEmpty())
    {
        applySheetChange([&] { sheet_->equipItem(itemId.toStdString()); }, failure);
        return;
    }
    applySheetTransaction([&](auto &candidate) {
        const auto inventory = sheet_->toView().inventory;
        const auto &item = findInventoryItem(inventory, itemId);
        const auto &replaced = findInventoryItem(inventory, replacedId);
        if (item.equipped || !replaced.equipped || !item.equipmentSlot || item.equipmentSlot != replaced.equipmentSlot)
            throw std::invalid_argument("incompatible equipment swap");
        candidate.unequipItem(replaced.id);
        candidate.equipItem(item.id);
        if (item.containerId != replaced.containerId)
        {
            try { candidate.moveItem(replaced.id, item.containerId); }
            catch (const std::invalid_argument &) {} // Keep the old item in the main container.
        }
    }, failure);
}

void SheetController::changeMoney(int denomination, int quantity, const QString &containerId)
{
    if (quantity == 0) return;
    applySheetChange([&] {
        const auto coin = denominationFromInt(denomination);
        if (quantity == std::numeric_limits<int>::min()) throw std::invalid_argument("coin quantity out of range");
        if (quantity > 0) sheet_->addMoney(coin, quantity, containerId.toStdString());
        else sheet_->removeMoney(coin, -quantity, containerId.toStdString());
    }, tr("Impossibile modificare la quantità di monete."));
}

void SheetController::convertMoney(int from, int to, int fromQuantity, int toQuantity, const QString &sourceId, const QString &destinationId)
{
    applySheetTransaction([&](auto &candidate) {
        const auto sourceCoin = denominationFromInt(from);
        const auto destinationCoin = denominationFromInt(to);
        if (!((to == from + 1 && fromQuantity == 10 && toQuantity == 1)
            || (to == from - 1 && fromQuantity == 1 && toQuantity == 10)))
            throw std::invalid_argument("invalid coin exchange rate");
        candidate.removeMoney(sourceCoin, fromQuantity, sourceId.toStdString());
        candidate.addMoney(destinationCoin, toQuantity, destinationId.toStdString());
    }, tr("Impossibile convertire le monete."));
}

QVariantList SheetController::raceCatalog()
{
    try
    {
        return snapshotList(golarion::RaceDefinitionManager::instance().listRaces(), namedEntry<golarion::RaceSummaryView>);
    }
    catch (const golarion::catalog::CatalogReadError &error)
    {
        setErrorMessage(catalogReadMessage(error));
        return {};
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile caricare il catalogo delle razze."));
        return {};
    }
}

void SheetController::setRace(const QString &raceId)
{
    applySheetChange([&] { sheet_->setRace(raceId.toStdString()); }, tr("Impossibile cambiare razza."));
}

void SheetController::selectAlternateRacialTrait(const QString &traitId)
{
    applySheetChange([&] { sheet_->selectAlternateRacialFeature(traitId.toStdString()); },
        tr("Impossibile aggiungere il tratto alternativo. Controlla che non sia in conflitto con quelli già selezionati."));
}

void SheetController::removeAlternateRacialTrait(const QString &traitId)
{
    applySheetChange([&] { sheet_->removeAlternateRacialFeature(traitId.toStdString()); },
        tr("Impossibile rimuovere il tratto alternativo."));
}

void SheetController::setRacialChoice(const QString &elementId, const QString &choiceId, const QStringList &optionIds)
{
    applySheetChange([&] {
        std::vector<std::string> options;
        options.reserve(static_cast<std::size_t>(optionIds.size()));
        for (const auto &option : optionIds) options.push_back(option.toStdString());
        sheet_->setRacialChoice(elementId.toStdString(), choiceId.toStdString(), std::move(options));
    }, tr("Impossibile applicare la scelta razziale."));
}

QVariantList SheetController::classCatalog() const
{
    if (!classCatalog_) return {};
    return snapshotList(classCatalog_->listClasses(), [](const auto &entry) {
        return QVariantMap{{"type", enumValue(entry.type)}, {"name", QString::fromStdString(entry.name)}};
    });
}

QVariantList SheetController::archetypeCatalog(int classType)
{
    if (!classCatalog_) return {};
    try { return snapshotList(classCatalog_->listArchetypes(classTypeFromInt(classType)), namedEntry<golarion::ArchetypeSummary>); }
    catch (const std::exception &) { setErrorMessage(tr("Impossibile leggere gli archetipi della classe.")); return {}; }
}

QString SheetController::archetypeDescription(const QString &id)
{
    if (!classCatalog_) return {};
    try { return QString::fromStdString(classCatalog_->getArchetype(id.toStdString()).description); }
    catch (const std::exception &) { setErrorMessage(tr("Impossibile leggere la descrizione dell’archetipo.")); return {}; }
}

void SheetController::addClassLevel(int type)
{
    if (!sheet_) return;
    QStringList blockers;
    for (const auto &choice : sheet_->pendingChoices())
        if (choice.blocksAdvancement) blockers.push_back(QString::fromStdString(choice.description));
    if (!blockers.isEmpty())
    {
        blockers.removeDuplicates();
        setErrorMessage(tr("Prima di avanzare, completa queste scelte: %1.").arg(blockers.join(QStringLiteral("; "))));
        return;
    }
    applySheetChange([&] { sheet_->addClassLevel(classTypeFromInt(type)); }, tr("Impossibile aggiungere un livello."));
}

void SheetController::removeClassLevel(int type)
{
    applySheetChange([&] { sheet_->removeClassLevel(classTypeFromInt(type)); }, tr("Impossibile rimuovere il livello."));
}

void SheetController::setFavoredClass(int type, bool favored)
{
    applySheetChange([&] {
        const auto classType = classTypeFromInt(type);
        auto selected = sheet_->toView().classes.favoredClasses.selectedClasses;
        if (favored) selected.insert(classType);
        else selected.erase(classType);
        sheet_->setFavoredClasses(std::move(selected));
    }, tr("Impossibile modificare la classe preferita."));
}

void SheetController::setClassHitDieResult(int type, int classLevel, int result)
{
    applySheetChange([&] { sheet_->setClassHitDieResult(classTypeFromInt(type), classLevel, result); },
        tr("Impossibile impostare il risultato del dado vita."));
}

void SheetController::selectFavoredClassBonus(int type, int classLevel, const QString &optionId)
{
    applySheetChange([&] { sheet_->selectFavoredClassBonus(classTypeFromInt(type), classLevel, optionId.toStdString()); },
        tr("Impossibile scegliere il bonus di classe preferita."));
}

void SheetController::setClassArchetype(int type, const QString &archetypeId, bool enabled)
{
    applySheetChange([&] {
        if (enabled) sheet_->addClassArchetype(classTypeFromInt(type), archetypeId.toStdString());
        else sheet_->removeClassArchetype(classTypeFromInt(type), archetypeId.toStdString());
    }, enabled ? tr("Impossibile aggiungere l’archetipo. Verifica eventuali conflitti.")
               : tr("Impossibile rimuovere l’archetipo."));
}

void SheetController::setClassOption(int type, const QString &acquisitionId, const QString &choiceId, const QString &optionId, bool selected)
{
    applySheetChange([&] {
        const golarion::ClassOptionDefinition option{acquisitionId.toStdString(), choiceId.toStdString(), optionId.toStdString()};
        if (selected) sheet_->selectClassOption(classTypeFromInt(type), option);
        else sheet_->unselectClassOption(classTypeFromInt(type), option);
    }, tr("Impossibile modificare la scelta del privilegio."));
}

QVariantList SheetController::featCatalog(const QString &slotId)
{
    if (!sheet_ || !browsingCatalogReady_) return {};
    QVariantMap slot;
    if (!slotId.isEmpty())
    {
        const auto slots = feats_.value("feats").toMap().value("slots").toList();
        const auto found = std::ranges::find_if(slots, [&](const QVariant &entry) {
            return entry.toMap().value("id").toString() == slotId;
        });
        if (found == slots.end()) return {};
        slot = found->toMap();
        if (!slot.value("available").toBool()) return {};
    }
    const auto allowedType = slot.value("allowedType");
    const auto allowedIds = slot.value("allowedFeatIds").toList();
    if (allowedType.isNull() && allowedIds.isEmpty())
    {
        auto catalog = standardFeatCatalog();
        catalog.append(customFeatCatalog_);
        return catalog;
    }
    QVariantList catalog;
    const auto append = [&](const QVariantList &entries) {
        for (const auto &entry : entries)
        {
            const auto feat = entry.toMap();
            if ((!allowedType.isNull() && !feat.value("types").toList().contains(allowedType))
                || (!allowedIds.isEmpty() && !allowedIds.contains(feat.value("id")))) continue;
            catalog.push_back(entry);
        }
    };
    append(standardFeatCatalog());
    append(customFeatCatalog_);
    return catalog;
}

QVariantMap SheetController::featDetail(const QString &featId)
{
    if (!sheet_ || !previousInputs_) return {};
    try
    {
        const auto id = featId.toStdString();
        const auto &custom = previousInputs_->view.customFeatDefinitions;
        const auto it = std::ranges::find(custom, id, &golarion::FeatDefinition::id);
        if (it != custom.end()) return featDetails(*it);
        return featDetails(golarion::FeatDefinitionManager::instance().get(id));
    }
    catch (const golarion::catalog::CatalogReadError &error) { setErrorMessage(catalogReadMessage(error)); }
    catch (const std::exception &) { setErrorMessage(tr("Impossibile leggere il talento.")); }
    return {};
}

void SheetController::selectFeat(const QString &slotId, const QString &featId, const QVariantMap &choices)
{
    applySheetChange([&] {
        golarion::FeatChoices selections;
        for (auto choice = choices.cbegin(); choice != choices.cend(); ++choice)
        {
            auto &options = selections[choice.key().toStdString()];
            for (const auto &option : choice.value().toList()) options.push_back(option.toString().toStdString());
        }
        const bool extra = slotId.isEmpty();
        const auto id = extra ? inventoryId("feat.extra.") : slotId.toStdString();
        if (extra) sheet_->addExtraFeatSlot(id);
        try { sheet_->selectFeat(id, featId.toStdString(), std::move(selections)); }
        catch (...) { if (extra) sheet_->removeExtraFeatSlot(id); throw; }
    }, tr("Impossibile assegnare il talento. Verifica prerequisiti, scelte e talenti già acquisiti."));
}

void SheetController::removeFeat(const QString &slotId, bool extra)
{
    applySheetChange([&] {
        if (extra) sheet_->removeExtraFeatSlot(slotId.toStdString());
        else sheet_->removeFeat(slotId.toStdString());
    }, tr("Impossibile rimuovere il talento."));
}

void SheetController::addLanguage(const QString &name, int origin)
{
    applySheetChange([&] {
        if (origin < enumValue(golarion::LanguageOrigin::Intelligence) || origin > enumValue(golarion::LanguageOrigin::CustomLinguistics))
            throw std::invalid_argument("unknown language origin");
        sheet_->addLanguage(name.toStdString(), static_cast<golarion::LanguageOrigin>(origin));
    }, tr("Impossibile aggiungere la lingua. Verifica che non sia già nota e che ci siano posti disponibili."));
}

void SheetController::removeLanguage(const QString &id)
{
    applySheetChange([&] { sheet_->removeLanguage(id.toStdString()); }, tr("Impossibile rimuovere la lingua."));
}

QVariantList SheetController::enhancementBonusTypes() const
{
    QVariantList result;
    // BonusType is contiguous and Size is its last entry.
    for (int value = enumValue(golarion::BonusType::Generic); value <= enumValue(golarion::BonusType::Size); ++value)
    {
        const auto name = golarion::displayName(static_cast<golarion::BonusType>(value));
        result.push_back(QVariantMap{{"value", value}, {"label", QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()))}});
    }
    return result;
}

void SheetController::addEnhancementGroup(const QVariantMap &definition)
{
    applySheetChange([&] {
        sheet_->addEnhancementGroup({.id = inventoryId("enhancement."), .name = definition.value("name").toString().trimmed().toStdString(),
            .active = false, .description = identityText(definition.value("description"))});
    }, tr("Impossibile creare il gruppo di potenziamenti."));
}

void SheetController::addEnhancement(const QString &groupId, const QVariantMap &definition)
{
    applySheetChange([&] {
        const auto inputKind = identityNumber<int>(definition.value("kind"));
        if (!inputKind || *inputKind < enumValue(EnhancementInputKind::Contribution) || *inputKind > enumValue(EnhancementInputKind::Penalty))
            throw std::invalid_argument("unknown enhancement effect kind");
        const auto kind = static_cast<EnhancementInputKind>(*inputKind);
        const auto id = inventoryId("effect.");
        const auto resource = definition.value("resource").toString().toStdString();
        const auto expression = definition.value("expression").toString().toStdString();
        if (kind == EnhancementInputKind::Contribution)
        {
            sheet_->addEnhancement(groupId.toStdString(), golarion::ContributionEffectDefinition{id, resource, expression});
            return;
        }
        const auto type = kind == EnhancementInputKind::Bonus ? golarion::ModifierType::Bonus : golarion::ModifierType::Penalty;
        std::optional<golarion::BonusType> bonus;
        if (type == golarion::ModifierType::Bonus)
        {
            const auto value = identityNumber<int>(definition.value("bonusType"));
            if (!value || *value < enumValue(golarion::BonusType::Generic) || *value > enumValue(golarion::BonusType::Size))
                throw std::invalid_argument("unknown enhancement bonus type");
            bonus = static_cast<golarion::BonusType>(*value);
        }
        sheet_->addEnhancement(groupId.toStdString(), golarion::ModifierEffectDefinition{id, resource,
            definition.value("description").toString().toStdString(), type, bonus, expression});
    }, tr("Impossibile aggiungere l’effetto. Controlla il gruppo, la risorsa e l’espressione."));
}

void SheetController::removeEnhancement(const QString &groupId, const QString &effectId)
{
    applySheetChange([&] { sheet_->removeEnhancement(groupId.toStdString(), effectId.toStdString()); },
        tr("Impossibile rimuovere l’effetto."));
}

void SheetController::removeEnhancementGroup(const QString &groupId)
{
    applySheetChange([&] { sheet_->removeEnhancementGroup(groupId.toStdString()); },
        tr("Impossibile rimuovere il gruppo di potenziamenti."));
}

void SheetController::setEnhancementGroupActive(const QString &groupId, bool active)
{
    applySheetChange([&] { sheet_->setEnhancementGroupActive(groupId.toStdString(), active); },
        tr("Impossibile modificare lo stato del gruppo. Controlla le espressioni degli effetti."));
}

void SheetController::addCondition(const QVariantMap &definition)
{
    applySheetChange([&] {
        const auto id = definition.value("conditionId").toString().trimmed().toStdString();
        if (automaticCondition(id)) throw std::invalid_argument("condition is automatic");
        sheet_->addCondition(golarion::ConditionEntry({.id = inventoryId("condition."), .conditionId = id,
            .source = tr("Aggiunta manuale").toStdString(), .severity = identityNumber<int>(definition.value("severity")).value_or(1),
            .parameter = identityText(definition.value("parameter"))}));
    }, tr("Impossibile applicare la condizione."));
}

void SheetController::removeConditions(const QStringList &ids)
{
    if (ids.isEmpty()) return;
    const auto failure = tr("Impossibile rimuovere le condizioni selezionate.");
    if (ids.size() == 1)
    {
        applySheetChange([&] { sheet_->removeCondition(ids.first().toStdString()); }, failure);
        return;
    }
    applySheetTransaction([&](auto &candidate) {
        for (const auto &id : ids) candidate.removeCondition(id.toStdString());
    }, failure);
}

void SheetController::publish(bool save, QString error)
{
    if (save) queueCurrentSheetSave();
    auto next = std::make_unique<PublishedInputs>(PublishedInputs{sheet_->toView(), sheet_->pendingChoices(),
        sheet_->modifierResourceNames(), sheet_->contributionResourceNames(), sheet_->expressionTargetNames()});
    const auto &view = next->view;
    const auto changed = compareSheetViews(previousInputs_ ? &previousInputs_->view : nullptr, view);
    const auto hasSpellCatalog = [](const auto &snapshot) { return !snapshot.spellcastings.castings.empty() || !snapshot.grantedSpells.empty(); };

    // Publish atomically: convert only dirty inputs, then notify only when visible data differs.
    std::vector<void (SheetController::*)()> notifications;
    auto update = [&](QVariantMap &current, QVariantMap value, void (SheetController::*notify)()) {
        if (replaceIfChanged(current, std::move(value))) notifications.push_back(notify);
    };
    if (changed.identity)
        update(identity_, identitySnapshot(view.identity), &SheetController::identityChanged);
    if (changed.abilities)
        update(abilityScores_, abilitiesSnapshot(view), &SheetController::abilityScoresChanged);
    if (changed.skills)
        update(skillScores_, skillsSnapshot(view), &SheetController::skillScoresChanged);
    if (changed.saves)
        update(savingThrows_, savesSnapshot(view), &SheetController::savingThrowsChanged);
    if (changed.armor)
        update(armorClass_, QVariantMap{{"armorClass", armorClassSnapshot(view.armorClass)}}, &SheetController::armorClassChanged);
    if (changed.initiative)
        update(initiative_, QVariantMap{{"initiative", QVariantMap{{"abilityOptions", abilityOptionsSnapshot(view.initiative.abilityOptions)}}}}, &SheetController::initiativeChanged);
    if (changed.hp)
        update(hitPoints_, QVariantMap{{"hitPoints", hitPointsSnapshot(view.hitPoints)}}, &SheetController::hitPointsChanged);
    if (changed.defenses)
        update(defenses_, QVariantMap{{"defenses", defensesSnapshot(view.specialDefenses)}}, &SheetController::defensesChanged);
    if (changed.movement)
        update(movement_, QVariantMap{{"movement", movementSnapshot(view.movement)}}, &SheetController::movementChanged);
    if (changed.conditions)
        update(activeConditions_, QVariantMap{{"activeConditions", activeConditionsSnapshot(view.conditions)}}, &SheetController::activeConditionsChanged);
    if (changed.conditions)
        update(conditions_, conditionsSnapshot(view.conditions), &SheetController::conditionsChanged);
    if (changed.attacks)
        update(attacks_, QVariantMap{{"attacks", QVariantMap{{"attacks", snapshotList(view.attacks.attacks, attackSnapshot)}}}}, &SheetController::attacksChanged);
    if (changed.actions)
        update(actions_, actionsSnapshot(view), &SheetController::actionsChanged);
    if (changed.bab)
        update(baseAttackBonus_, QVariantMap{{"baseAttackBonus", QVariantMap{{"total", view.baseAttackBonus.total}}}}, &SheetController::baseAttackBonusChanged);
    if (changed.routines)
        update(attackRoutines_, attackRoutinesSnapshot(view), &SheetController::attackRoutinesChanged);
    if (changed.maneuvers)
        update(combatManeuvers_, maneuversSnapshot(view), &SheetController::combatManeuversChanged);
    if (changed.race)
        update(race_, raceSnapshot(view.race), &SheetController::raceChanged);
    if (changed.classes)
        update(classes_, classesSnapshot(view.classes), &SheetController::classesChanged);
    if (changed.items || changed.weight)
        update(inventoryItems_, inventoryItemsSnapshot(view.inventory), &SheetController::inventoryItemsChanged);
    if (changed.containers)
        update(inventoryContainers_, inventoryContainersSnapshot(view.inventory), &SheetController::inventoryContainersChanged);
    if (changed.equipment)
        update(inventoryEquipment_, inventoryEquipmentSnapshot(view.inventory), &SheetController::inventoryEquipmentChanged);
    if (changed.money)
        update(inventoryMoney_, inventoryMoneySnapshot(view.inventory), &SheetController::inventoryMoneyChanged);
    if (changed.load)
        update(encumbrance_, encumbranceSnapshot(view.encumbrance), &SheetController::encumbranceChanged);
    if (changed.feats)
        update(feats_, featsSnapshot(view.feats), &SheetController::featsChanged);
    if (changed.languages)
        update(languages_, languagesSnapshot(view.languages), &SheetController::languagesChanged);
    if (changed.reminders || previousInputs_->pending != next->pending)
        update(reminders_, remindersSnapshot(view.reminders, next->pending), &SheetController::remindersChanged);
    if (changed.castings)
        update(spellcastings_, spellcastingSnapshot(view), &SheetController::spellcastingsChanged);
    if (changed.granted)
        update(grantedSpells_, grantedSpellSnapshot(view), &SheetController::grantedSpellsChanged);
    if (changed.customSpells || hasSpellCatalog(previousInputs_->view) != hasSpellCatalog(view))
        update(spellCatalogData_, spellCatalogSnapshot(view), &SheetController::spellCatalogDataChanged);
    if (!previousInputs_ || lastCast_.value("lastCast").toMap() != lastSpellCast_)
        update(lastCast_, QVariantMap{{"lastCast", lastSpellCast_}}, &SheetController::lastCastChanged);
    if (changed.enhancements || previousInputs_->modifiers != next->modifiers ||
        previousInputs_->contributions != next->contributions || previousInputs_->targets != next->targets)
    {
        auto data = enhancementsSnapshot(view.enhancements);
        data.insert("modifierResources", snapshotList(next->modifiers, QString::fromStdString));
        data.insert("contributionResources", snapshotList(next->contributions, QString::fromStdString));
        data.insert("expressionTargets", snapshotList(next->targets, QString::fromStdString));
        update(enhancements_, std::move(data), &SheetController::enhancementsChanged);
    }
    if (changed.customFeats)
        customFeatCatalog_ = snapshotList(view.customFeatDefinitions, featSummary<golarion::FeatDefinition>);
    if (changed.notes && replaceIfChanged(notes_, QString::fromStdString(view.notes))) notifications.push_back(&SheetController::notesChanged);
    if (replaceIfChanged(errorMessage_, std::move(error))) notifications.push_back(&SheetController::errorMessageChanged);
    previousInputs_ = std::move(next);
    for (auto notify : notifications) Q_EMIT (this->*notify)();
}
