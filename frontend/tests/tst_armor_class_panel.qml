import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "ArmorClassPanel"
    width: 500; height: 1000
    visible: true
    when: windowShown
    Component { id: component; ArmorClassPanel {} }
    function sample() {
        return {maximumDexterityBonus: 2, maximumDexterityLimits: [{source: "Armatura <test>", effectiveValue: 2}],
            abilitySuppressions: [], abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Dexterity, abilityModifier: 4,
                values: [0, 1, 2].map(type => ({type: type, appliedAbilityModifier: type === 2 ? 0 : 2, totalValue: [19, 14, 17][type],
                    modifiers: {total: 0, modifiers: []}}))}]};
    }
    function test_valuesAndDetails() {
        const panel = createTemporaryObject(component, testCase, {armorClass: sample()});
        waitForRendering(panel);
        const row = findChild(panel, "armorClass0");
        compare(findChild(row, "score").text, "19");
        compare(findChild(findChild(panel, "armorClass1"), "score").text, "14");
        compare(findChild(findChild(panel, "armorClass2"), "score").text, "17");
        mouseClick(findChild(panel, "armorClassInfo"));
        const details = findChild(panel, "armorClassDetails");
        verify(details.visible);
        verify(details.text.includes("+2 Destrezza"));
        verify(details.text.includes("Totale: 19"));
        verify(details.text.includes("+2 Destrezza (Max 2)"));
        verify(!details.text.includes("Armatura &lt;test&gt;"));
        verify(!details.text.includes("Limite Destrezza"));
        verify(!panel.detailRows(ArmorClassPanel.FlatFooted).some(row => row.text.includes("(Max")));
        compare(details.width, panel.width);
        verify(details.text.includes("Contatto"));
        verify(details.text.includes("Impreparato"));
        verify(!details.text.includes("Modificatore applicato"));
        verify(!details.text.includes("originale:"));
        panel.width = 260;
        waitForRendering(panel);
        verify(details.width <= panel.width);
        verify(details.y >= findChild(panel, "armorClassInfo").y + findChild(panel, "armorClassInfo").height);
        verify(findChild(panel, "armorClass1").x < row.x);
        verify(findChild(panel, "armorClass2").x > row.x);
        const info = findChild(panel, "armorClassInfo");
        const position = info.mapToItem(row, 0, 0);
        verify(position.y > findChild(row, "score").mapToItem(row, 0, findChild(row, "score").height).y);
        verify(position.y + info.height < row.height);
        compare(position.x + info.width / 2, row.width / 2);
    }
    function test_alternativesAndMissing() {
        const data = sample();
        data.abilityOptions.push({source: "Alternativa", abilityType: AbilityScoresPanel.Charisma, abilityModifier: 5,
            values: [{type: 0, appliedAbilityModifier: 5, totalValue: 22, modifiers: {total: 0, modifiers: []}}]});
        const panel = createTemporaryObject(component, testCase, {armorClass: data});
        compare(findChild(findChild(panel, "armorClass0"), "score").text, "19 / 22");
        panel.armorClass = null;
        compare(findChild(findChild(panel, "armorClass0"), "score").text, "—");
        compare(panel.detailRows(0)[0].text, "Dati non disponibili.");
    }
}
