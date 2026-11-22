import QtQuick
import QtTest
import "../preview" as Preview

TestCase {
    id: testCase
    name: "PendingChoicesPanel"
    when: windowShown
    Component { id: previewComponent; Preview.Main {} }

    function test_groupsAndNavigatesToSelectedCasting() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const selector = findChild(preview, "panelSelector");
        selector.currentIndex = 22;
        const panel = findChild(preview, "pendingChoicesPreview");
        waitForRendering(panel);
        const blocking = findChild(panel, "blockingChoices");
        compare(blocking.choices.length, 2);
        compare(findChild(panel, "optionalChoices").choices.length, 1);
        findChild(blocking, "blockingChoices_1").click();
        compare(selector.currentIndex, 16);
        const spells = findChild(preview, "spellcastingsPreview");
        compare(spells.activeCasting.id, "bard");
        spells.selectRepertoireSpellRequested("bard", {quotaId: "bard-known", spellLevel: 1, spellId: "grease"});
        selector.currentIndex = 22;
        compare(findChild(panel, "blockingChoices").choices.length, 1);
        findChild(panel, "optionalChoices_0").click();
        compare(selector.currentIndex, 2);
        preview.close();
    }
}
