import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "SpellcastingsPanel"
    width: 700; height: 500
    visible: true
    when: windowShown
    Component { id: panelComponent; SpellcastingsPanel {} }
    SignalSpy { id: preparationSpy; signalName: "prepareSpellRequested" }
    SignalSpy { id: consumptionSpy; signalName: "consumeSpellRequested" }
    SignalSpy { id: clearSpy; signalName: "clearSpellSlotRequested" }
    SignalSpy { id: resetSpy; signalName: "resetSpellSlotsRequested" }
    SignalSpy { id: spontaneousSpy; signalName: "castSpontaneousSpellRequested" }
    function test_spontaneousDragCastsOrPromptsForMetamagic() {
        const first = {groupId: "base", spellLevel: 1, index: 0};
        const second = {groupId: "base", spellLevel: 2, index: 0};
        const applications = [{type: SpellcastingsPanel.Still, grantId: "still"}];
        const bard = {id: "bard", mode: SpellcastingsPanel.Spontaneous, repertoire: ["sleep"], state: {groups: [{slots: [
            {slot: first, state: SpellcastingsPanel.CastSlotFree},
            {slot: {groupId: "base", spellLevel: 1, index: 1}, state: SpellcastingsPanel.CastSlotUsed},
            {slot: second, state: SpellcastingsPanel.CastSlotFree}
        ]}]}};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [bard]},
            canCastSpontaneousSpell: () => true,
            castingMetamagicOptions: (casting, entry) => entry.slot.spellLevel === 2 ? [{applications: applications}] : []});
        spontaneousSpy.target = panel;
        spontaneousSpy.clear();
        waitForRendering(panel);
        const spell = findChild(panel, "repertoireSpell_sleep");
        const picker = findChild(panel, "preparationMetamagicPicker");
        function dropOn(name) {
            const destination = findChild(panel, name);
            mousePress(spell, 10, 10);
            mouseMove(spell, 40, 10);
            mouseMove(destination, 20, 12);
            mouseRelease(destination, 20, 12);
        }
        dropOn("spellSlot_1_1");
        compare(spontaneousSpy.count, 0);
        dropOn("spellSlot_1_0");
        compare(spontaneousSpy.count, 1);
        compare(spontaneousSpy.signalArguments[0][1], first);
        compare(spontaneousSpy.signalArguments[0][2], "sleep");
        compare(spontaneousSpy.signalArguments[0][3], []);
        dropOn("spellSlot_2_0");
        tryCompare(picker, "opened", true);
        compare(spontaneousSpy.count, 1);
        findChild(picker.contentItem, "preparationMetamagicChoice_0").click();
        picker.accept();
        compare(spontaneousSpy.count, 2);
        compare(spontaneousSpy.signalArguments[1][1], second);
        compare(spontaneousSpy.signalArguments[1][3], applications);
    }
    function test_preparationAutomaticallyPicksSameLevelRegularSlot() {
        const target = {groupId: "base", spellLevel: 1, index: 0};
        const regular = {groupId: "base", spellLevel: 1, index: 1};
        const restricted = {groupId: "restricted", spellLevel: 1, index: 0};
        const free = slot => ({slot: slot, state: SpellcastingsPanel.Free});
        const wizard = {id: "wizard", mode: SpellcastingsPanel.Prepared, repertoire: ["shield"], state: {groups: [
            {grant: {allowedSpellIds: ["shield"]}, slots: [free(restricted)]},
            {slots: [free(target), free(regular), free({groupId: "base", spellLevel: 2, index: 0})]}
        ]}};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [wizard]},
            canPrepareSpell: () => true, preparationSlotCount: () => 2});
        preparationSpy.target = panel;
        preparationSpy.clear();
        waitForRendering(panel);
        const spell = findChild(panel, "repertoireSpell_shield");
        const destination = findChild(panel, "spellSlot_1_1");
        mousePress(spell, 10, 10);
        mouseMove(spell, 40, 10);
        mouseMove(destination, 20, 12);
        mouseRelease(destination, 20, 12);
        tryCompare(preparationSpy, "count", 1);
        compare(preparationSpy.signalArguments[0][1], [target, regular]);
        compare(panel.preparationSlots({groupId: "base", spellLevel: 2, index: 0}, "shield", []), null);
    }
    function test_conversionReloadsMetamagicForReplacement() {
        const slot = {groupId: "base", spellLevel: 1, index: 0};
        const cleric = {id: "cleric", mode: SpellcastingsPanel.Prepared, state: {groups: [{slots: [
            {slot: slot, spellId: "bless", state: SpellcastingsPanel.Ready}
        ]}]}};
        const applications = [{type: SpellcastingsPanel.Silent, grantId: "replacement-silent"}];
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [cleric]},
            conversionSpellOptions: () => ["cure"], castingMetamagicOptions: (casting, entry, replacement) =>
                replacement === "cure" ? [{applications: applications}] : []});
        consumptionSpy.target = panel;
        consumptionSpy.clear();
        waitForRendering(panel);
        findChild(panel, "consumeSpellButton").click();
        const picker = findChild(panel, "preparationMetamagicPicker");
        tryCompare(picker, "opened", true);
        verify(!findChild(picker.contentItem, "preparationMetamagicChoice_0"));
        const selector = findChild(picker.contentItem, "conversionSpellChoice");
        selector.currentIndex = 1;
        selector.activated(1);
        findChild(picker.contentItem, "preparationMetamagicChoice_0").click();
        picker.accept();
        compare(consumptionSpy.count, 1);
        compare(consumptionSpy.signalArguments[0][1], slot);
        compare(consumptionSpy.signalArguments[0][2], applications);
        compare(consumptionSpy.signalArguments[0][3], "cure");
    }
    function test_limitedSlotDetails() {
        const wizard = {id: "wizard", mode: SpellcastingsPanel.Prepared, state: {groups: [
            {grant: {allowedSpellIds: null}, slots: [{slot: {spellLevel: 1}, state: SpellcastingsPanel.Free}]},
            {grant: {allowedSpellIds: ["shield"]}, slots: [
                {slot: {spellLevel: 1}, state: SpellcastingsPanel.Free},
                {slot: {spellLevel: 1}, state: SpellcastingsPanel.Ready, spellId: "shield"}
            ]},
            {grant: {allowedSpellIds: []}, slots: [{slot: {spellLevel: 1}, state: SpellcastingsPanel.Free}]}
        ]}};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [wizard]},
            spellNames: {shield: "Scudo"}});
        waitForRendering(panel);
        for (const [index, name] of ["Slot 1", "Slot 2 · Limited", "Scudo · Limited", "Slot 4 · Limited"].entries()) {
            const card = findChild(panel, "spellSlot_1_" + index);
            const label = findChild(card, "slotSpellLabel");
            compare(label.text, name);
            compare(label.width, label.implicitWidth);
            const info = findChild(card, "slotRestrictionInfo");
            compare(info.visible, index !== 0);
            if (!info.visible) continue;
            const details = findChild(card, "slotRestrictions");
            verify(!details.visible);
            if (index === 1) {
                mouseMove(findChild(panel, "spellRepertoire"), 10, 8);
                tryCompare(info, "opacity", 0);
                mouseMove(card, 10, 8);
                tryCompare(info, "opacity", 1);
            }
            info.click();
            verify(details.visible);
            compare(details.rows[1].text, index === 3 ? "Nessun incantesimo ammesso." : "Scudo");
            info.click();
            verify(!details.visible);
        }
    }
    function test_preparedSlotActions() {
        const slot = {groupId: "base", spellLevel: 1, index: 0};
        const wizard = {id: "wizard", mode: SpellcastingsPanel.Prepared, repertoire: ["shield"],
            state: {groups: [{slots: [{slot: slot, spellId: "shield", state: SpellcastingsPanel.Ready}]}]}};
        const applications = [{type: SpellcastingsPanel.Silent, grantId: "silent"}];
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [wizard]},
            castingMetamagicOptions: () => [{applications: applications}]});
        consumptionSpy.target = clearSpy.target = resetSpy.target = panel;
        consumptionSpy.clear();
        clearSpy.clear();
        resetSpy.clear();
        waitForRendering(panel);
        const card = findChild(panel, "spellSlot_1_0");
        const castButton = findChild(card, "consumeSpellButton");
        const picker = findChild(panel, "preparationMetamagicPicker");
        castButton.click();
        tryCompare(picker, "opened", true);
        compare(consumptionSpy.count, 0);
        findChild(picker.contentItem, "preparationMetamagicChoice_0").click();
        picker.accept();
        tryCompare(picker, "opened", false);
        compare(consumptionSpy.count, 1);
        compare(consumptionSpy.signalArguments[0][2], applications);
        panel.castingMetamagicOptions = () => [];
        castButton.click();
        verify(!picker.opened);
        compare(consumptionSpy.count, 2);
        compare(consumptionSpy.signalArguments[1][2], []);

        const spell = findChild(card, "slotSpellLabel");
        const repertoire = findChild(panel, "spellRepertoire");
        mousePress(spell, 10, 8);
        mouseMove(spell, 35, 8);
        mouseMove(repertoire, 20, repertoire.height - 10);
        tryCompare(panel, "removingPreparation", true);
        mouseRelease(repertoire, 20, repertoire.height - 10);
        tryCompare(clearSpy, "count", 1);
        compare(clearSpy.signalArguments[0][1], slot);
        findChild(panel, "resetSpellSlotsButton").click();
        compare(resetSpy.count, 1);
        compare(resetSpy.signalArguments[0][0], "wizard");
    }
    function test_dragPreparedSpellToCompatibleSlot() {
        const allowed = {groupId: "base", spellLevel: 1, index: 0};
        const blocked = {groupId: "restricted", spellLevel: 1, index: 0};
        const wizard = {id: "wizard", source: "Mago", mode: SpellcastingsPanel.Prepared, repertoire: ["shield"],
            state: {spellListClass: ClassesPanel.Wizard, groups: [{slots: [
                {slot: allowed, state: SpellcastingsPanel.Free}, {slot: blocked, state: SpellcastingsPanel.Free},
                {slot: {groupId: "base", spellLevel: 1, index: 1}, state: SpellcastingsPanel.Ready}
            ]}]}};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [wizard]},
            canPrepareSpell: (casting, slot, spellId) => casting.id === "wizard" && spellId === "shield" && slot.groupId === "base"});
        preparationSpy.target = panel;
        preparationSpy.clear();
        waitForRendering(panel);
        const spell = findChild(panel, "repertoireSpell_shield");
        const allowedCard = findChild(panel, "spellSlot_1_0");
        const blockedCard = findChild(panel, "spellSlot_1_1");
        const occupiedCard = findChild(panel, "spellSlot_1_2");
        for (const destination of [blockedCard, allowedCard]) {
            mousePress(spell, 10, 10);
            mouseMove(spell, 40, 10);
            mouseMove(destination, 20, 12);
            tryCompare(allowedCard, "acceptsPreparation", true);
            compare(blockedCard.opacity, 0.38);
            compare(occupiedCard.opacity, 0.38);
            mouseRelease(destination, 20, 12);
            tryCompare(preparationSpy, "count", destination === allowedCard ? 1 : 0);
            compare(blockedCard.opacity, 1);
        }
        const request = preparationSpy.signalArguments[0];
        compare(request[0], "wizard");
        compare(request[1], [allowed]);
        compare(request[2], "shield");
        compare(request[3], []);
        compare(wizard.state.groups[0].slots[0].state, SpellcastingsPanel.Free);
    }
    function test_dropPromptsOnlyForAvailableMetamagic() {
        const slot = {groupId: "base", spellLevel: 2, index: 0};
        const wizard = {id: "wizard", mode: SpellcastingsPanel.Prepared, repertoire: ["shield"],
            state: {groups: [{slots: [{slot: slot, state: SpellcastingsPanel.Free}]}]}};
        const applications = [{type: SpellcastingsPanel.Silent, grantId: "silent"}];
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [wizard]},
            canPrepareSpell: () => true, preparationMetamagicOptions: () => [
                {applications: applications}, {applications: [{type: SpellcastingsPanel.Still, grantId: "still"}]}
            ]});
        preparationSpy.target = panel;
        preparationSpy.clear();
        waitForRendering(panel);
        const spell = findChild(panel, "repertoireSpell_shield");
        const destination = findChild(panel, "spellSlot_2_0");
        const picker = findChild(panel, "preparationMetamagicPicker");
        for (const confirm of [false, true]) {
            mousePress(spell, 10, 10);
            mouseMove(spell, 40, 10);
            mouseMove(destination, 20, 12);
            mouseRelease(destination, 20, 12);
            tryCompare(picker, "opened", true);
            compare(preparationSpy.count, 0);
            if (confirm) {
                const first = findChild(picker.contentItem, "preparationMetamagicChoice_0");
                const second = findChild(picker.contentItem, "preparationMetamagicChoice_1");
                verify(second.enabled);
                first.click();
                verify(first.checked);
                verify(!second.enabled);
                compare(second.opacity, 0.45);
                first.click();
                verify(!first.checked);
                verify(second.enabled);
                first.click();
                picker.accept();
            } else {
                picker.reject();
            }
            tryCompare(picker, "opened", false);
        }
        compare(preparationSpy.count, 1);
        compare(preparationSpy.signalArguments[0][3], applications);
    }
    function test_repertoireUsesSelectedClassLevels() {
        const wizard = {id: "wizard", state: {spellListClass: ClassesPanel.Wizard}, repertoire: ["shared", "unknown", "cantrip"]};
        const bard = {id: "bard", state: {spellListClass: ClassesPanel.Bard}, repertoire: ["shared"]};
        const levels = {};
        levels[ClassesPanel.Wizard] = {shared: 2, cantrip: 0};
        levels[ClassesPanel.Bard] = {shared: 1};
        const panel = createTemporaryObject(panelComponent, testCase, {spellcastings: {castings: [wizard, bard]}, spellLevelsByClass: levels});
        const wizardGroups = panel.repertoireLevels(wizard);
        compare(wizardGroups.map(group => group.level), [0, 2, -1]);
        compare(wizardGroups[1].spells, ["shared"]);
        compare(panel.repertoireLevels(bard)[0].level, 1);
    }
    function test_selectCastingWithoutMixingSlots() {
        const wizard = {id: "wizard", source: "Mago", mode: SpellcastingsPanel.Prepared, state: {groups: [{slots: [
            {slot: {spellLevel: 1}, state: SpellcastingsPanel.Ready}, {slot: {spellLevel: 1}, state: SpellcastingsPanel.Used}
        ]}]}};
        const bard = {id: "bard", source: "Bardo", mode: SpellcastingsPanel.Spontaneous, state: {groups: [{slots: [
            {slot: {spellLevel: 2}, state: SpellcastingsPanel.CastSlotFree}
        ]}]}};
        const panel = createTemporaryObject(panelComponent, testCase, {width: 700, spellcastings: {castings: [wizard, bard]}});
        waitForRendering(panel);
        const selector = findChild(panel, "spellcastingSelector");
        verify(selector.visible);
        compare(findChild(panel, "spellSlotCount_1").text, "1 / 2 disponibili");
        verify(findChild(panel, "spellSlot_1_0") !== null);
        verify(findChild(panel, "spellSlot_1_1") !== null);
        verify(findChild(panel, "spellRepertoire").x < findChild(panel, "spellSlotGroups").x);
        verify(findChild(panel, "spellSlotCount_2") === null);
        selector.currentIndex = 1;
        selector.activated(1);
        compare(findChild(panel, "activeSpellcastingName").text, "Bardo");
        compare(findChild(panel, "spellSlotCount_2").text, "1 / 1 disponibili");
        verify(findChild(panel, "spellSlotCount_1") === null);
        verify(findChild(panel, "spellSlot_1_0") === null);
        panel.spellcastings = {castings: [wizard]};
        verify(!selector.visible);
        compare(findChild(panel, "activeSpellcastingName").text, "Mago");
        panel.spellcastings = {castings: []};
        compare(panel.activeCasting, null);
    }
}
