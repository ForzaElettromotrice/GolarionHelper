#include "../sheet_controller.hpp"
#include "golarion/character/ability.hpp"
#include "golarion/character/character_class_type.hpp"
#include "golarion/character/pending_choice.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/data/languages_data.hpp"
#include "golarion/resource/modifier.hpp"

#include <QSignalSpy>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class SheetControllerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createsReopensAndForgetsSavedSheets()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        QCoreApplication::setOrganizationName("GolarionSheetTest");
        const auto url = QUrl::fromLocalFile(directory.filePath(QStringLiteral("Scheda di prova.json")));

        SheetController creator;
        QSignalSpy opened(&creator, &SheetController::sheetOpened);
        creator.createSavedSheet(url);
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 15000);
        QVERIFY(QFile::exists(url.toLocalFile()));
        creator.setNotes(QStringLiteral("Note salvate"));
        QVERIFY2(creator.errorMessage().isEmpty(), qPrintable(creator.errorMessage()));

        creator.createSavedSheet(url);
        QCOMPARE(opened.count(), 1);
        QVERIFY(!creator.errorMessage().isEmpty());

        SheetController reader;
        QCOMPARE(reader.savedSheets().size(), 1);
        QSignalSpy reopened(&reader, &SheetController::sheetOpened);
        reader.openSavedSheet(url);
        QTRY_COMPARE_WITH_TIMEOUT(reopened.count(), 1, 15000);
        QCOMPARE(reader.notes(), QStringLiteral("Note salvate"));
        QCOMPARE(reader.savedSheets().size(), 1);

        const auto moved = directory.filePath(QStringLiteral("Importata.json"));
        QVERIFY(QFile::rename(url.toLocalFile(), moved));
        QVERIFY(!reader.savedSheets().first().toMap().value("available").toBool());
        const auto imported = QUrl::fromLocalFile(moved);
        reader.openSavedSheet(imported);
        QCOMPARE(reopened.count(), 2);
        QCOMPARE(reader.savedSheets().size(), 2);
        reader.forgetSavedSheet(url);
        reader.forgetSavedSheet(imported);
        QVERIFY(reader.savedSheets().isEmpty());
        QVERIFY(QFile::exists(moved));
    }

    void listsAllEnhancementBonusTypesWithoutAnOpenSheet()
    {
        SheetController controller;
        const auto types = controller.enhancementBonusTypes();
        QCOMPARE(types.size(), 19);
        for (int index = 0; index < types.size(); ++index)
        {
            const auto entry = types.at(index).toMap();
            QCOMPARE(entry.value("value").toInt(), index);
            const auto name = golarion::displayName(static_cast<golarion::BonusType>(index));
            QCOMPARE(entry.value("label").toString(), QString::fromStdString(std::string(name)));
        }
    }

    void updatesLiveEnhancementsAndConditions()
    {
        SheetController controller;
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        controller.openLocalSheet();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        const auto group = [&] { return controller.enhancements().value("groups").toList().value(0).toMap(); };
        const auto modifiers = controller.enhancements().value("modifierResources").toList();
        const auto contributions = controller.enhancements().value("contributionResources").toList();
        const auto targets = controller.enhancements().value("expressionTargets").toList();
        QVERIFY(targets.contains(QStringLiteral("str")));
        QVERIFY(targets.contains(QStringLiteral("level")));
        QVERIFY(!targets.contains(QStringLiteral("hp.max")));
        QVERIFY(modifiers.contains(QStringLiteral("str")));
        QVERIFY(!modifiers.contains(QStringLiteral("hp.max")));
        QVERIFY(!modifiers.contains(QStringLiteral("level")));
        QVERIFY(contributions.contains(QStringLiteral("hp.max")));
        QVERIFY(!contributions.contains(QStringLiteral("str")));
        const auto strength = [&] { return controller.home().value("abilities").toList().first().toMap().value("totalValue").toInt(); };
        const int initialStrength = strength();
        controller.addEnhancementGroup({{"name", QStringLiteral("Prova")}});
        const auto groupId = group().value("id").toString();
        QVERIFY(!groupId.isEmpty());
        QVERIFY(group().value("effectsEditable").toBool());
        const QVariantMap effect{{"kind", 1}, {"resource", QStringLiteral("str")}, {"description", QStringLiteral("Prova")},
            {"expression", QStringLiteral("2")}, {"bonusType", 0}};
        controller.addEnhancement(groupId, effect);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(strength(), initialStrength);
        controller.setEnhancementGroupActive(groupId, true);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(strength(), initialStrength + 2);
        QVERIFY(!group().value("effectsEditable").toBool());
        controller.addEnhancement(groupId, effect);
        QVERIFY(!controller.errorMessage().isEmpty());
        controller.setEnhancementGroupActive(groupId, false);
        QCOMPARE(strength(), initialStrength);
        const auto effectId = group().value("enhancements").toList().first().toMap().value("definition").toMap().value("id").toString();
        controller.removeEnhancement(groupId, effectId);
        QVERIFY(group().value("enhancements").toList().isEmpty());
        controller.removeEnhancementGroup(groupId);
        QVERIFY(group().isEmpty());

        const auto condition = [&](const QString &conditionId) {
            for (const auto &entry : controller.conditions().value("conditions").toList())
                if (entry.toMap().value("id").toString() == conditionId) return entry.toMap();
            return QVariantMap{};
        };
        controller.addCondition({{"conditionId", QStringLiteral("dead")}, {"severity", 1}});
        QVERIFY(!controller.errorMessage().isEmpty());
        const auto bleed = QStringLiteral("bleeding");
        controller.addCondition({{"conditionId", bleed}, {"severity", 1}, {"parameter", QStringLiteral("1d6")}});
        controller.addCondition({{"conditionId", bleed}, {"severity", 1}, {"parameter", QStringLiteral("2 PF")}});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto entries = condition(bleed).value("entries").toList();
        QCOMPARE(entries.size(), 2);
        const auto first = entries.first().toMap().value("id").toString();
        controller.removeConditions({first, QStringLiteral("missing")});
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(condition(bleed).value("entries").toList().size(), 2);
        controller.removeConditions({first});
        QCOMPARE(condition(bleed).value("entries").toList().size(), 1);
        QCOMPARE(condition(bleed).value("effectiveSeverity").toInt(), 1);
        controller.removeConditions({entries.last().toMap().value("id").toString()});
        QCOMPARE(condition(bleed).value("effectiveSeverity").toInt(), 0);
    }

    void updatesLiveSpells()
    {
        SheetController controller;
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        controller.openLocalSheet();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        const auto casting = [&] { return controller.spells().value("castings").toList().value(0).toMap(); };
        QVERIFY(casting().isEmpty());
        controller.setIdentityField("alignment", 1);
        controller.setRace("dwarf");
        const int wizard = static_cast<int>(golarion::CharacterClassType::Wizard);
        controller.setFavoredClass(wizard, true);
        controller.setAbilityBaseValue(static_cast<int>(golarion::AbilityType::Intelligence), 14);
        controller.addClassLevel(wizard);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto id = casting().value("id").toString();
        QVERIFY(!id.isEmpty());
        QVERIFY(!controller.spells().value("catalog").toList().isEmpty());
        const auto slotAtLevelOne = [&] {
            for (const auto &group : casting().value("state").toMap().value("groups").toList())
                for (const auto &entry : group.toMap().value("slots").toList())
                    if (entry.toMap().value("slot").toMap().value("spellLevel").toInt() == 1)
                        return entry.toMap();
            return QVariantMap{};
        };
        const auto slot = slotAtLevelOne().value("slot").toMap();
        QVERIFY(!slot.isEmpty());
        controller.spellCommand(SheetController::AddToRepertoire, id, {{"spellId", QStringLiteral("magicMissile")}});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(controller.spellPreparationCount(id, slot, QStringLiteral("magicMissile"), {}), 1);
        QCOMPARE(slotAtLevelOne().value("state").toInt(), 0);
        controller.spellCommand(SheetController::Prepare, id, {{"slots", QVariantList{slot}}, {"spellId", QStringLiteral("magicMissile")}});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QVERIFY(controller.canConsumeSpell(SheetController::ConsumePrepared, id, slot, {}));
        QCOMPARE(slotAtLevelOne().value("state").toInt(), 1);
        controller.spellCommand(SheetController::ConsumePrepared, id, {{"slot", slot}});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(slotAtLevelOne().value("state").toInt(), 2);
        QVERIFY(!controller.spells().value("lastCast").toMap().isEmpty());
        controller.spellCommand(SheetController::ResetSlots, id, {});
        QCOMPARE(slotAtLevelOne().value("state").toInt(), 0);
        QVERIFY(casting().value("repertoire").toList().contains(QStringLiteral("magicMissile")));

        const QVariantMap usage{{"type", QStringLiteral("sharedPool")}, {"poolId", QStringLiteral("charges")}, {"cost", 2}};
        const QVariantMap effect{{"id", QStringLiteral("spell")}, {"type", QStringLiteral("spellGrant")},
            {"activation", QStringLiteral("possessed")}, {"spellId", QStringLiteral("magicMissile")}, {"usage", usage}};
        controller.addCustomItem({{"name", QStringLiteral("Bastone di prova")}, {"weightGrams", 0}, {"volumeMilliliters", 0},
            {"usagePools", QVariantList{QVariantMap{{"id", QStringLiteral("charges")}, {"capacity", 3}}}},
            {"effects", QVariantList{effect}}});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto grant = [&] { return controller.spells().value("granted").toList().value(0).toMap(); };
        QCOMPARE(grant().value("remaining").toInt(), 3);
        controller.spellCommand(SheetController::ConsumeGranted, grant().value("id").toString(), {});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(grant().value("remaining").toInt(), 1);
        const auto item = controller.inventory().value("items").toList().value(0).toMap();
        QCOMPARE(item.value("usagePools").toList().value(0).toMap().value("remaining").toInt(), 1);
    }

    void updatesLiveTalentsAndLanguages()
    {
        SheetController controller;
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        controller.openLocalSheet();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        const auto feats = [&] { return controller.talents().value("feats").toMap(); };
        const auto languages = [&] { return controller.talents().value("languages").toMap(); };
        const auto reminders = [&] { return controller.talents().value("reminders").toMap().value("messages").toList(); };
        const auto hasChoice = [&](golarion::PendingChoiceType type) {
            for (const auto &entry : controller.talents().value("reminders").toMap().value("choices").toList())
                if (entry.toMap().value("type").toInt() == static_cast<int>(type)) return true;
            return false;
        };
        QVERIFY(hasChoice(golarion::PendingChoiceType::Race));
        controller.setRace("human");
        QVERIFY(controller.errorMessage().isEmpty());
        QVERIFY(!hasChoice(golarion::PendingChoiceType::Race));
        QVERIFY(hasChoice(golarion::PendingChoiceType::RacialOption));
        QVERIFY(feats().value("slots").toList().isEmpty());
        controller.setIdentityField("alignment", 1);
        const int fighter = static_cast<int>(golarion::CharacterClassType::Fighter);
        controller.setFavoredClass(fighter, true);
        controller.setRacialChoice("human.abilityScores", "ability", {QStringLiteral("strength")});
        controller.addClassLevel(fighter);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QVERIFY(!controller.featCatalog("base.feat.level1").isEmpty());
        controller.selectFeat("base.feat.level1", "improvedInitiative", {});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        controller.selectFeat({}, "unknown", {});
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(feats().value("feats").toList().size(), 1);
        controller.selectFeat({}, "dodge", {});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto acquisitions = feats().value("feats").toList();
        QCOMPARE(acquisitions.size(), 2);
        for (const auto &acquisition : acquisitions)
            if (acquisition.toMap().value("id").toString() == QStringLiteral("dodge"))
                controller.removeFeat(acquisition.toMap().value("slotId").toString(), true);
        QCOMPARE(feats().value("feats").toList().size(), 1);
        controller.removeFeat("base.feat.level1", false);
        QVERIFY(feats().value("feats").toList().isEmpty());

        const auto origin = [](golarion::LanguageOrigin value) { return static_cast<int>(value); };
        const auto spokenCount = languages().value("spoken").toList().size();
        controller.addLanguage("Naviganti", origin(golarion::LanguageOrigin::Extra));
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(languages().value("spoken").toList().size(), spokenCount + 1);
        controller.removeLanguage("Naviganti");
        QCOMPARE(languages().value("spoken").toList().size(), spokenCount);
        controller.addLanguage("Segni delle isole", origin(golarion::LanguageOrigin::CustomLinguistics));
        QVERIFY(!controller.errorMessage().isEmpty());
        for (const auto &entry : controller.home().value("skills").toList())
            if (entry.toMap().value("type").toInt() == static_cast<int>(golarion::SkillType::Linguistics))
                controller.setSkillRanks(entry.toMap().value("resourceName").toString(), 1);
        QCOMPARE(languages().value("remainingLinguistics").toInt(), 1);
        const auto before = reminders().size();
        controller.addLanguage("Segni delle isole", origin(golarion::LanguageOrigin::CustomLinguistics));
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(languages().value("remainingLinguistics").toInt(), 0);
        QCOMPARE(reminders().size(), before - 1);
    }

    void updatesLiveClasses()
    {
        SheetController controller;
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        controller.openLocalSheet();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        const int wizard = static_cast<int>(golarion::CharacterClassType::Wizard);
        const auto level = [&] { return controller.classes().value("level").toInt(); };
        const auto entry = [&] { return controller.classes().value("classes").toList().value(0).toMap(); };
        const auto catalog = controller.classCatalog();
        QVERIFY(!catalog.value("classes").toList().isEmpty());
        const auto archetypes = catalog.value("archetypes").toMap().value(QString::number(wizard)).toList();
        QVERIFY(!archetypes.isEmpty());
        controller.addClassLevel(wizard);
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(level(), 0);
        controller.setIdentityField("alignment", 1);
        controller.setRace("dwarf");
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        controller.setFavoredClass(wizard, true);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        controller.addClassLevel(wizard);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(level(), 1);
        QCOMPARE(entry().value("hitDie").toInt(), 6);
        controller.setClassHitDieResult(wizard, 1, 6);
        QVERIFY(controller.errorMessage().isEmpty());
        QCOMPARE(entry().value("hitDieResults").toMap().value("1").toInt(), 6);
        const auto favoredClasses = [&] { return controller.classes().value("favoredClasses").toMap(); };
        QVERIFY(favoredClasses().value("selectedClasses").toList().contains(wizard));
        controller.selectFavoredClassBonus(wizard, 1, QStringLiteral("base.hitPoint"));
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto favored = favoredClasses().value("classes").toList().first().toMap();
        QCOMPARE(favored.value("selections").toList().first().toMap().value("optionId").toString(), QStringLiteral("base.hitPoint"));
        const auto archetypeId = archetypes.first().toMap().value("id").toString();
        controller.setClassArchetype(wizard, archetypeId, true);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(entry().value("archetypes").toList().first().toMap().value("id").toString(), archetypeId);
        controller.setClassArchetype(wizard, archetypeId, false);
        QVERIFY(controller.errorMessage().isEmpty());
        QVERIFY(entry().value("archetypes").toList().isEmpty());
        QString acquisitionId, choiceId, optionId;
        for (const auto &feature : entry().value("features").toList())
            for (const auto &acquisition : feature.toMap().value("acquisitions").toList())
                if (acquisition.toMap().value("active").toBool() && choiceId.isEmpty())
                    for (const auto &choice : acquisition.toMap().value("choices").toList())
                        if (const auto options = choice.toMap().value("options").toList(); !options.isEmpty())
                        {
                            acquisitionId = acquisition.toMap().value("id").toString();
                            choiceId = choice.toMap().value("id").toString();
                            optionId = options.first().toMap().value("id").toString();
                            break;
                        }
        QVERIFY(!optionId.isEmpty());
        controller.setClassOption(wizard, acquisitionId, choiceId, optionId, true);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        controller.setClassOption(wizard, acquisitionId, choiceId, optionId, false);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        controller.setFavoredClass(wizard, false);
        QVERIFY(controller.errorMessage().isEmpty());
        QVERIFY(favoredClasses().value("selectedClasses").toList().isEmpty());
        controller.removeClassLevel(wizard);
        QVERIFY(controller.errorMessage().isEmpty());
        QCOMPARE(level(), 0);
    }

    void updatesLiveRace()
    {
        SheetController controller;
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        controller.openLocalSheet();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        QVERIFY(controller.race().value("race").isNull());
        QVERIFY(!controller.raceCatalog().isEmpty());
        controller.setRace("human");
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(controller.race().value("race").toMap().value("name").toString(), QStringLiteral("Umano"));
        QVERIFY(!controller.race().value("alternateFeatures").toList().isEmpty());
        const auto dualTalent = [&] {
            for (const auto &entry : controller.race().value("activeElements").toList())
                if (entry.toMap().value("id").toString() == QStringLiteral("human.dualTalent")) return entry.toMap();
            return QVariantMap{};
        };
        const auto strength = [&] { return controller.home().value("abilities").toList().first().toMap().value("totalValue").toInt(); };
        controller.selectAlternateRacialTrait("human.dualTalent");
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QVERIFY(dualTalent().value("removable").toBool());
        const auto pending = dualTalent().value("missingChoices").toList();
        QCOMPARE(pending.size(), 1);
        QVERIFY(pending.first().toMap().value("blocksAdvancement").toBool());
        const int baseStrength = strength();
        controller.setRacialChoice("human.dualTalent", "abilities", {QStringLiteral("strength")});
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(strength(), baseStrength);
        controller.setRacialChoice("human.dualTalent", "abilities", {QStringLiteral("strength"), QStringLiteral("dexterity")});
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(strength(), baseStrength + 2);
        QVERIFY(dualTalent().value("missingChoices").toList().isEmpty());
        QCOMPARE(dualTalent().value("choices").toList().first().toMap().value("selectedOptions").toList().size(), 2);
        controller.setRacialChoice("human.dualTalent", "abilities", {QStringLiteral("constitution"), QStringLiteral("dexterity")});
        QVERIFY(controller.errorMessage().isEmpty());
        QCOMPARE(strength(), baseStrength);
        controller.removeAlternateRacialTrait("human.dualTalent");
        QVERIFY(controller.errorMessage().isEmpty());
        QVERIFY(dualTalent().isEmpty());
        controller.setRace("elf");
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(controller.race().value("race").toMap().value("id").toString(), QStringLiteral("elf"));
    }

    void updatesLiveInventory()
    {
        SheetController controller;
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        controller.openLocalSheet();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        const auto items = [&] { return controller.inventory().value("items").toList(); };
        const auto containers = [&] { return controller.inventory().value("containers").toList(); };
        const auto amount = [&](const QString &coin) {
            return controller.inventory().value("money").toMap().value("total").toMap().value(coin).toInt();
        };
        QVERIFY(items().isEmpty());
        QVERIFY(controller.inventory().contains("encumbrance"));
        QVERIFY(!controller.itemCatalog().isEmpty());
        controller.addCatalogItem(QStringLiteral("goldCoin"));
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto goldId = items().first().toMap().value("id").toString();
        controller.setItemQuantity(goldId, 3);
        QCOMPARE(amount("gold"), 3);
        controller.splitCoinStack(goldId, 1);
        QCOMPARE(items().size(), 2);
        const auto otherCoins = items().first().toMap().value("id").toString() == goldId ? items().last() : items().first();
        controller.mergeCoinStacks(otherCoins.toMap().value("id").toString(), goldId);
        QCOMPARE(items().size(), 1);
        QCOMPARE(amount("gold"), 3);
        controller.addContainer({{"name", QStringLiteral("Casa")}, {"allowsPossessionEffects", false}});
        QCOMPARE(containers().size(), 2);
        QString homeId;
        for (const auto &container : containers())
            if (container.toMap().value("name").toString() == QStringLiteral("Casa"))
                homeId = container.toMap().value("id").toString();
        QVERIFY(!homeId.isEmpty());
        controller.convertMoney(2, 1, 1, 10, QStringLiteral("worn"), QStringLiteral("missing"));
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(amount("gold"), 3);
        controller.convertMoney(2, 1, 1, 10, QStringLiteral("worn"), homeId);
        QCOMPARE(amount("gold"), 2);
        QCOMPARE(amount("silver"), 10);
        controller.changeMoney(1, -4, homeId);
        QCOMPARE(amount("silver"), 6);
        controller.moveAndReorderItem(goldId, homeId, QStringLiteral("missing"));
        QVERIFY(!controller.errorMessage().isEmpty());
        const auto findItem = [&](const QString &key, const QString &value) {
            for (const auto &entry : items())
                if (entry.toMap().value(key).toString() == value) return entry.toMap();
            return QVariantMap{};
        };
        QCOMPARE(findItem("id", goldId).value("containerId").toString(), QStringLiteral("worn"));
        controller.moveAndReorderItem(goldId, homeId, QVariant{});
        QCOMPARE(findItem("id", goldId).value("containerId").toString(), homeId);

        const auto addArmor = [&](const QString &name) {
            controller.addCustomItem({{"name", name}, {"weightGrams", 100}, {"volumeMilliliters", 50}, {"slot", 1}});
            return findItem("name", name).value("id").toString();
        };
        const auto firstArmor = addArmor(QStringLiteral("Corazza A"));
        QVERIFY2(!firstArmor.isEmpty(), qPrintable(controller.errorMessage()));
        const auto secondArmor = addArmor(QStringLiteral("Corazza B"));
        QVERIFY(!secondArmor.isEmpty());
        controller.equipItem(firstArmor, {});
        QVERIFY(findItem("id", firstArmor).value("equipped").toBool());
        controller.moveItem(secondArmor, homeId);
        controller.equipItem(secondArmor, firstArmor);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QVERIFY(findItem("id", secondArmor).value("equipped").toBool());
        QCOMPARE(findItem("id", firstArmor).value("containerId").toString(), homeId);
        controller.unequipItem(secondArmor, homeId);
        QVERIFY(!findItem("id", secondArmor).value("equipped").toBool());
        QCOMPARE(controller.inventory().value("totalCarriedWeightGrams").toLongLong(), 0);
    }

    void createsAndReopensLocalSheet()
    {
        SheetController controller;
        QSignalSpy changes(&controller, &SheetController::sheetChanged);
        QSignalSpy opened(&controller, &SheetController::sheetOpened);
        QSignalSpy openingChanges(&controller, &SheetController::openingChanged);
        QVERIFY(controller.identity().isEmpty());
        controller.setNotes(QStringLiteral("Not opened"));
        QCOMPARE(changes.count(), 0);

        controller.preloadClassCatalog();
        controller.openLocalSheet();
        controller.openLocalSheet();
        QVERIFY(controller.opening());
        QCOMPARE(opened.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 10000);
        QVERIFY(!controller.opening());
        QCOMPARE(openingChanges.count(), 2);
        QVERIFY(controller.identity().contains(QStringLiteral("name")));
        QCOMPARE(controller.home().value(QStringLiteral("abilities")).toList().size(), 6);
        QCOMPARE(controller.home().value(QStringLiteral("savingThrows")).toList().size(), 3);
        QVERIFY(!controller.home().value(QStringLiteral("skills")).toList().isEmpty());
        QVERIFY(controller.home().value(QStringLiteral("movement")).toMap().contains(QStringLiteral("grants")));
        const auto maneuvers = controller.combat().value(QStringLiteral("combatManeuvers")).toMap().value(QStringLiteral("maneuvers")).toList();
        QCOMPARE(maneuvers.size(), 10);
        const auto maneuverModifiers = maneuvers.first().toMap().value(QStringLiteral("bonus")).toMap().value(QStringLiteral("modifiers")).toMap();
        QVERIFY(maneuverModifiers.contains(QStringLiteral("conditionalTotals")));
        QVERIFY(!controller.combat().value(QStringLiteral("actions")).toMap().value(QStringLiteral("categories")).toList().isEmpty());
        const auto armorOptions = controller.home().value(QStringLiteral("armorClass")).toMap().value(QStringLiteral("abilityOptions")).toList();
        QVERIFY(!armorOptions.isEmpty());
        QCOMPARE(armorOptions.first().toMap().value(QStringLiteral("values")).toList().size(), 3);
        const auto temporary = controller.home().value(QStringLiteral("hitPoints")).toMap().value(QStringLiteral("temporary")).toMap();
        QVERIFY(temporary.contains(QStringLiteral("pools")));
        QCOMPARE(temporary.value(QStringLiteral("total")).toInt(), 0);
        QVERIFY(controller.notes().isEmpty());
        QCOMPARE(changes.count(), 1);

        controller.addTemporaryHitPoints(QStringLiteral("Prova"), 5);
        const auto updatedTemporary = controller.home().value(QStringLiteral("hitPoints")).toMap().value(QStringLiteral("temporary")).toMap();
        QCOMPARE(updatedTemporary.value(QStringLiteral("total")).toInt(), 5);
        QCOMPARE(updatedTemporary.value(QStringLiteral("pools")).toList().first().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("Prova"));

        const auto notes = QStringLiteral("Prima riga\nSeconda riga");
        controller.setNotes(notes);
        QCOMPARE(controller.notes(), notes);
        QCOMPARE(changes.count(), 3);
        controller.setIdentityField(QStringLiteral("name"), QStringLiteral("Arianna"));
        controller.setIdentityField(QStringLiteral("age"), 28);
        QCOMPARE(controller.identity().value(QStringLiteral("name")).toString(), QStringLiteral("Arianna"));
        QCOMPARE(controller.identity().value(QStringLiteral("age")).toInt(), 28);
        controller.setIdentityField(QStringLiteral("age"), QVariant{});
        QVERIFY(controller.identity().value(QStringLiteral("age")).isNull());
        controller.openLocalSheet();
        QCOMPARE(opened.count(), 2);
        QCOMPARE(controller.notes(), notes);
        QCOMPARE(changes.count(), 6);

        const auto hp = [&] { return controller.home().value(QStringLiteral("hitPoints")).toMap(); };
        controller.removeTemporaryHitPoints(QStringLiteral("Prova"));
        QCOMPARE(hp().value(QStringLiteral("temporary")).toMap().value(QStringLiteral("total")).toInt(), 0);
        const auto current = hp().value(QStringLiteral("current")).toInt();
        controller.damage(1, false);
        QCOMPARE(hp().value(QStringLiteral("current")).toInt(), current - 1);
        controller.heal(1);
        QCOMPARE(hp().value(QStringLiteral("current")).toInt(), current);

        const auto findSkill = [&](const QString &name) -> QVariantMap
        {
            for (const auto &entry : controller.home().value(QStringLiteral("skills")).toList())
                if (entry.toMap().value(QStringLiteral("name")).toString() == name) return entry.toMap();
            return {};
        };
        controller.setAbilityBaseValue(static_cast<int>(golarion::AbilityType::Strength), 16);
        QCOMPARE(controller.home().value(QStringLiteral("abilities")).toList().first().toMap().value(QStringLiteral("baseValue")).toInt(), 16);
        const auto acrobatics = QStringLiteral("Acrobazia");
        const auto acrobaticsResource = findSkill(acrobatics).value(QStringLiteral("resourceName")).toString();
        QVERIFY(!acrobaticsResource.isEmpty());
        controller.setSkillRanks(acrobaticsResource, 2);
        QCOMPARE(findSkill(acrobatics).value(QStringLiteral("ranks")).toInt(), 2);
        const auto acrobaticsType = static_cast<int>(golarion::SkillType::Acrobatics);
        controller.setCustomClassSkill(acrobaticsType, true);
        QVERIFY(findSkill(acrobatics).value(QStringLiteral("customClassSkill")).toBool());
        controller.setCustomClassSkill(acrobaticsType, false);
        QVERIFY(!findSkill(acrobatics).value(QStringLiteral("classSkill")).toBool());

        const auto craftType = static_cast<int>(golarion::SkillType::Craft);
        controller.setCustomClassSkill(craftType, true);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        const auto craftCategory = [&] {
            for (const auto &entry : controller.home().value(QStringLiteral("skillCategories")).toList())
                if (entry.toMap().value(QStringLiteral("type")).toInt() == craftType) return entry.toMap();
            return QVariantMap{};
        };
        QVERIFY(craftCategory().value(QStringLiteral("customClassSkill")).toBool());
        const auto name = QStringLiteral("Intarsio di prova");
        controller.addSkillSpecialization(craftType, name);
        const auto specialization = findSkill(name);
        QVERIFY(specialization.value(QStringLiteral("custom")).toBool());
        QVERIFY(specialization.value(QStringLiteral("classSkill")).toBool());
        controller.setCustomClassSkill(craftType, false);
        QVERIFY(controller.errorMessage().isEmpty());
        QVERIFY(!craftCategory().value(QStringLiteral("classSkill")).toBool());
        QVERIFY(!findSkill(name).value(QStringLiteral("classSkill")).toBool());
        controller.setSkillRanks(specialization.value(QStringLiteral("resourceName")).toString(), 1);
        QCOMPARE(findSkill(name).value(QStringLiteral("ranks")).toInt(), 1);
        controller.removeSkillSpecialization(craftType, specialization.value(QStringLiteral("specializationId")).toString());
        QVERIFY(findSkill(name).isEmpty());

        controller.addTemporaryHitPoints(QStringLiteral("Prova"), 2);
        controller.damage(1, true);
        QCOMPARE(hp().value(QStringLiteral("temporary")).toMap().value(QStringLiteral("total")).toInt(), 1);
        QCOMPARE(hp().value(QStringLiteral("current")).toInt(), current);
        const auto attacks = [&] { return controller.combat().value(QStringLiteral("attacks")).toMap().value(QStringLiteral("attacks")).toList(); };
        QVERIFY(attacks().isEmpty());
        controller.addAttackRoutine(QStringLiteral("base.standardAttack"));
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(attacks().size(), 1);
        const auto routine = attacks().first().toMap();
        const auto attackId = routine.value(QStringLiteral("id")).toString();
        const auto routineSlots = routine.value(QStringLiteral("slots")).toList();
        QVERIFY(!attackId.isEmpty());
        QCOMPARE(routineSlots.size(), 1);
        const auto slotId = routineSlots.first().toMap().value(QStringLiteral("id")).toString();
        controller.renameAttackRoutine(attackId, QStringLiteral(" Colpo preciso "));
        QVERIFY(controller.errorMessage().isEmpty());
        QCOMPARE(attacks().first().toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Colpo preciso"));
        QCOMPARE(attacks().first().toMap().value(QStringLiteral("id")).toString(), attackId);
        controller.assignAttackStrike(attackId, slotId, QStringLiteral("missing-strike"));
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(attacks().size(), 1);
        controller.addAttackRoutine(QStringLiteral("base.standardAttack"));
        QCOMPARE(attacks().size(), 2);
        const auto otherId = attacks().last().toMap().value(QStringLiteral("id")).toString();
        controller.reorderAttackRoutine(otherId, attackId);
        QVERIFY(controller.errorMessage().isEmpty());
        QCOMPARE(attacks().first().toMap().value(QStringLiteral("id")).toString(), otherId);
        controller.reorderAttackRoutine(otherId, QVariant{});
        QCOMPARE(attacks().last().toMap().value(QStringLiteral("id")).toString(), otherId);
        controller.removeAttackRoutine(otherId);
        controller.removeAttackRoutine(attackId);
        QVERIFY(controller.errorMessage().isEmpty());
        QVERIFY(attacks().isEmpty());
        controller.damage(100, false);
        bool dead = false;
        for (const auto &entry : controller.home().value(QStringLiteral("activeConditions")).toList())
            dead |= entry.toMap().value(QStringLiteral("id")).toString() == QStringLiteral("dead");
        QVERIFY(dead);
    }
};

QTEST_GUILESS_MAIN(SheetControllerTest)
#include "sheet_controller_test.moc"
