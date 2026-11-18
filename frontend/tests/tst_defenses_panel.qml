import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "DefensesPanel"
    width: 500; height: 1000
    visible: true
    when: windowShown
    Component { id: component; DefensesPanel {} }
    function test_emptyAndUpdates() {
        const panel = createTemporaryObject(component, testCase, {defenses: {}});
        waitForRendering(panel);
        compare(panel.sections.length, 0);
        compare(panel.implicitHeight, 0);
        panel.defenses = {energyResistances: [{energy: DefensesPanel.Fire, effectiveValue: 17}]};
        compare(panel.sections.length, 1);
        compare(panel.sections[0].entries[0], "Fuoco 17");
        const energies = [DefensesPanel.Acid, DefensesPanel.Cold, DefensesPanel.Electricity, DefensesPanel.Fire,
            DefensesPanel.Sonic, DefensesPanel.Force, DefensesPanel.PositiveEnergy, DefensesPanel.NegativeEnergy];
        compare(new Set(energies.map(type => panel.energyPalette(type).fill)).size, energies.length);
        const originalWidth = panel.badgeWidth;
        panel.defenses = {immunities: [{name: "Affaticamento", grants: []}]};
        verify(panel.badgeWidth > originalWidth);
        const immunityWidth = panel.badgeWidth;
        panel.presentation = DefensesPanel.Shields;
        compare(panel.badgeWidth, immunityWidth);
    }
    function test_damageReductionsDescendingWithoutMutatingSnapshot() {
        const reductions = [
            {effectiveValue: 5, bypass: {anyOf: []}},
            {effectiveValue: 10, bypass: {anyOf: [{allOf: [DefensesPanel.Silver]}]}},
            {effectiveValue: 5, bypass: {anyOf: [{allOf: [DefensesPanel.Magic]}]}}
        ];
        const panel = createTemporaryObject(component, testCase, {defenses: {damageReductions: reductions}});
        compare(panel.sections[0].entries, ["10/argento", "5/—", "5/magia"]);
        compare(reductions.map(entry => entry.effectiveValue), [5, 10, 5]);
    }
    function test_conditionsAndBypass() {
        const panel = createTemporaryObject(component, testCase, {width: 260, defenses: {
            immunities: [{name: "Veleno", grants: [{applicability: "Solo a contatto"}]}],
            damageReductions: [{effectiveValue: 5, bypass: {anyOf: [{allOf: [DefensesPanel.Silver, DefensesPanel.Magic]}, {allOf: [DefensesPanel.Good]}]}, applicability: "Contro non morti"}],
            energyVulnerabilities: [{energy: DefensesPanel.Cold, damageIncreasePercent: 50, grants: []}]
        }});
        compare(panel.sections.length, 3);
        compare(panel.sections[0].entries[0], "Veleno · Solo a contatto");
        verify(panel.sections[0].joinedList);
        verify(!panel.sections[0].inlineList);
        compare(panel.sections[1].entries[0], "5/(argento e magia) o bene · Contro non morti");
        compare(panel.sections[2].entries[0], "Freddo");
        verify(panel.sections[2].joinedList);
        panel.presentation = DefensesPanel.Tags;
        compare(panel.badges.length, 2);
        compare(panel.badges[0].kind, DefensesPanel.Immunity);
        compare(panel.badges[1].kind, DefensesPanel.Vulnerability);
        panel.presentation = DefensesPanel.Shields;
        compare(panel.badges.length, 1);
        compare(panel.badges[0].kind, DefensesPanel.Reduction);
        compare(panel.bypassText({anyOf: []}), "—");
        compare(panel.contexts([{applicability: "Solo a contatto"}, {applicability: null}]), "");
        waitForRendering(panel);
        verify(panel.implicitHeight > 0);
    }
}
