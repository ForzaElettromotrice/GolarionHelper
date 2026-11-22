import QtQuick
import QtQuick.Controls
import QtTest
import "../preview" as Preview

TestCase {
    id: testCase
    name: "PreviewHitPoints"
    when: windowShown
    Component { id: previewComponent; Preview.Main {} }

    function test_combatTabArrangesAllThreePanels() {
        const preview = createTemporaryObject(previewComponent, testCase, {actionsSnapshot: {categories: [
            {id: "movement", name: "Movimento", actions: [{id: "move", name: "Muoversi", usable: true}]}
        ]}});
        const home = findChild(preview.contentItem, "homePreview");
        const tabs = findChild(home, "sheetTabs");
        compare(tabs.itemAt(2).text, "Combattimento");
        home.currentTab = 2;
        const attacks = findChild(home, "sheetCombatAttacks");
        verify(attacks.visible);
        compare(attacks.x, 0);
        tryVerify(() => attacks.width > 0 && attacks.width < home.width);
        compare(attacks.attacks.attacks.length, 2);
        const maneuvers = findChild(home, "sheetCombatManeuvers");
        const actions = findChild(home, "sheetCombatActions");
        tryVerify(() => attacks.x < maneuvers.x && maneuvers.x < actions.x);
        verify(Math.abs(attacks.width - maneuvers.width) <= 1);
        verify(Math.abs(maneuvers.width - actions.width) <= 1);
        compare(maneuvers.x - attacks.x - attacks.width, actions.x - maneuvers.x - maneuvers.width);
        verify(attacks.prominentHeading && maneuvers.prominentHeading && actions.prominentHeading);
        compare(maneuvers.y, attacks.y);
        compare(actions.y, attacks.y);
        compare(maneuvers.combatManeuvers.maneuvers.length, 10);
        const disarm = findChild(maneuvers, "maneuverSituational_2");
        const trip = findChild(maneuvers, "maneuverSituational_9");
        verify(disarm.visible && trip.visible);
        compare(findChild(disarm, "situationalBonusValue").text, "+2 BMC · +1 DMC");
        compare(findChild(trip, "situationalBonusValue").text, "-2 DMC");
        verify(!findChild(maneuvers, "maneuverSituational_0").visible);
        compare(actions.visibleCategories[0].actions[0].name, "Muoversi");
        preview.width = 900;
        tryVerify(() => actions.y >= maneuvers.y + maneuvers.height);
        compare(actions.x, maneuvers.x);
        verify(maneuvers.x > attacks.x);
        preview.width = 700;
        tryVerify(() => maneuvers.y >= attacks.y + attacks.height);
        compare(maneuvers.x, attacks.x);
        compare(actions.x, attacks.x);
        findChild(attacks, "routine_full").click();
        const sword = findChild(attacks, "attack_full_0");
        const bite = findChild(attacks, "attack_full_3");
        const roll = findChild(sword, "attackRollSituational");
        const damage = findChild(sword, "attackDamageSituational");
        const confirmation = findChild(sword, "attackConfirmationSituational");
        verify(roll.visible && damage.visible && confirmation.visible);
        tryVerify(() => roll.mapToItem(sword, 0, 0).y < damage.mapToItem(sword, 0, 0).y && damage.y < confirmation.y);
        compare(findChild(roll, "attackSituationalValue").text, "+2");
        compare(findChild(damage, "attackSituationalValue").text, "+1");
        compare(findChild(confirmation, "attackSituationalValue").text, "+2");
        const rollLines = Array.from(roll.children).filter(child => findChild(child, "attackSituationalValue") !== null);
        const damageLine = findChild(damage, "attackSituationalValue").parent;
        compare(rollLines.length, 2);
        tryVerify(() => rollLines[0].width > 0 && rollLines[1].y > rollLines[0].y);
        for (const line of rollLines) compare(line.x, damageLine.x);
        const firstSpine = findChild(rollLines[0], "attackSituationalSpine");
        const secondSpine = findChild(rollLines[1], "attackSituationalSpine");
        tryVerify(() => firstSpine.mapToItem(roll, 0, firstSpine.height).y >= secondSpine.mapToItem(roll, 0, 0).y);
        compare(firstSpine.mapToItem(roll, 0, 0).x, secondSpine.mapToItem(roll, 0, 0).x);
        compare(findChild(rollLines[1], "attackSituationalValue").text, "-1");
        const rollBonus = findChild(sword, "attackBonus_full_0");
        compare(rollBonus.x + rollBonus.width, rollBonus.parent.width);
        compare(roll.y - rollBonus.parent.height, 1);
        compare(damageLine.x, 10);
        verify(damageLine.width < damage.width);
        verify(findChild(sword, "confirmation_full_0").visible);
        verify(!findChild(bite, "attackRollSituational").visible);
        verify(!findChild(bite, "confirmation_full_3").visible);
        home.combat = Object.assign({}, home.combat, {attacks: {attacks: home.combat.attacks.attacks.map(attack =>
            Object.assign({}, attack, {name: attack.name + " di prova"}))}});
        verify(findChild(attacks, "routine_full").checked);
        preview.close();
    }

    function test_inventoryTabReusesEquipmentItemsAndMoney() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const home = findChild(preview.contentItem, "homePreview");
        const tabs = findChild(home, "sheetTabs");
        compare(tabs.itemAt(3).text, "Inventario");
        home.currentTab = 3;
        const inventory = findChild(home, "sheetInventoryPreview");
        const equipment = findChild(inventory, "equipmentDiagram");
        const money = findChild(inventory, "sheetInventoryMoneyPreview");
        const load = findChild(inventory, "inventoryLoadCard");
        const lastContainer = findChild(inventory, "inventoryRow_home");
        verify(equipment.visible && money.visible && load.visible);
        tryVerify(() => money.mapToItem(inventory, 0, 0).y >= equipment.mapToItem(inventory, 0, equipment.height).y);
        compare(money.mapToItem(inventory, 0, 0).x, equipment.mapToItem(inventory, 0, 0).x);
        verify(load.mapToItem(inventory, 0, 0).x > money.mapToItem(inventory, 0, 0).x);
        verify(load.mapToItem(inventory, 0, 0).y >= lastContainer.mapToItem(inventory, 0, lastContainer.height).y);
        compare(money.money.total.gold, 13);
        inventory.restoreItemUsagePoolRequested("staff", "bastone", 1);
        compare(inventory.inventory.items.find(item => item.id === "staff").usagePools[0].remaining, 8);
        preview.close();
    }

    function test_featsTabPlacesRemindersBetweenLanguagesAndFeats() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const sheet = findChild(preview.contentItem, "homePreview");
        compare(findChild(sheet, "sheetTabs").itemAt(sheet.featsTabIndex).text, "Capacità");
        sheet.currentTab = sheet.featsTabIndex;
        const left = findChild(sheet, "sheetFeatLanguages");
        const feats = findChild(sheet, "sheetFeatsPreview");
        const reminders = findChild(sheet, "sheetFeatReminders");
        verify(left.visible && feats.visible && reminders.visible);
        tryVerify(() => left.x < reminders.x && reminders.x < feats.x);
        compare(reminders.y, left.y);
        compare(feats.y, left.y);
        compare(left.width, reminders.width);
        compare(reminders.width, feats.width);
        compare(reminders.x - left.x - left.width, 24);
        compare(feats.x - reminders.x - reminders.width, 24);
        for (const panel of [left, reminders, feats]) {
            const heading = findChild(panel, "sectionHeading");
            compare(heading.height, 42);
            compare(heading.background.width, panel.width);
        }
        preview.width = 950;
        tryVerify(() => reminders.x > left.x && feats.x === reminders.x && feats.y > reminders.y);
        preview.width = 700;
        tryVerify(() => left.x === reminders.x && reminders.x === feats.x && left.y < reminders.y && reminders.y < feats.y);
        preview.close();
    }

    function test_spellsTabShowsCastingAndGrants() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const sheet = findChild(preview.contentItem, "homePreview");
        compare(findChild(sheet, "sheetTabs").itemAt(sheet.spellsTabIndex).text, "Incantesimi");
        sheet.currentTab = sheet.spellsTabIndex;
        const page = sheet.spellsTabItem;
        const castings = page.castingArea.item;
        const grants = findChild(page.grantsArea.item, "grantedSpellsPreview");
        tryVerify(() => castings.visible && grants.visible && castings.height > 0 && grants.height > 0);
        compare(castings.width, page.width);
        tryVerify(() => page.grantsArea.y > page.castingArea.y);
        findChild(preview, "panelSelector").currentIndex = 16;
        tryVerify(() => findChild(preview.contentItem, "spellcastingsPreview")?.visible === true);
        preview.close();
    }

    function test_choiceReminderOpensRaceTab() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const sheet = findChild(preview.contentItem, "homePreview");
        sheet.currentTab = sheet.featsTabIndex;
        const reminders = findChild(sheet, "sheetFeatReminders");
        const choice = findChild(reminders, "choiceReminder_0");
        verify(choice.navigable);
        waitForRendering(preview.contentItem);
        mouseClick(choice);
        tryCompare(sheet, "currentTab", sheet.raceTabIndex);
        preview.close();
    }

    function test_raceHasItsOwnSheetTab() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const sheet = findChild(preview.contentItem, "homePreview");
        const tabs = findChild(sheet, "sheetTabs");
        compare(tabs.itemAt(sheet.raceTabIndex).text, "Razza");
        sheet.currentTab = sheet.raceTabIndex;
        const race = findChild(sheet, "racePanel");
        verify(race.visible);
        compare(race.race.race.name, "Umano");
        verify(race.race.activeElements.some(element => element.missingChoices.length > 0));
        preview.close();
    }

    function test_homeOpensWithAllDefenseCategories() {
        const preview = createTemporaryObject(previewComponent, testCase, {conditionCatalog: [
            {id: "bleeding", name: "Sanguinante", stages: [{name: "Sanguinante", severity: 1, effects: ["Danno a inizio turno"]}]},
            {id: "negativeLevels", name: "Livelli negativi", stages: [{name: "Livello negativo", severity: 1, effects: ["–1 ai tiri salvezza"]}]},
            {id: "fatigue", name: "Affaticamento", stages: [
                {name: "Affaticato", severity: 1, effects: ["–2 a Forza"]},
                {name: "Esausto", severity: 2, effects: ["Velocità dimezzata"]}
            ]},
            {id: "fear", name: "Paura", stages: [{name: "Scosso", severity: 1}, {name: "Spaventato", severity: 2}, {name: "In preda al panico", severity: 3}]},
            {id: "dazed", name: "Frastornato"}, {id: "prone", name: "Prono"},
            {id: "cowering", name: "Accovacciato"}, {id: "stunned", name: "Stordito"},
            {id: "unconscious", name: "Privo di sensi"}
        ]});
        const home = findChild(preview.contentItem, "homePreview");
        verify(home !== null);
        compare(home.currentTab, 1);
        const abilities = findChild(home, "sheetHomeAbilities");
        const checks = findChild(home, "sheetHomeChecks");
        tryVerify(() => checks.x > abilities.x && checks.height > 0 && Math.abs(checks.height - abilities.height) <= 1);
        verify(checks.topPadding > 0);
        compare(checks.bottomPadding, checks.topPadding);
        compare(checks.spacing, checks.freeHeight * 0.30);
        const hitPoints = findChild(home, "sheetHomeHitPoints");
        verify(hitPoints.visible);
        const land = findChild(home, "movement_land");
        verify(land.visible);
        tryVerify(() => land.mapToItem(home, 0, 0).y >= hitPoints.mapToItem(home, 0, hitPoints.height).y);
        compare(findChild(land, "runDistance").text, "36 m");
        const condition = findChild(home, "homeCondition_bleeding_cut");
        compare(condition.text, "Sanguinante · 3 PF");
        compare(findChild(home, "homeCondition_bleeding_claw").text, "Sanguinante · 1d4 PF");
        compare(findChild(home, "homeCondition_negativeLevels").text, "Livelli negativi · 2");
        compare(home.home.activeConditions.filter(entry => entry.iconId === "negativeLevels").length, 1);
        verify(condition.icon.source.toString().endsWith("/droplet.svg"));
        for (const [id, asset] of [["dazed", "knocked-out-stars"], ["prone", "despair"], ["cowering", "despair"],
                ["stunned", "punch"], ["unconscious", "sleep"]]) {
            verify(findChild(home, "homeCondition_" + id).icon.source.toString().endsWith("/" + asset + ".svg"));
        }
        verify(findChild(home, "homeCondition_dazed").icon.width > condition.icon.width);
        compare(findChild(home, "homeCondition_fatigue_1").text, "Affaticato");
        compare(findChild(home, "homeCondition_fatigue_2").text, "Esausto");
        verify(findChild(home, "homeCondition_fatigue_2").icon.source.toString().endsWith("/battery-low.svg"));
        for (const [severity, name] of [[1, "Scosso"], [2, "Spaventato"], [3, "In preda al panico"]]) {
            const stage = findChild(home, "homeCondition_fear_" + severity);
            compare(stage.text, name);
            verify(stage.icon.source.toString().endsWith("/ghost.svg"));
        }
        const conditionDetails = findChild(home, "sheetHomeConditionDetails_bleeding_cut");
        verify(condition.visible);
        verify(!conditionDetails.visible);
        condition.click();
        verify(conditionDetails.visible);
        verify(conditionDetails.summaryText.includes("Danno a inizio turno"));
        verify(conditionDetails.summaryText.includes("Ferita da lama"));
        condition.click();
        verify(!conditionDetails.visible);
        findChild(home, "homeCondition_fatigue_2").click();
        const exhaustion = findChild(home, "sheetHomeConditionDetails_fatigue_2");
        verify(exhaustion.visible);
        verify(exhaustion.summaryText.includes("–2 a Forza"));
        verify(exhaustion.summaryText.includes("Velocità dimezzata"));
        const defenses = home.home.defenses;
        verify(defenses.damageReductions.length > 0);
        verify(defenses.energyResistances.length > 0);
        verify(defenses.immunities.length > 0);
        verify(defenses.energyVulnerabilities.length > 0);
        preview.close();
    }

    function test_backgroundCancelsWithoutLeavingWindow() {
        const preview = createTemporaryObject(previewComponent, testCase);
        verify(preview !== null);
        preview.show();
        findChild(preview, "panelSelector").currentIndex = 4;
        waitForRendering(preview.contentItem);
        const display = findChild(preview.contentItem, "currentAndMaximum");
        const input = findChild(preview.contentItem, "amount");
        const checkbox = findChild(preview.contentItem, "nonLethal");
        verify(display !== null);
        mouseClick(display);
        input.text = "-7";
        waitForRendering(preview.contentItem);
        mouseClick(checkbox);
        verify(input.visible);
        compare(input.text, "-7");
        mouseClick(preview.contentItem, preview.contentItem.width - 20, preview.contentItem.height - 20);
        tryCompare(input, "visible", false);
        compare(input.text, "");
        compare(display.text, "24 / 32");
        preview.close();
    }
}
