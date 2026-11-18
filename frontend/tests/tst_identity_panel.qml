import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "IdentityPanel"
    width: 800
    height: 1100
    visible: true
    when: windowShown

    Component { id: panelComponent; IdentityPanel { width: 720 } }
    SignalSpy { id: requested; signalName: "identityChangeRequested" }
    property var panel

    function sample() {
        return { name: "Arianna", playerName: "Giulia", alignment: IdentityPanel.NeutralGood,
            deity: null, homeland: "Sandpoint", gender: null, age: 27,
            heightCentimeters: 172, weightGrams: 64125, hair: "Castani", eyes: "Verdi", appearance: null };
    }

    function init() {
        panel = createTemporaryObject(panelComponent, testCase, { identity: sample() });
        verify(panel !== null);
        verify(panel.visible);
        waitForRendering(panel);
        requested.target = panel;
        requested.clear();
    }

    function test_initialData() {
        compare(findChild(panel, "nameInput").text, "Arianna");
        compare(findChild(panel, "weightInput").text, "64.125");
        compare(findChild(panel, "alignmentInput").currentValue, IdentityPanel.NeutralGood);
        verify(panel.valid);
        verify(!panel.dirty);
        verify(!findChild(panel, "applyButton").enabled);
        compare(requested.count, 0);
    }

    function test_editRequestsWithoutMutatingInput() {
        const input = findChild(panel, "nameInput");
        input.forceActiveFocus();
        input.selectAll();
        keyClick(Qt.Key_E);
        keyClick(Qt.Key_L);
        keyClick(Qt.Key_E);
        keyClick(Qt.Key_N);
        keyClick(Qt.Key_A);
        compare(panel.draft.name, "elena");
        compare(panel.identity.name, "Arianna");
        compare(requested.count, 0);
        mouseClick(findChild(panel, "applyButton"));
        compare(requested.count, 1);
        compare(requested.signalArguments[0][0].name, "elena");
        compare(panel.identity.name, "Arianna");
        panel.identity = requested.signalArguments[0][0];
        verify(!panel.dirty);
        verify(!panel.externalChangePending);
    }

    function test_optionalValuesAndUnits() {
        panel.edit("age", "");
        panel.edit("heightCentimeters", "");
        panel.edit("weightKilograms", "72,345");
        panel.edit("deity", "  ");
        panel.edit("alignment", IdentityPanel.Unspecified);
        panel.applyChanges();
        compare(requested.count, 1);
        const value = requested.signalArguments[0][0];
        compare(value.age, null);
        compare(value.heightCentimeters, null);
        compare(value.weightGrams, 72345);
        compare(value.deity, null);
        compare(value.alignment, IdentityPanel.Unspecified);
    }

    function test_validation_data() {
        return [
            { tag: "negativeAge", field: "age", value: "-1" },
            { tag: "fractionalAge", field: "age", value: "2.5" },
            { tag: "overflowAge", field: "age", value: "2147483648" },
            { tag: "zeroHeight", field: "heightCentimeters", value: "0" },
            { tag: "negativeWeight", field: "weightKilograms", value: "-12" },
            { tag: "weightPrecision", field: "weightKilograms", value: "1.0001" },
            { tag: "unsafeWeight", field: "weightKilograms", value: "9007199254740.992" },
            { tag: "unknownAlignment", field: "alignment", value: 100 }
        ];
    }

    function test_validation(data) {
        panel.edit(data.field, data.value);
        verify(!panel.valid);
        verify(!panel.canApply);
        panel.applyChanges();
        compare(requested.count, 0);
    }

    function test_resetAndReadOnly() {
        panel.edit("name", "Elena");
        mouseClick(findChild(panel, "resetButton"));
        compare(panel.draft.name, "Arianna");
        compare(findChild(panel, "nameInput").text, "Arianna");
        verify(!panel.dirty);
        panel.readOnly = true;
        panel.edit("name", "Changed");
        compare(panel.draft.name, "Arianna");
        verify(findChild(panel, "nameInput").readOnly);
        verify(!findChild(panel, "alignmentInput").enabled);
        compare(requested.count, 0);
    }

    function test_busyAndRejectionKeepDraft() {
        panel.edit("name", "Elena");
        panel.busy = true;
        panel.applyChanges();
        compare(requested.count, 0);
        panel.busy = false;
        panel.errorMessage = "Modifica rifiutata";
        compare(panel.draft.name, "Elena");
        verify(panel.dirty);
        verify(panel.canApply);
    }

    function test_alignmentKeyboardAndAppearance() {
        const alignment = findChild(panel, "alignmentInput");
        alignment.forceActiveFocus();
        keyClick(Qt.Key_Down);
        compare(panel.draft.alignment, IdentityPanel.ChaoticGood);
        compare(panel.identity.alignment, IdentityPanel.NeutralGood);
        const appearance = findChild(panel, "appearanceInput");
        appearance.forceActiveFocus();
        keyClick(Qt.Key_A);
        compare(panel.draft.appearance, "a");
        panel.resetDraft();
        compare(appearance.text, "");
        compare(alignment.currentValue, IdentityPanel.NeutralGood);
    }

    function test_emptyIdentity() {
        panel.identity = {};
        verify(panel.valid);
        compare(panel.draft.alignment, IdentityPanel.Unspecified);
        compare(findChild(panel, "weightInput").text, "");
        panel.edit("age", "0");
        panel.applyChanges();
        compare(requested.count, 1);
        compare(requested.signalArguments[0][0].age, 0);
        compare(requested.signalArguments[0][0].weightGrams, null);
    }

    function test_weightPrecision() {
        for (const grams of [1, 10, 100, 1000, 64125, 9007199254740991]) {
            const next = sample();
            next.weightGrams = grams;
            panel.identity = next;
            verify(panel.valid);
            compare(panel.proposal().weightGrams, grams);
        }
    }

    function test_longAppearanceGrowsWithoutOverlappingActions() {
        const next = sample();
        next.appearance = "Una descrizione lunga del personaggio. ".repeat(100);
        panel.identity = next;
        panel.width = 360;
        waitForRendering(panel);
        const appearance = findChild(panel, "appearanceInput");
        const apply = findChild(panel, "applyButton");
        verify(appearance.height >= appearance.contentHeight);
        const bottom = appearance.mapToItem(panel, 0, appearance.height).y;
        verify(apply.mapToItem(panel, 0, 0).y >= bottom);
    }

    function test_externalUpdatePreservesUnsavedEdits() {
        panel.edit("name", "Elena");
        const next = sample();
        next.playerName = "Marco";
        panel.identity = next;
        compare(panel.draft.name, "Elena");
        verify(panel.externalChangePending);
        verify(!panel.canApply);
        panel.resetDraft();
        compare(panel.draft.playerName, "Marco");
        compare(panel.draft.name, "Arianna");
        verify(!panel.externalChangePending);
    }

    function test_cleanExternalUpdateAndNarrowLayout() {
        const next = sample();
        next.name = "Elena";
        next.alignment = IdentityPanel.ChaoticGood;
        panel.identity = next;
        compare(findChild(panel, "nameInput").text, "Elena");
        compare(findChild(panel, "alignmentInput").currentValue, IdentityPanel.ChaoticGood);
        panel.width = 360;
        waitForRendering(panel);
        for (const name of ["nameInput", "alignmentInput", "weightInput", "appearanceInput", "applyButton"]) {
            const control = findChild(panel, name);
            const position = control.mapToItem(panel, 0, 0);
            verify(position.x >= 0);
            verify(position.x + control.width <= panel.width + 1);
        }
    }
}
