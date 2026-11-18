import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "LanguagesPanel"
    width: 500; height: 800
    visible: true
    when: windowShown
    Component { id: component; LanguagesPanel {} }
    SignalSpy { id: addSpy; signalName: "addRequested" }
    SignalSpy { id: removeSpy; signalName: "removeRequested" }
    SignalSpy { id: grantSpy; signalName: "grantRequested" }
    SignalSpy { id: customLearnSpy; signalName: "customLearnRequested" }
    SignalSpy { id: customGrantSpy; signalName: "customGrantRequested" }
    function test_customLanguages() {
        const panel = createTemporaryObject(component, testCase, {languages: {
            spoken: [], remainingIntelligence: 1, remainingLinguistics: 1, options: []
        }});
        customLearnSpy.target = panel; customLearnSpy.clear();
        customGrantSpy.target = panel; customGrantSpy.clear();
        waitForRendering(panel);
        const linguistics = findChild(panel, "languageSlot_1_0");
        mouseClick(linguistics);
        const menu = linguistics.choicesPopup;
        tryCompare(menu, "opened", true);
        const input = menu.itemAt(menu.count - 1);
        verify(input.visible);
        input.textField.forceActiveFocus();
        input.textField.text = "   ";
        keyClick(Qt.Key_Return);
        compare(customLearnSpy.count, 0);
        input.textField.text = "  Lingua della campagna  ";
        keyClick(Qt.Key_Return);
        compare(customLearnSpy.count, 1);
        compare(Array.from(customLearnSpy.signalArguments[0]), ["Lingua della campagna"]);
        tryCompare(menu, "visible", false);
        mouseClick(linguistics);
        tryCompare(menu, "opened", true);
        compare(input.textField.text, "");
        menu.close();
        const intelligence = findChild(panel, "languageSlot_0_0");
        mouseClick(intelligence);
        tryCompare(intelligence.choicesPopup, "opened", true);
        verify(!intelligence.choicesPopup.itemAt(intelligence.choicesPopup.count - 1).visible);
        intelligence.choicesPopup.close();
        const extra = findChild(panel, "addExtraLanguage");
        mouseClick(extra);
        tryCompare(extra.choicesPopup, "opened", true);
        const extraInput = extra.choicesPopup.itemAt(extra.choicesPopup.count - 1);
        extraInput.textField.forceActiveFocus();
        extraInput.textField.text = "Lingua libera";
        keyClick(Qt.Key_Return);
        compare(customGrantSpy.count, 1);
        compare(Array.from(customGrantSpy.signalArguments[0]), ["Lingua libera"]);
        compare(panel.languages.remainingLinguistics, 1);
    }
    function test_extraLanguageWithoutQuota() {
        const snapshot = {spoken: [{id: "common", grantedSources: ["Razza"]}], remainingIntelligence: 0, remainingLinguistics: -1, options: []};
        const panel = createTemporaryObject(component, testCase, {languages: snapshot, extraLanguageIds: ["common", "sylvan", "sylvan"]});
        grantSpy.target = panel;
        grantSpy.clear();
        waitForRendering(panel);
        const button = findChild(panel, "addExtraLanguage");
        verify(button.enabled);
        mouseClick(button);
        const menu = button.choicesPopup;
        tryCompare(menu, "opened", true);
        compare(panel.extraChoices(), ["sylvan"]);
        mouseClick(button);
        tryCompare(menu, "visible", false);
        mouseClick(button);
        tryCompare(menu, "opened", true);
        mouseClick(menu.itemAt(0));
        compare(grantSpy.count, 1);
        compare(Array.from(grantSpy.signalArguments[0]), ["sylvan"]);
        compare(snapshot.remainingIntelligence, 0);
        compare(snapshot.spoken.length, 1);
        panel.extraLanguageIds = [];
        verify(button.enabled);
    }
    function fixture() {
        return {spoken: [{id: "common", grantedSources: ["Razza"], learnedFrom: null},
            {id: "elven", grantedSources: ["Altra fonte"], learnedFrom: LanguagesPanel.Intelligence}],
            remainingIntelligence: 1, remainingLinguistics: 0, options: [
                {origin: LanguagesPanel.Intelligence, languageIds: ["common", "elven", "orc", "orc", "dwarven"]},
                {origin: LanguagesPanel.Linguistics, languageIds: ["sylvan"]}]};
    }
    function test_groupsQuotasAndRemoval() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {languages: snapshot, width: 260,
            languageNames: {common: "Comune", elven: "Elfico", orc: "Orchesco", dwarven: "Nanico"}});
        removeSpy.target = panel;
        removeSpy.clear();
        waitForRendering(panel);
        compare(panel.groupEntries(false).length, 1);
        compare(panel.groupEntries(true).length, 1);
        verify(!findChild(panel, "removeLanguage_common").visible);
        mouseClick(findChild(panel, "removeLanguage_elven"));
        compare(removeSpy.count, 1);
        compare(Array.from(removeSpy.signalArguments[0]), ["elven"]);
        compare(panel.choices(LanguagesPanel.Intelligence), ["dwarven", "orc"]);
        compare(findChild(panel, "languageQuota_0").text, "Intelligenza: 1 disponibile");
        compare(JSON.stringify(snapshot), original);
        panel.languages = {spoken: snapshot.spoken, remainingIntelligence: -1, remainingLinguistics: 0};
        compare(findChild(panel, "languageQuota_0").text, "Intelligenza: 1 in eccesso");
        compare(panel.groupEntries(true).length, 1);
    }
    function test_addMenu() {
        const snapshot = fixture();
        const panel = createTemporaryObject(component, testCase, {languages: snapshot});
        addSpy.target = panel;
        addSpy.clear();
        waitForRendering(panel);
        const button = findChild(panel, "languageSlot_0_0");
        verify(findChild(panel, "languageSlot_0_1") === null);
        verify(findChild(panel, "languageSlot_1_0") === null);
        mouseClick(button);
        const menu = button.choicesPopup;
        tryCompare(menu, "opened", true);
        mouseClick(button);
        tryCompare(menu, "visible", false);
        compare(addSpy.count, 0);
        mouseClick(button);
        tryCompare(menu, "opened", true);
        compare(menu.itemAt(0).text, "dwarven");
        mouseClick(menu.itemAt(0));
        compare(addSpy.count, 1);
        compare(Array.from(addSpy.signalArguments[0]), ["dwarven", LanguagesPanel.Intelligence]);
        compare(snapshot.spoken.length, 2);
        menu.close();
        tryCompare(menu, "visible", false);
        mouseMove(testCase, 480, 20);
        compare(button.background.border.color, "#c6cfd5");
        mouseClick(button);
        tryCompare(menu, "opened", true);
        mouseClick(testCase, 480, 20);
        tryCompare(menu, "visible", false);
        compare(button.background.border.color, "#c6cfd5");
        mouseClick(button);
        tryCompare(menu, "opened", true);
        keyClick(Qt.Key_Escape);
        tryCompare(menu, "visible", false);
        mouseMove(testCase, 480, 20);
        compare(button.background.border.color, "#c6cfd5");
    }
    function test_missing() {
        const panel = createTemporaryObject(component, testCase, {languages: null});
        compare(panel.quotaText(LanguagesPanel.Intelligence), "Intelligenza: —");
        compare(panel.choices(LanguagesPanel.Intelligence).length, 0);
        compare(panel.languageName("unknown"), "unknown");
        panel.languages = {remainingIntelligence: 2, options: []};
        compare(panel.choices(LanguagesPanel.Intelligence).length, 0);
        verify(findChild(panel, "languageSlot_0_0") !== null);
        verify(findChild(panel, "languageSlot_0_1") !== null);
        verify(findChild(panel, "languageSlot_0_2") === null);
        panel.languages = {remainingIntelligence: -1};
        verify(findChild(panel, "languageSlot_0_0") === null);
    }
}
