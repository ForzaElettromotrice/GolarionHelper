import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "ClassesPanel"
    width: 900; height: 1200
    visible: true
    when: windowShown
    Component { id: component; ClassesPanel {} }
    Component { id: dieComponent; HitDieButton {} }
    SignalSpy { id: addArchetypeSpy; signalName: "addArchetypeRequested" }
    SignalSpy { id: removeArchetypeSpy; signalName: "removeArchetypeRequested" }
    function test_archetypePickerAndRemoval() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {classes: snapshot, width: 800,
            archetypeCatalog: {6: [{id: "example", name: "Selezionato", description: ""}, {id: "new", name: "Altro <test>", description: "Descrizione"}], 5: [{id: "rogue", name: "Ladro", description: ""}]}});
        addArchetypeSpy.target = panel; addArchetypeSpy.clear();
        removeArchetypeSpy.target = panel; removeArchetypeSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "chooseArchetype_6"));
        tryCompare(panel.archetypePopup, "opened", true);
        waitForRendering(panel.archetypePopup.contentItem);
        compare(panel.archetypeCandidates().map(entry => entry.id), ["new", "example"]);
        verify(!findChild(panel.archetypePopup.contentItem, "archetypeCandidate_example").enabled);
        const search = findChild(panel.archetypePopup.contentItem, "archetypeSearch");
        search.text = "altro";
        compare(panel.archetypeCandidates().length, 1);
        waitForRendering(panel.archetypePopup.contentItem);
        mouseClick(findChild(panel.archetypePopup.contentItem, "archetypeCandidate_new"));
        compare(Array.from(addArchetypeSpy.signalArguments[0]), [6, "new"]);
        tryCompare(panel.archetypePopup, "opened", false);
        mouseClick(findChild(panel, "removeArchetype_6_example"));
        compare(Array.from(removeArchetypeSpy.signalArguments[0]), [6, "example"]);
        compare(JSON.stringify(snapshot), original);
        mouseClick(findChild(panel, "chooseArchetype_6"));
        tryCompare(panel.archetypePopup, "opened", true);
        compare(search.text, "");
        panel.archetypePopup.reject();
        compare(addArchetypeSpy.count, 1);
    }
    function test_dieShapes_data() {
        return [6, 8, 10, 12].map(sides => ({tag: "d" + sides, sides: sides}));
    }
    function test_dieShapes(data) {
        const die = createTemporaryObject(dieComponent, testCase, {sides: data.sides, result: data.sides, classLevel: 1});
        waitForRendering(die);
        compare(die.width, 42);
        compare(die.height, 44);
        compare(die.contentItem.text, data.sides.toString());
        const svg = findChild(die, "hitDieSvg");
        verify(svg.visible);
        tryCompare(svg, "status", Image.Ready);
        verify(grabImage(die).width > 0);
    }
    SignalSpy { id: selectionSpy; signalName: "selectOptionRequested" }
    SignalSpy { id: removalSpy; signalName: "unselectOptionRequested" }
    SignalSpy { id: levelUpSpy; signalName: "addLevelRequested" }
    SignalSpy { id: levelDownSpy; signalName: "removeLevelRequested" }
    SignalSpy { id: favoredSpy; signalName: "favoredClassRequested" }
    SignalSpy { id: dieSpy; signalName: "hitDieResultRequested" }
    SignalSpy { id: bonusSpy; signalName: "favoredBonusRequested" }
    function test_favoredBonuses() {
        const snapshot = fixture();
        snapshot.favoredClasses = {classes: [{type: 6, favored: true,
            selections: [{classLevel: 1, optionId: "both", active: true}, {classLevel: 2, optionId: "both", active: false}],
            options: [{id: "both", name: "Entrambi", description: "Descrizione", available: true, benefitTypes: [0, 1], selectionsPerBenefit: 1},
                {id: "blocked", name: "Non disponibile", description: "", available: false, benefitTypes: [2]}]}]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {classes: snapshot, width: 800});
        bonusSpy.target = panel; bonusSpy.clear();
        waitForRendering(panel);
        compare(findChild(panel, "favoredBonus_6_1").text, "♥ 📖");
        const book = findChild(findChild(panel, "favoredBonus_6_1"), "skillRankBookIcon");
        verify(book.visible);
        tryCompare(book, "status", Image.Ready);
        compare(findChild(panel, "favoredBonus_6_2").text, "+");
        verify(!findChild(panel, "favoredBonus_5_1").visible);
        mouseClick(findChild(panel, "favoredBonus_6_3"));
        tryCompare(panel.bonusPopup, "opened", true);
        waitForRendering(panel.bonusPopup.contentItem);
        verify(!findChild(panel.bonusPopup.contentItem, "favoredBonusOption_blocked").enabled);
        mouseClick(findChild(panel.bonusPopup.contentItem, "favoredBonusOption_both"));
        compare(Array.from(bonusSpy.signalArguments[0]), [6, 3, "both"]);
        compare(findChild(panel, "favoredBonus_6_3").text, "+");
        compare(JSON.stringify(snapshot), original);
    }
    function test_favoredStarAndHitDice() {
        const snapshot = fixture();
        snapshot.favoredClasses = {selectedClasses: [6]};
        snapshot.classes[0].hitDie = 6;
        snapshot.classes[0].hitDieResults = {1: 6, 2: 3};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {classes: snapshot, width: 800});
        favoredSpy.target = panel; favoredSpy.clear();
        dieSpy.target = panel; dieSpy.clear();
        waitForRendering(panel);
        const star = findChild(panel, "favoredClass_6");
        compare(star.text, "★");
        mouseClick(star);
        compare(Array.from(favoredSpy.signalArguments[0]), [6, false]);
        compare(star.text, "★");
        const name = findChild(panel, "className_6");
        compare(star.y + star.baselineOffset, name.y + name.baselineOffset);
        compare(findChild(panel, "hitDie_6_1").result, 6);
        compare(findChild(panel, "hitDie_6_2").sides, 6);
        const die = findChild(panel, "hitDie_6_3");
        compare(die.result, 0);
        verify(findChild(panel, "hitDie_6_4") === null);
        mouseClick(die);
        tryCompare(panel.hitDiePopup, "opened", true);
        const input = findChild(panel.hitDiePopup.contentItem, "hitDieInput") ?? panel.hitDiePopup.contentItem;
        input.text = "7";
        verify(!input.acceptableInput);
        input.text = "0";
        verify(!input.acceptableInput);
        input.text = "5";
        verify(input.acceptableInput);
        input.accepted();
        compare(Array.from(dieSpy.signalArguments[0]), [6, 3, 5]);
        compare(die.result, 0);
        tryCompare(panel.hitDiePopup, "opened", false);
        compare(JSON.stringify(snapshot), original);
        const updated = JSON.parse(original);
        updated.favoredClasses.selectedClasses = [];
        updated.classes[0].hitDieResults[3] = 5;
        panel.classes = updated;
        waitForRendering(panel);
        compare(findChild(panel, "favoredClass_6").text, "☆");
        compare(findChild(panel, "hitDie_6_3").result, 5);
    }
    function test_chooseFavoredClassBeforeFirstLevel() {
        const panel = createTemporaryObject(component, testCase, {
            classes: {level: 0, classes: [], favoredClasses: {selectedClasses: []}},
            catalog: [{type: 6, name: "Mago"}], width: 800
        });
        favoredSpy.target = panel; favoredSpy.clear();
        levelUpSpy.target = panel; levelUpSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "chooseFavoredClass"));
        tryCompare(panel.classPopup, "opened", true);
        waitForRendering(panel.classPopup.contentItem);
        mouseClick(findChild(panel.classPopup.contentItem, "newClass_6"));
        compare(favoredSpy.count, 1);
        compare(Array.from(favoredSpy.signalArguments[0]), [6, true]);
        compare(levelUpSpy.count, 0);
    }

    function test_levelControlsAndNewClass() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {classes: snapshot, width: 800,
            catalog: [{type: 6, name: "Mago"}, {type: 4, name: "Guerriero"}, {type: 0, name: "Barbaro"}]});
        levelUpSpy.target = panel; levelUpSpy.clear();
        levelDownSpy.target = panel; levelDownSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "classLevelUp_6"));
        compare(levelUpSpy.count, 1);
        compare(levelUpSpy.signalArguments[0][0], 6);
        mouseClick(findChild(panel, "classLevelDown_6"));
        compare(levelDownSpy.count, 1);
        compare(levelDownSpy.signalArguments[0][0], 6);
        mouseClick(findChild(panel, "addClass"));
        tryCompare(panel.classPopup, "opened", true);
        compare(panel.availableClasses().map(entry => entry.type), [0, 4]);
        const search = findChild(panel.classPopup.contentItem, "classSearch");
        search.text = "guerr";
        compare(panel.availableClasses().map(entry => entry.type), [4]);
        waitForRendering(panel.classPopup.contentItem);
        mouseClick(findChild(panel.classPopup.contentItem, "newClass_4"));
        compare(levelUpSpy.count, 2);
        compare(levelUpSpy.signalArguments[1][0], 4);
        tryCompare(panel.classPopup, "opened", false);
        mouseClick(findChild(panel, "addClass"));
        tryCompare(panel.classPopup, "opened", true);
        compare(search.text, "");
        panel.classPopup.reject();
        compare(levelUpSpy.count, 2);
        compare(JSON.stringify(snapshot), original);
    }
    function test_choicePicker() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {classes: snapshot, width: 800});
        selectionSpy.target = panel; selectionSpy.clear();
        removalSpy.target = panel; removalSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "completeClassChoice_6_active1_choice"));
        tryCompare(panel.choicePopup, "opened", true);
        waitForRendering(panel.choicePopup.contentItem);
        const add = findChild(panel.choicePopup.contentItem, "classChoiceOption_open");
        verify(add.enabled);
        mouseClick(add);
        compare(selectionSpy.count, 1);
        compare(Array.from(selectionSpy.signalArguments[0]), [6, "active1", "choice", "open"]);
        tryCompare(panel.choicePopup, "opened", false);
        mouseClick(findChild(panel, "editClassChoice_6_active1_choice"));
        tryCompare(panel.choicePopup, "opened", true);
        mouseClick(findChild(panel.choicePopup.contentItem, "classChoiceOption_selected"));
        compare(removalSpy.count, 1);
        compare(Array.from(removalSpy.signalArguments[0]), [6, "active1", "choice", "selected"]);
        const full = snapshot.classes[0].features[0].acquisitions[0].choices[0];
        panel.editChoice(6, "active1", Object.assign({}, full, {remainingSelections: 0}));
        tryCompare(panel.choicePopup, "opened", true);
        verify(!findChild(panel.choicePopup.contentItem, "classChoiceOption_open").enabled);
        verify(findChild(panel.choicePopup.contentItem, "classChoiceOption_selected").enabled);
        panel.choicePopup.reject();
        compare(selectionSpy.count, 1);
        compare(removalSpy.count, 1);
        compare(JSON.stringify(snapshot), original);
    }
    function fixture() {
        return {level: 99, classes: [
            {type: ClassesPanel.Wizard, name: "Mago <test>", level: 3, archetypes: [{id: "example", name: "Archetipo <test>", description: "Test <description>"}], features: [
                {id: "active", name: "Privilegio", description: "Descrizione <test>", acquisitions: [
                    {id: "active1", level: 1, active: true, choices: [{id: "choice", prompt: "Scelta", selectionCount: 2, remainingSelections: 1,
                        blocksAdvancement: true, options: [{id: "selected", name: "Opzione scelta", selected: true}, {id: "open", name: "Da scegliere", selected: false}]}]},
                    {id: "inactive", level: 2, active: false, choices: [{id: "stale", remainingSelections: 1, options: [{id: "hidden", selected: true, name: "Nascosta"}]}]}
                ]},
                {id: "future", name: "Futuro", description: "Non ottenuto", acquisitions: [{id: "future5", level: 5, active: false, choices: []}]},
                {id: "other", name: "Altro", description: "Descrizione", acquisitions: [{id: "other1", level: 1, active: true, choices: []}]}
            ]},
            {type: ClassesPanel.Rogue, name: "Ladro", level: 2, features: []}
        ]};
    }
    function test_authoritativeDataAndActiveFeatures() {
        const snapshot = fixture();
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {classes: snapshot, width: 800});
        waitForRendering(panel);
        compare(findChild(panel, "totalClassLevel").text, "Livello totale: 99");
        compare(findChild(panel, "className_6").text, "Mago <test>");
        compare(findChild(panel, "className_6").textFormat, Text.PlainText);
        const archetypeNames = findChild(panel, "classArchetypeNames_6");
        compare(archetypeNames.text, "Archetipo <test>");
        compare(archetypeNames.textFormat, Text.PlainText);
        verify(archetypeNames.font.italic);
        verify(archetypeNames.font.pixelSize < findChild(panel, "className_6").font.pixelSize);
        verify(!findChild(panel, "classArchetypeNames_5").visible);
        compare(findChild(panel, "classLevel_6").text, "Livello 3");
        verify(findChild(panel, "classSection_5") !== null);
        verify(findChild(panel, "classFeature_6_future") === null);
        const features = panel.activeFeatures(snapshot.classes[0]);
        compare(features.length, 2);
        compare(features[0].acquisitions.length, 1);
        compare(panel.levelText(features[0]), "Livello 1");
        compare(panel.choicesFor(features[0])[0].selected[0].name, "Opzione scelta");
        compare(panel.choicesFor(features[0]).length, 1);
        const pending = panel.missingChoices(snapshot.classes[0]);
        compare(pending.length, 1);
        compare(pending[0].choice.remainingSelections, 1);
        compare(pending[0].acquisition.id, "active1");
        verify(findChild(panel, "classPendingChoice_6_active1_choice").visible);
        verify(findChild(panel, "classPendingChoice_6_inactive_stale") === null);
        compare(findChild(panel, "classFeatureDescription_6_active").textFormat, Text.PlainText);
        verify(findChild(panel, "classArchetype_6_example").text.includes("&lt;description&gt;"));
        compare(JSON.stringify(snapshot), original);
    }
    function test_responsiveLayoutAndEmptyState() {
        const panel = createTemporaryObject(component, testCase, {classes: fixture(), width: 800});
        waitForRendering(panel);
        const grid = findChild(panel, "classFeatureGrid_6");
        compare(grid.columns, 2);
        const first = findChild(panel, "classFeature_6_active");
        const second = findChild(panel, "classFeature_6_other");
        compare(first.width, second.width);
        compare(first.y, second.y);
        verify(second.x > first.x);
        panel.width = 280;
        waitForRendering(panel);
        compare(grid.columns, 1);
        compare(first.width, 280);
        compare(findChild(panel, "classPendingChoice_6_active1_choice").width, 280);
        panel.classes = null;
        compare(findChild(panel, "totalClassLevel").text, "Livello totale: —");
        verify(findChild(panel, "classSection_6") === null);
        panel.classes = {level: 0, classes: []};
        compare(findChild(panel, "totalClassLevel").text, "Livello totale: 0");
    }
}
