import QtQuick
import QtTest
import Golarion.Sheet
import "../preview" as Preview

TestCase {
    id: testCase
    name: "EnhancementsPanel"
    when: windowShown
    Component { id: panelComponent; EnhancementsPanel {} }
    Component { id: previewComponent; Preview.Main {} }
    SignalSpy { id: activationSpy; signalName: "setEnhancementGroupActiveRequested" }

    function test_previewCreatesInactiveEmptyGroup() {
        const preview = createTemporaryObject(previewComponent, testCase);
        const panel = findChild(preview, "enhancementsPreview");
        const plus = findChild(panel, "addEnhancementGroupButton");
        const editor = findChild(panel, "enhancementGroupEditor");
        const name = findChild(editor.contentItem, "enhancementGroupName");
        const description = findChild(editor.contentItem, "enhancementGroupDescription");
        const create = findChild(editor.footer, "createEnhancementGroupButton");
        plus.click();
        tryCompare(editor, "opened", true);
        name.text = "  ";
        verify(!create.enabled);
        name.text = "Da scartare";
        editor.reject();
        compare(panel.enhancements.groups.length, 2);
        plus.click();
        tryCompare(editor, "opened", true);
        compare(name.text, "");
        name.text = "  Difesa  ";
        description.text = "  Nota di esempio  ";
        create.click();
        const group = panel.enhancements.groups[2];
        compare(group.name, "Difesa");
        compare(group.description, "Nota di esempio");
        verify(!group.active);
        verify(group.effectsEditable);
        compare(group.enhancements.length, 0);
        preview.close();
    }

    function test_referenceSeparatesModifiersAndContributions() {
        const preview = createTemporaryObject(previewComponent, testCase);
        findChild(preview, "panelSelector").currentIndex = 18;
        const panel = findChild(preview, "enhancementsPreview");
        findChild(panel, "enhancementReference_targets").click();
        compare(findChild(panel, "enhancementReferenceResource_@level").text, "@level — Livello totale del personaggio.");
        findChild(panel, "enhancementReference_" + EnhancementsPanel.Bonus).click();
        compare(findChild(panel, "enhancementReferenceResource_str").text, "str — Punteggio di Forza.");
        verify(!findChild(panel, "enhancementReferenceResource_hp.max"));
        findChild(panel, "enhancementReference_" + EnhancementsPanel.Contribution).click();
        compare(findChild(panel, "enhancementReferenceResource_hp.max").text, "hp.max — Punti ferita massimi.");
        preview.close();
    }

    function test_requestsActivationAndWaitsForSnapshot() {
        const snapshot = {groups: [{id: "bless", name: "Benedizione", active: false}]};
        const panel = createTemporaryObject(panelComponent, testCase, {enhancements: snapshot, width: 450});
        activationSpy.target = panel;
        activationSpy.clear();
        const toggle = () => findChild(panel, "enhancementToggle_bless");
        verify(!toggle().checked);
        toggle().click();
        compare(activationSpy.count, 1);
        compare(activationSpy.signalArguments[0][0], "bless");
        compare(activationSpy.signalArguments[0][1], true);
        verify(!toggle().checked);
        panel.enhancements = {groups: [{id: "bless", name: "Benedizione", active: true}]};
        verify(toggle().checked);
        toggle().click();
        compare(activationSpy.signalArguments[1][1], false);
        verify(toggle().checked);
        verify(!snapshot.groups[0].active);
    }

    function test_previewAddsEffectOnlyToEditableGroup() {
        const preview = createTemporaryObject(previewComponent, testCase);
        findChild(preview, "panelSelector").currentIndex = 18;
        const panel = findChild(preview, "enhancementsPreview");
        findChild(panel, "enhancementGroupHeader_bless").click();
        findChild(panel, "enhancementToggle_bless").click();
        verify(!findChild(panel, "addEnhancementEffect_bless").visible);
        findChild(panel, "enhancementGroupHeader_fatigue").click();
        findChild(panel, "addEnhancementEffect_fatigue").click();
        const editor = findChild(panel, "enhancementEffectEditor");
        tryCompare(editor, "opened", true);
        const confirm = findChild(editor.footer, "confirmEnhancementEffectButton");
        verify(!confirm.enabled);
        const suggestions = findChild(editor.contentItem, "enhancementResourceSuggestions");
        const search = findChild(editor.contentItem, "enhancementResourceSearch");
        const expression = findChild(editor.contentItem, "enhancementEffectExpression");
        verify(!expression.visible);
        verify(!suggestions.visible);
        search.text = " ORZ ";
        compare(suggestions.count, 1);
        verify(suggestions.visible);
        waitForRendering(suggestions);
        const strength = findChild(suggestions, "enhancementResourceOption_str");
        compare(strength.text, "Forza");
        verify(findChild(strength, "highlightedResourceText").text.includes(">orz</font>"));
        strength.click();
        compare(search.text, "Forza");
        verify(!suggestions.visible);
        verify(expression.visible);
        expression.text = "3";
        verify(confirm.enabled);
        confirm.click();
        tryCompare(editor, "opened", false);
        compare(panel.enhancements.groups[1].enhancements[2].definition.resource, "str");
        verify(findChild(panel, "enhancementGroupDetails_fatigue").summaryText.includes("+3 Forza"));
        findChild(panel, "addEnhancementEffect_fatigue").click();
        tryCompare(editor, "opened", true);
        compare(search.text, "");
        search.text = "p.M";
        compare(suggestions.count, 1);
        waitForRendering(suggestions);
        const hp = findChild(suggestions, "enhancementResourceOption_hp.max");
        verify(findChild(hp, "highlightedResourceText").text.includes(">p.m</font>"));
        hp.click();
        compare(search.text, "Punti ferita massimi");
        compare(findChild(editor.contentItem, "enhancementResourceKind").text, "Contributo");
        verify(!findChild(editor.contentItem, "enhancementModifierKind").visible);
        expression.text = "4";
        verify(confirm.enabled);
        confirm.click();
        const contribution = panel.enhancements.groups[1].enhancements[3].definition;
        compare(contribution.resource, "hp.max");
        compare(contribution.kind, EnhancementsPanel.Contribution);
        verify(contribution.bonusType === undefined);
        findChild(panel, "addEnhancementEffect_fatigue").click();
        tryCompare(editor, "opened", true);
        search.text = "nessuna-corrispondenza";
        compare(suggestions.count, 0);
        verify(!confirm.enabled);
        panel.effectResources = [
            {id: "z", name: "zFoo", kinds: [EnhancementsPanel.Bonus]},
            {id: "b", name: "<B>Foo", kinds: [EnhancementsPanel.Bonus]},
            {id: "a", name: "aFoo", kinds: [EnhancementsPanel.Bonus]},
            {id: "short", name: "foo", kinds: [EnhancementsPanel.Bonus]}
        ];
        search.text = "FOO";
        compare(suggestions.model.map(entry => entry.id), ["short", "a", "z", "b"]);
        waitForRendering(suggestions);
        const escaped = findChild(findChild(suggestions, "enhancementResourceOption_b"), "highlightedResourceText");
        verify(escaped.text.startsWith("&lt;B&gt;"));
        editor.reject();
        findChild(panel, "enhancementToggle_fatigue").click();
        verify(!findChild(panel, "addEnhancementEffect_fatigue").visible);
        preview.close();
    }

    function test_previewRemovesEffectsAndCustomGroups() {
        const preview = createTemporaryObject(previewComponent, testCase);
        findChild(preview, "panelSelector").currentIndex = 18;
        const panel = findChild(preview, "enhancementsPreview");
        findChild(panel, "enhancementGroupHeader_bless").click();
        verify(!findChild(panel, "removeEnhancementGroup_bless").visible);
        findChild(panel, "enhancementGroupHeader_fatigue").click();
        findChild(panel, "removeEnhancementEffect_strength-penalty").click();
        compare(findChild(panel, "enhancementGroupDetails_fatigue").summaryText, "−2 Destrezza");
        findChild(panel, "enhancementToggle_fatigue").click();
        verify(!findChild(panel, "removeEnhancementEffect_dexterity-penalty").visible);
        const dialog = findChild(panel, "removeEnhancementGroupDialog");
        findChild(panel, "removeEnhancementGroup_fatigue").click();
        tryCompare(dialog, "opened", true);
        dialog.reject();
        compare(panel.enhancements.groups.length, 2);
        findChild(panel, "removeEnhancementGroup_fatigue").click();
        tryCompare(dialog, "opened", true);
        findChild(dialog.footer, "confirmRemoveEnhancementGroup").click();
        compare(panel.enhancements.groups.map(group => group.id), ["bless"]);
        preview.close();
    }

    function test_previewUpdatesOnlySelectedGroup() {
        const preview = createTemporaryObject(previewComponent, testCase);
        findChild(preview, "panelSelector").currentIndex = 18;
        const panel = findChild(preview, "enhancementsPreview");
        verify(panel.visible);
        waitForRendering(panel);
        const initialX = findChild(panel, "enhancementToggle_fatigue").x;
        const header = () => findChild(panel, "enhancementGroupHeader_fatigue");
        const details = () => findChild(panel, "enhancementGroupDetails_fatigue");
        verify(!details().visible);
        header().click();
        verify(details().visible);
        compare(details().summaryText, "−2 Forza\n−2 Destrezza");
        verify(!findChild(panel, "enhancementToggle_fatigue").checked);
        findChild(panel, "enhancementToggle_fatigue").click();
        waitForRendering(panel);
        verify(findChild(panel, "enhancementToggle_fatigue").checked);
        verify(details().visible);
        compare(findChild(panel, "enhancementToggle_fatigue").x, initialX);
        verify(findChild(panel, "enhancementToggle_bless").checked);
        findChild(panel, "enhancementToggle_bless").click();
        verify(!findChild(panel, "enhancementToggle_bless").checked);
        findChild(panel, "enhancementGroupHeader_bless").click();
        verify(findChild(panel, "enhancementGroupDetails_bless").summaryText.includes("Effetti in sola lettura."));
        findChild(panel, "enhancementToggle_fatigue").click();
        verify(!findChild(panel, "enhancementToggle_fatigue").checked);
        header().click();
        verify(!details().visible);
        preview.close();
    }
}
