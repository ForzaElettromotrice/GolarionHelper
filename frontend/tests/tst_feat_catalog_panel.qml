import QtQuick
import QtTest
import Golarion.Sheet
import "../preview" as Preview

TestCase {
    id: testCase
    name: "FeatCatalogPanel"
    width: 600; height: 900
    visible: true
    when: windowShown
    Component { id: component; FeatCatalogPanel {} }
    Component { id: previewComponent; Preview.Main {} }
    SignalSpy { id: addSpy; signalName: "addRequested" }
    SignalSpy { id: extraSpy; signalName: "extraRequested" }
    function test_addToSelectedSlotOrExtra() {
        const catalog = [{id: "test", name: "Talento <test>", description: "Non visualizzata", types: []}];
        const panel = createTemporaryObject(component, testCase, {catalog: catalog, slotId: "level3", width: 280});
        addSpy.target = panel; addSpy.clear(); extraSpy.target = panel; extraSpy.clear();
        panel.open();
        tryCompare(panel, "opened", true);
        const list = findChild(panel.contentItem, "featCatalogList");
        list.forceLayout();
        compare(list.itemAtIndex(0).contentItem.textFormat, Text.PlainText);
        mouseClick(list.itemAtIndex(0));
        compare(addSpy.count, 1);
        compare(Array.from(addSpy.signalArguments[0]), ["level3", "test"]);
        tryCompare(panel, "visible", false);
        panel.slotId = "bonus";
        panel.open();
        tryCompare(panel, "opened", true);
        mouseClick(list.itemAtIndex(0));
        compare(Array.from(addSpy.signalArguments[1]), ["bonus", "test"]);
        panel.extra = true;
        panel.slotId = "";
        panel.open();
        tryCompare(panel, "opened", true);
        mouseClick(list.itemAtIndex(0));
        compare(extraSpy.count, 1);
        compare(Array.from(extraSpy.signalArguments[0]), ["test"]);
        compare(addSpy.count, 2);
        panel.extra = false;
        panel.open();
        tryCompare(panel, "opened", true);
        verify(!list.itemAtIndex(0).enabled);
        panel.close();
        compare(catalog.length, 1);
    }
    function test_searchSortAndCancel() {
        const catalog = [{id: "z", name: "Zeta"}, {id: "a", name: "Alfa"}];
        const original = JSON.stringify(catalog);
        const panel = createTemporaryObject(component, testCase, {catalog: catalog, slotId: "slot"});
        panel.open();
        tryCompare(panel, "opened", true);
        compare(panel.filtered[0].id, "a");
        const search = findChild(panel.contentItem, "featCatalogSearch");
        search.text = "  ZETA ";
        compare(panel.filtered.length, 1);
        compare(panel.filtered[0].id, "z");
        search.text = "missing";
        compare(panel.filtered.length, 0);
        compare(JSON.stringify(catalog), original);
        keyClick(Qt.Key_Escape);
        tryCompare(panel, "visible", false);
        panel.open();
        tryCompare(panel, "opened", true);
        compare(search.text, "");
        panel.catalog = null;
        compare(panel.filtered.length, 0);
        panel.close();
    }
    function test_largeCatalogVirtualized() {
        const panel = createTemporaryObject(component, testCase, {slotId: "slot", catalog: Array.from({length: 1700}, (_, i) => ({
            id: "feat" + i, name: "Talento " + String(i).padStart(4, "0")
        }))});
        panel.open();
        tryCompare(panel, "opened", true);
        compare(panel.filtered.length, 1700);
        const list = findChild(panel.contentItem, "featCatalogList");
        compare(list.count, 1700);
        verify(list.contentItem.children.length < 100);
        tryVerify(() => {
            list.positionViewAtIndex(1699, ListView.End);
            return list.itemAtIndex(1699) !== null;
        });
        panel.close();
    }
    function test_previewNavigation() {
        const preview = createTemporaryObject(previewComponent, testCase, {featCatalog: [
            {id: "example", name: "Esempio", description: "Descrizione", types: []}
        ]});
        findChild(preview, "panelSelector").currentIndex = 11;
        waitForRendering(preview.contentItem);
        mouseClick(findChild(preview.contentItem, "addExtraFeat"));
        const picker = preview.featPickerPopup;
        tryCompare(picker, "opened", true);
        verify(picker.extra);
        verify(findChild(preview.contentItem, "addExtraFeat").visible);
        keyClick(Qt.Key_Escape);
        tryCompare(picker, "visible", false);
        mouseClick(findChild(preview.contentItem, "selectFeat_level3"));
        tryCompare(picker, "opened", true);
        compare(picker.slotId, "level3");
        verify(!picker.extra);
        const list = findChild(picker.contentItem, "featCatalogList");
        list.forceLayout();
        mouseClick(list.itemAtIndex(0));
        tryCompare(picker, "visible", false);
        verify(findChild(preview.contentItem, "selectFeat_level3").visible);
    }
}
