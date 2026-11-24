import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "NotesPanel"
    when: windowShown
    width: 450
    height: 400
    visible: true
    Component { id: panelComponent; NotesPanel {} }
    Component { id: sheetComponent; SheetView {} }
    SignalSpy { id: saveSpy; signalName: "notesChangeRequested" }
    SignalSpy { id: identitySpy; signalName: "identityFieldChangeRequested" }

    function test_identityEnterCommitsOnlyThatField() {
        const sheet = createTemporaryObject(sheetComponent, testCase, {
            width: 420, height: 380, identity: {name: "Arianna", age: 30}, notes: ""
        });
        identitySpy.target = sheet;
        identitySpy.clear();
        const panel = findChild(sheet, "sheetIdentityPanel");
        verify(!findChild(panel, "applyButton").visible && !findChild(panel, "resetButton").visible);
        panel.edit("age", "31");
        const field = findChild(panel, "nameInput");
        field.forceActiveFocus();
        field.selectAll();
        keyClick(Qt.Key_E);
        keyClick(Qt.Key_Return);
        compare(identitySpy.count, 1);
        compare(identitySpy.signalArguments[0][0], "name");
        compare(identitySpy.signalArguments[0][1], "e");
        verify(!field.activeFocus);
        sheet.identity = {name: "e", age: 30};
        compare(panel.draft.age, "31");
        compare(findChild(panel, "ageInput").parent.parent, findChild(panel, "weightInput").parent.parent);
    }

    function test_identityTabIncludesNotes() {
        const sheet = createTemporaryObject(sheetComponent, testCase, {
            width: 420, height: 380, identity: {name: "Arianna"}, notes: "Note di prova"
        });
        waitForRendering(sheet);
        compare(findChild(sheet, "sheetTabs").count, 2);
        verify(findChild(sheet, "sheetIdentityPanel").visible);
        const editor = findChild(sheet, "sheetNotesEditor");
        verify(editor.visible);
        compare(editor.text, "Note di prova");
        compare(findChild(sheet, "sheetContentScroll").contentItem.boundsBehavior, Flickable.StopAtBounds);
        compare(findChild(sheet, "sheetNotesScroll").contentItem.boundsBehavior, Flickable.StopAtBounds);
        const identity = findChild(sheet, "sheetIdentityPanel");
        const notes = findChild(sheet, "sheetNotesPanel");
        const divider = findChild(sheet, "identityDivider");
        verify(notes.y > identity.y);
        verify(!divider.visible);
        sheet.width = 1100;
        tryVerify(() => notes.x > identity.x);
        compare(notes.y, identity.y);
        verify(divider.visible);
        findChild(sheet, "sheetTabs").currentIndex = 1;
        const summary = findChild(sheet, "sheetHomeSummary");
        tryVerify(() => findChild(sheet, "sheetHomeSkills")?.visible === true);
        const skills = findChild(sheet, "sheetHomeSkills");
        const skillsPosition = () => skills.mapToItem(summary.parent, 0, 0);
        tryVerify(() => skillsPosition().x > summary.x);
        compare(skillsPosition().y, summary.y);
        const abilities = findChild(sheet, "sheetHomeAbilities");
        const checks = findChild(sheet, "sheetHomeChecks");
        const armor = findChild(sheet, "sheetHomeArmorClass");
        const hitPoints = findChild(sheet, "sheetHomeHitPoints");
        compare(armor.parent, summary);
        compare(hitPoints.parent, summary);
        tryVerify(() => armor.y >= checks.y + checks.height && hitPoints.y > armor.y);
        compare(hitPoints.width, summary.width);
        const addTemporary = findChild(hitPoints, "addTemporary");
        verify(!addTemporary.visible);
        sheet.hitPointEditingEnabled = true;
        verify(addTemporary.visible);
        verify(checks.x > abilities.x);
        compare(checks.y, abilities.y);
        sheet.width = 950;
        tryVerify(() => checks.y > abilities.y);
        verify(skillsPosition().x > summary.x);
        sheet.width = 420;
        tryVerify(() => skillsPosition().y > summary.y);
    }

    function test_keyboardSaveAndNewline() {
        const panel = createTemporaryObject(panelComponent, testCase, {width: 420, notes: "Prima riga"});
        saveSpy.target = panel;
        saveSpy.clear();
        const editor = findChild(panel, "sheetNotesEditor");
        compare(editor.text, "Prima riga");
        editor.forceActiveFocus();
        editor.cursorPosition = editor.length;
        keyClick(Qt.Key_Return, Qt.ShiftModifier);
        verify(editor.activeFocus);
        compare(editor.text, "Prima riga\n");
        compare(saveSpy.count, 0);
        editor.insert(editor.cursorPosition, "Seconda riga");
        keyClick(Qt.Key_Return);
        verify(!editor.activeFocus);
        compare(saveSpy.count, 1);
        compare(saveSpy.signalArguments[0][0], "Prima riga\nSeconda riga");
        compare(panel.notes, "Prima riga");
        panel.notes = "Prima riga\nSeconda riga";
        editor.forceActiveFocus();
        keyClick(Qt.Key_Enter);
        verify(!editor.activeFocus);
        compare(saveSpy.count, 1);
        panel.notes = "Aggiornamento esterno";
        compare(editor.text, "Aggiornamento esterno");
    }
}
