#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <QVariantList>
#include <QVariantMap>

#include "golarion/character/feat_definition_manager.hpp"
#include "golarion/character/class_definition_manager.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/race_definition_manager.hpp"

Q_IMPORT_QML_PLUGIN(GolarionSheetPlugin)

namespace
{
    QVariantList requirementVariants(const std::vector<golarion::FeatRequirementView> &requirements)
    {
        QVariantList result;
        for (const auto &requirement : requirements)
            result.push_back(QVariantMap{{"description", QString::fromStdString(requirement.description)}, {"manual", requirement.manual}});
        return result;
    }

    QVariantList noteVariants(const std::vector<std::string> &notes)
    {
        QVariantList result;
        for (const auto &note : notes) result.push_back(QString::fromStdString(note));
        return result;
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;
    QVariantList featCatalog;
    for (const auto &feat : golarion::FeatDefinitionManager::instance().toView())
    {
        QVariantList types;
        for (const auto type : feat.types) types.push_back(static_cast<int>(type));
        QVariantList choices;
        for (const auto &choice : feat.choices)
        {
            QVariantList options;
            for (const auto &option : choice.options)
                options.push_back(QVariantMap{{"id", QString::fromStdString(option.id)}, {"name", QString::fromStdString(option.name)},
                    {"requirements", requirementVariants(option.requirements)}, {"notes", noteVariants(option.notes)}});
            choices.push_back(QVariantMap{{"id", QString::fromStdString(choice.id)}, {"prompt", QString::fromStdString(choice.prompt)},
                {"selectionCount", static_cast<qulonglong>(choice.selectionCount)}, {"options", options}});
        }
        featCatalog.push_back(QVariantMap{{"id", QString::fromStdString(feat.id)},
            {"name", QString::fromStdString(feat.name)}, {"description", QString::fromStdString(feat.description)}, {"types", types},
            {"requirements", requirementVariants(feat.requirements)}, {"choices", choices},
            {"repeatability", static_cast<int>(feat.repeatability)}, {"notes", noteVariants(feat.notes)}});
    }
    QVariantList raceCatalog;
    QVariantMap raceAlternateCatalog;
    for (const auto &race : golarion::RaceDefinitionManager::instance().toView())
    {
        raceCatalog.push_back(QVariantMap{{"id", QString::fromStdString(race.id)}, {"name", QString::fromStdString(race.name)}});
        QVariantList alternatives;
        for (const auto &feature : golarion::RaceDefinitionManager::instance().alternateFeaturesView(race.id))
        {
            QVariantList replaces;
            for (const auto &id : feature.replaces) replaces.push_back(QString::fromStdString(id));
            alternatives.push_back(QVariantMap{{"id", QString::fromStdString(feature.id)}, {"name", QString::fromStdString(feature.name)},
                {"description", QString::fromStdString(feature.description)}, {"replaces", replaces}});
        }
        raceAlternateCatalog.insert(QString::fromStdString(race.id), alternatives);
    }
    QVariantList classCatalog;
    QVariantMap archetypeCatalog;
    for (int value = static_cast<int>(golarion::CharacterClassType::Barbarian); value <= static_cast<int>(golarion::CharacterClassType::UnchainedMonk); ++value)
    {
        const auto type = static_cast<golarion::CharacterClassType>(value);
        const auto &definition = golarion::ClassDefinitionManager::instance().get(type);
        classCatalog.push_back(QVariantMap{{"type", value}, {"name", QString::fromStdString(definition.name)}});
        QVariantList archetypes;
        for (const auto &archetype : golarion::ClassDefinitionManager::instance().archetypesView(type))
            archetypes.push_back(QVariantMap{{"id", QString::fromStdString(archetype.id)}, {"name", QString::fromStdString(archetype.name)}, {"description", QString::fromStdString(archetype.description)}});
        archetypeCatalog.insert(QString::number(value), archetypes);
    }
    QVariantList conditionCatalog;
    golarion::CharacterSheet sampleSheet;
    QStringList expressionTargetCatalog;
    for (const auto &name : sampleSheet.expressionTargetNames())
        expressionTargetCatalog.push_back(QString::fromStdString(name));
    sampleSheet.addCondition(golarion::ConditionEntry({"preview.staggered", "staggered", "Anteprima"}));
    const auto sheetView = sampleSheet.toView();
    for (const auto &condition : sheetView.conditions.conditions)
    {
        QVariantList stages;
        for (const auto &stage : condition.stages)
        {
            QStringList effects;
            QVariantList derivedConditions;
            for (const auto &effect : stage.effects)
                effects.push_back(QString::fromStdString(effect.description));
            for (const auto &derived : stage.derivedConditions)
                derivedConditions.push_back(QVariantMap{{"id", QString::fromStdString(derived.conditionId)}, {"severity", derived.severity}});
            stages.push_back(QVariantMap{{"name", QString::fromStdString(stage.name)}, {"severity", stage.severity},
                {"effects", effects}, {"derivedConditions", derivedConditions}});
        }
        QVariantMap entry{{"id", QString::fromStdString(condition.id)}, {"name", QString::fromStdString(condition.name)}, {"stages", stages}};
        if (condition.entryParameterName)
            entry.insert("entryParameterName", QString::fromStdString(*condition.entryParameterName));
        conditionCatalog.push_back(entry);
    }
    QVariantList actionCategories;
    for (const auto &category : sheetView.actions.categories)
    {
        QVariantList actions;
        for (const auto &action : category.actions)
        {
            QVariantList inhibitions;
            for (const auto &inhibition : action.inhibitions)
                inhibitions.push_back(QVariantMap{{"reason", QString::fromStdString(inhibition.reason)}, {"source", QString::fromStdString(inhibition.source)}});
            QVariantList notes;
            for (const auto &note : action.notes)
                notes.push_back(QVariantMap{{"text", QString::fromStdString(note.text)}, {"source", QString::fromStdString(note.source)}});
            actions.push_back(QVariantMap{{"id", QString::fromStdString(action.id)}, {"name", QString::fromStdString(action.name)},
                {"description", QString::fromStdString(action.description)}, {"source", QString::fromStdString(action.source)},
                {"effectiveCost", static_cast<int>(action.effectiveCost)}, {"usable", action.usable}, {"inhibitions", inhibitions}, {"notes", notes}});
        }
        actionCategories.push_back(QVariantMap{{"id", QString::fromStdString(category.id)}, {"name", QString::fromStdString(category.name)}, {"actions", actions}});
    }
    QVariantList actionRoutines;
    for (const auto &routine : sheetView.attackRoutines.routines)
        actionRoutines.push_back(QVariantMap{{"actionId", QString::fromStdString(routine.actionId)}});
    engine.setInitialProperties({{"featCatalog", featCatalog}, {"raceCatalog", raceCatalog}, {"raceAlternateCatalog", raceAlternateCatalog},
        {"classCatalog", classCatalog}, {"archetypeCatalog", archetypeCatalog}, {"conditionCatalog", conditionCatalog},
        {"expressionTargetCatalog", expressionTargetCatalog},
        {"actionsSnapshot", QVariantMap{{"categories", actionCategories}}}, {"actionRoutinesSnapshot", QVariantMap{{"routines", actionRoutines}}}});
    engine.loadFromModule("Golarion.Preview", "Main");
    if (engine.rootObjects().isEmpty()) return 1;
    return application.exec();
}
