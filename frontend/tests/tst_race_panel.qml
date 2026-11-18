import QtQuick
import QtTest
import Golarion.Sheet
import "../preview" as Preview

TestCase {
    id: testCase
    name: "RacePanel"
    width: 500; height: 900
    visible: true
    when: windowShown
    Component { id: component; RacePanel {} }
    Component { id: previewComponent; Preview.Main {} }
    function test_previewUsesSelectedRaceCatalog() {
        const preview = createTemporaryObject(previewComponent, testCase, {raceAlternateCatalog: {
            human: [{id: "human.alt", name: "Alternativo umano"}], elf: [{id: "elf.alt", name: "Alternativo elfico"}]
        }});
        findChild(preview, "panelSelector").currentIndex = 12;
        waitForRendering(preview.contentItem);
        const panel = findChild(preview.contentItem, "racePanel");
        const button = findChild(panel, "selectAlternateTraits");
        mouseClick(button);
        const popup = button.pickerPopup;
        tryCompare(popup, "opened", true);
        const list = findChild(popup.contentItem, "alternateTraitList");
        compare(list.count, 1);
        compare(list.model[0].id, "human.alt");
        panel.race = {race: {id: "elf", name: "Elfo"}, activeElements: []};
        tryCompare(popup, "visible", false);
        mouseClick(button);
        tryCompare(popup, "opened", true);
        compare(list.count, 1);
        compare(list.model[0].id, "elf.alt");
        popup.close();
    }
    SignalSpy { id: changeSpy; signalName: "setRaceRequested" }
    SignalSpy { id: choiceSpy; signalName: "changeChoiceRequested" }
    function test_completeMissingChoice() {
        const snapshot = {race: {id: "human", name: "Umano"}, activeElements: [{id: "alternate", name: "Doppia Dote",
            description: "Descrizione", choices: [], missingChoices: [{id: "abilities", prompt: "Scegli due", selectionCount: 2,
                blocksAdvancement: true, options: [{id: "strength", name: "Forza"}, {id: "constitution", name: "Costituzione"},
                    {id: "dexterity", name: "Destrezza"}]}]
        }]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {race: snapshot, width: 320});
        choiceSpy.target = panel; choiceSpy.clear();
        waitForRendering(panel);
        const button = findChild(panel, "completeRacialChoice_alternate_abilities");
        verify(button.enabled);
        mouseClick(button);
        const popup = button.pickerPopup;
        tryCompare(popup, "opened", true);
        compare(popup.title, "Completa scelta");
        waitForRendering(panel);
        findChild(popup.contentItem, "racialChoiceList").forceLayout();
        const strength = findChild(popup.contentItem, "racialChoiceOption_strength");
        const constitution = findChild(popup.contentItem, "racialChoiceOption_constitution");
        const confirm = findChild(popup.footer, "confirmRacialChoice");
        verify(!strength.checked && !constitution.checked);
        verify(!confirm.enabled);
        mouseClick(strength);
        verify(!confirm.enabled);
        mouseClick(constitution);
        verify(confirm.enabled);
        compare(choiceSpy.count, 0);
        mouseClick(confirm);
        compare(choiceSpy.count, 1);
        compare(choiceSpy.signalArguments[0][0], "alternate");
        compare(choiceSpy.signalArguments[0][1], "abilities");
        compare(Array.from(choiceSpy.signalArguments[0][2]), ["strength", "constitution"]);
        tryCompare(popup, "visible", false);
        compare(JSON.stringify(snapshot), original);
        mouseClick(button);
        tryCompare(popup, "opened", true);
        verify(!strength.checked && !constitution.checked);
        keyClick(Qt.Key_Escape);
        tryCompare(popup, "visible", false);
        compare(choiceSpy.count, 1);
    }
    function test_changeCompletedChoice() {
        const snapshot = {race: {id: "human", name: "Umano"}, activeElements: [{
            id: "alternate", name: "Doppia Dote", removable: true, description: "Descrizione",
            choices: [{id: "abilities", prompt: "Caratteristiche scelte", selectedOptions: [
                {id: "strength", name: "Forza"}, {id: "constitution", name: "Costituzione"}
            ], selectionCount: 2, options: [{id: "strength", name: "Forza"}, {id: "constitution", name: "Costituzione"},
                {id: "dexterity", name: "Destrezza"}]}], missingChoices: []
        }]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {race: snapshot, width: 280});
        choiceSpy.target = panel; choiceSpy.clear();
        removeSpy.target = panel; removeSpy.clear();
        waitForRendering(panel);
        compare(findChild(panel, "racialSelected_alternate_abilities").text, "Forza · Costituzione");
        compare(panel.pending.length, 0);
        verify(!findChild(panel, "racialPending_alternate").visible);
        const button = findChild(panel, "changeRacialChoice_alternate_abilities");
        verify(button.visible);
        verify(button.mapToItem(panel, button.width, 0).x <= panel.width);
        mouseClick(button);
        const popup = button.pickerPopup;
        tryCompare(popup, "opened", true);
        compare(choiceSpy.count, 0);
        waitForRendering(panel);
        findChild(popup.contentItem, "racialChoiceList").forceLayout();
        tryVerify(() => findChild(popup.contentItem, "racialChoiceOption_dexterity") !== null);
        const strength = findChild(popup.contentItem, "racialChoiceOption_strength");
        const constitution = findChild(popup.contentItem, "racialChoiceOption_constitution");
        const dexterity = findChild(popup.contentItem, "racialChoiceOption_dexterity");
        const confirm = findChild(popup.footer, "confirmRacialChoice");
        verify(strength.checked && constitution.checked);
        verify(!dexterity.enabled);
        mouseClick(strength);
        verify(!confirm.enabled);
        verify(dexterity.enabled);
        mouseClick(dexterity);
        verify(confirm.enabled);
        mouseClick(confirm);
        compare(choiceSpy.count, 1);
        compare(choiceSpy.signalArguments[0][0], "alternate");
        compare(choiceSpy.signalArguments[0][1], "abilities");
        compare(Array.from(choiceSpy.signalArguments[0][2]), ["constitution", "dexterity"]);
        tryCompare(popup, "visible", false);
        compare(removeSpy.count, 0);
        compare(JSON.stringify(snapshot), original);
        mouseClick(button);
        tryCompare(popup, "opened", true);
        verify(strength.checked && constitution.checked);
        keyClick(Qt.Key_Escape);
        tryCompare(popup, "visible", false);
        compare(choiceSpy.count, 1);
    }
    SignalSpy { id: alternateSpy; signalName: "selectAlternateTraitRequested" }
    SignalSpy { id: removeSpy; signalName: "removeAlternateTraitRequested" }
    function test_removeAlternatesOnly() {
        const snapshot = {race: {id: "human", name: "Umano"}, activeElements: [
            {id: "standard", name: "Standard", description: "Non rimovibile", removable: false},
            {id: "alternate", name: "Alternativo", description: "Rimovibile", removable: true},
            {id: "unspecified", name: "Altro", description: "Non dedurre dal nome"}
        ]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {race: snapshot, width: 280});
        removeSpy.target = panel; removeSpy.clear();
        waitForRendering(panel);
        verify(!findChild(panel, "removeRacialTrait_standard").visible);
        verify(!findChild(panel, "removeRacialTrait_unspecified").visible);
        const button = findChild(panel, "removeRacialTrait_alternate");
        verify(button.visible);
        const tag = findChild(panel, "racialAlternateLabel_alternate");
        verify(tag.visible);
        compare(tag.text, "Tratto Alternativo");
        verify(tag.font.bold);
        verify(tag.font.pixelSize < findChild(panel, "racialTitle_alternate").font.pixelSize);
        verify(!findChild(panel, "racialAlternateLabel_standard").visible);
        verify(!findChild(panel, "racialAlternateLabel_unspecified").visible);
        verify(button.mapToItem(panel, button.width, 0).x <= panel.width);
        mouseClick(button);
        compare(removeSpy.count, 1);
        compare(Array.from(removeSpy.signalArguments[0]), ["alternate"]);
        compare(JSON.stringify(snapshot), original);
        panel.race = {race: snapshot.race, activeElements: [snapshot.activeElements[0]]};
        verify(findChild(panel, "removeRacialTrait_alternate") === null);
        verify(!findChild(panel, "removeRacialTrait_standard").visible);
    }
    function test_alternateTraitsRequest() {
        const snapshot = {race: {id: "human", name: "Umano"}, activeElements: [
            {id: "selected", name: "Scelto", removable: true, description: "Tratto"}
        ]};
        const catalog = [{id: "selected", name: "Scelto"}, {id: "new", name: "Alternativo <test>"}];
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {race: snapshot, alternateFeatures: catalog, width: 280});
        alternateSpy.target = panel; alternateSpy.clear();
        waitForRendering(panel);
        const button = findChild(panel, "selectAlternateTraits");
        verify(button.enabled);
        verify(button.mapToItem(panel, button.width, 0).x <= panel.width);
        mouseClick(button);
        const popup = button.pickerPopup;
        tryCompare(popup, "opened", true);
        const list = findChild(popup.contentItem, "alternateTraitList");
        const search = findChild(popup.contentItem, "alternateTraitSearch");
        compare(list.count, 2);
        compare(list.model[0].id, "new");
        compare(alternateSpy.count, 0);
        search.text = "SCELTO";
        waitForRendering(panel);
        list.forceLayout();
        verify(!list.itemAtIndex(0).enabled);
        search.text = "alternativo";
        waitForRendering(panel);
        list.forceLayout();
        compare(list.itemAtIndex(0).contentItem.textFormat, Text.PlainText);
        mouseClick(list.itemAtIndex(0));
        compare(alternateSpy.count, 1);
        compare(Array.from(alternateSpy.signalArguments[0]), ["new"]);
        tryCompare(popup, "visible", false);
        compare(panel.race.race.id, "human");
        compare(JSON.stringify(snapshot), original);
        mouseClick(button);
        tryCompare(popup, "opened", true);
        compare(search.text, "");
        keyClick(Qt.Key_Escape);
        tryCompare(popup, "visible", false);
        compare(alternateSpy.count, 1);
        mouseClick(button);
        tryCompare(popup, "opened", true);
        panel.race = null;
        tryCompare(popup, "visible", false);
        verify(!button.enabled);
    }
    function test_racePicker() {
        const snapshot = {race: {id: "human", name: "Umano"}, activeElements: []};
        const catalog = [{id: "human", name: "Umano"}, {id: "elf", name: "Elfo <test>"}, {id: "dwarf", name: "Nano"}];
        const original = JSON.stringify(catalog);
        const panel = createTemporaryObject(component, testCase, {race: snapshot, catalog: catalog, width: 280});
        changeSpy.target = panel; changeSpy.clear();
        waitForRendering(panel);
        mouseClick(findChild(panel, "changeRace"));
        const popup = findChild(panel, "changeRace").pickerPopup;
        // Dialog content is reparented to the window overlay when opened.
        verify(popup !== null);
        tryCompare(popup, "opened", true);
        const list = findChild(popup.contentItem, "raceCatalogList");
        const search = findChild(popup.contentItem, "raceSearch");
        compare(list.count, 3);
        compare(list.model[0].id, "elf");
        search.text = " UMANO ";
        compare(list.count, 1);
        waitForRendering(panel);
        list.forceLayout();
        verify(!list.itemAtIndex(0).enabled);
        search.text = "ELFO";
        compare(list.count, 1);
        waitForRendering(panel);
        list.forceLayout();
        const candidate = list.itemAtIndex(0);
        compare(candidate.contentItem.textFormat, Text.PlainText);
        mouseClick(candidate);
        compare(changeSpy.count, 1);
        compare(Array.from(changeSpy.signalArguments[0]), ["elf"]);
        tryCompare(popup, "visible", false);
        compare(snapshot.race.id, "human");
        compare(JSON.stringify(catalog), original);
        mouseClick(findChild(panel, "changeRace"));
        tryCompare(popup, "opened", true);
        compare(search.text, "");
        keyClick(Qt.Key_Escape);
        tryCompare(popup, "visible", false);
        compare(changeSpy.count, 1);
        panel.race = null;
        compare(findChild(panel, "changeRace").text, "Scegli razza");
    }
    function test_traitsAndChoices() {
        const snapshot = {race: {id: "test", name: "Razza <test>"}, activeElements: [
            {id: "first", name: "Tratto <test>", description: "Descrizione <test>",
                choices: [{id: "done", prompt: "Scelta effettuata", selectedOptions: [{id: "one", name: "Opzione scelta"}]}], missingChoices: []},
            {id: "second", name: "Da completare", description: "Descrizione", choices: [], missingChoices: [
                {id: "required", prompt: "Scegli due", selectionCount: 2, blocksAdvancement: true, options: [{id: "a", name: "Alfa"}, {id: "b", name: "Beta"}]},
                {id: "optional", prompt: "Facoltativa", selectionCount: 1, blocksAdvancement: false, options: []}
            ]}
        ]};
        const original = JSON.stringify(snapshot);
        const panel = createTemporaryObject(component, testCase, {race: snapshot, width: 260});
        waitForRendering(panel);
        compare(findChild(panel, "raceName").text, "Razza <test>");
        compare(findChild(panel, "raceName").textFormat, Text.PlainText);
        const description = findChild(panel, "racialDescription_first");
        verify(description.visible);
        compare(description.text, "Descrizione <test>");
        compare(description.textFormat, Text.PlainText);
        compare(findChild(panel, "racialSelected_first_done").text, "Opzione scelta");
        compare(findChild(panel, "racialTraitsGrid").columns, 1);
        compare(findChild(panel, "racialTrait_first").width, 260);
        verify(!findChild(panel, "racialPending_first").visible);
        verify(findChild(panel, "racialPending_second").visible);
        verify(findChild(panel, "racialChoice_second_required").visible);
        compare(findChild(panel, "racialChoiceCount_second_required").text, "Opzioni da scegliere: 2");
        verify(findChild(panel, "racialChoicePolicy_second_required").text.includes("Da completare prima"));
        verify(findChild(panel, "racialChoicePolicy_second_optional").text.includes("Non blocca"));
        compare(panel.pending.length, 2);
        panel.width = 800;
        waitForRendering(panel);
        compare(findChild(panel, "racialTraitsGrid").columns, 2);
        const first = findChild(panel, "racialTrait_first");
        const second = findChild(panel, "racialTrait_second");
        compare(first.width, second.width);
        compare(first.y, second.y);
        verify(second.x > first.x);
        verify(second.x + second.width <= panel.width);
        compare(findChild(panel, "racialChoice_second_required").width, 800);
        compare(JSON.stringify(snapshot), original);
    }
    function test_missingRaceAndEmptyTraits() {
        const panel = createTemporaryObject(component, testCase, {race: null});
        compare(findChild(panel, "raceName").text, "Nessuna razza selezionata");
        panel.race = {race: null, activeElements: [{id: "hidden", name: "Non mostrare"}]};
        verify(findChild(panel, "racialTrait_hidden") === null);
        panel.race = {race: {id: "empty", name: "Vuota"}, activeElements: []};
        compare(findChild(panel, "raceName").text, "Vuota");
    }
}
