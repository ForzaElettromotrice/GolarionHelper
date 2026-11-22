import QtQuick
import QtTest
import "../preview" as Preview

TestCase {
    id: testCase
    name: "GrantedSpellsPanel"
    when: windowShown
    Component { id: previewComponent; Preview.Main {} }

    function test_previewActivatesByUsageMode() {
        const preview = createTemporaryObject(previewComponent, testCase);
        findChild(preview, "panelSelector").currentIndex = 17;
        const panel = findChild(preview, "grantedSpellsPreview");
        verify(panel.visible);
        function row(id) { return findChild(panel, "grantedSpell_" + id); }
        function button(id) { return findChild(row(id), "castGrantedSpellButton"); }
        function availability(id) { return findChild(row(id), "grantedSpellAvailability").text; }

        compare(availability("ancestry"), "1/1 utilizzi");
        button("ancestry").click();
        compare(availability("ancestry"), "0/1 utilizzi");
        verify(!button("ancestry").enabled);
        compare(availability("ability"), "A volontà");
        button("ability").click();
        verify(button("ability").enabled);
        compare(availability("amulet"), "Permanente");
        verify(!button("amulet").visible);
        compare(availability("staff"), "7 cariche");
        for (let cast = 0; cast < 3; ++cast) button("staff").click();
        compare(availability("staff"), "1 carica");
        verify(!button("staff").enabled);
        const selector = findChild(preview, "panelSelector");
        selector.currentIndex = 14;
        const inventory = findChild(preview, "inventoryPreview");
        waitForRendering(inventory);
        tryVerify(() => findChild(inventory, "inventoryInfo_staff") !== null);
        const info = findChild(inventory, "inventoryInfo_staff");
        info.click();
        const pool = () => findChild(inventory, "inventoryUsagePool_staff_bastone");
        const balance = () => findChild(pool(), "inventoryUsagePoolBalance").text;
        tryVerify(() => pool() !== null);
        compare(balance(), "Cariche (bastone): 1 / 10");
        findChild(pool(), "inventoryRestoreUsage").click();
        tryVerify(() => pool() !== null);
        compare(balance(), "Cariche (bastone): 2 / 10");
        findChild(pool(), "inventoryRefillUsage").click();
        selector.currentIndex = 17;
        compare(availability("staff"), "10 cariche");
        verify(button("staff").enabled);
        preview.close();
    }
}
