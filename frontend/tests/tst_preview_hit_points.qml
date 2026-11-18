import QtQuick
import QtQuick.Controls
import QtTest
import "../preview" as Preview

TestCase {
    id: testCase
    name: "PreviewHitPoints"
    when: windowShown
    Component { id: previewComponent; Preview.Main {} }

    function test_backgroundCancelsWithoutLeavingWindow() {
        const preview = createTemporaryObject(previewComponent, testCase);
        verify(preview !== null);
        preview.show();
        findChild(preview, "panelSelector").currentIndex = 4;
        waitForRendering(preview.contentItem);
        const display = findChild(preview.contentItem, "currentAndMaximum");
        const input = findChild(preview.contentItem, "amount");
        const checkbox = findChild(preview.contentItem, "nonLethal");
        verify(display !== null);
        mouseClick(display);
        input.text = "-7";
        waitForRendering(preview.contentItem);
        mouseClick(checkbox);
        verify(input.visible);
        compare(input.text, "-7");
        mouseClick(preview.contentItem, preview.contentItem.width - 20, preview.contentItem.height - 20);
        tryCompare(input, "visible", false);
        compare(input.text, "");
        compare(display.text, "24 / 32");
        preview.close();
    }
}
