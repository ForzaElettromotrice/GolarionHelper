import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    name: "InfoDetails"
    InfoDetails {
        id: details
        rows: [
            {kind: InfoDetails.Heading, text: "Punteggio"},
            {kind: InfoDetails.Group, text: "Bonus di resistenza"},
            {kind: InfoDetails.Entry, indent: 4, text: "+2 <Mantello> & fonte"},
            {kind: InfoDetails.Total, text: "Totale: +2"},
            {kind: InfoDetails.Note, text: "Nota"}
        ]
    }
    function test_sharedStylesAndEscaping() {
        verify(details.framed);
        compare(details.background.color, "#f8fafb");
        compare(details.background.border.color, "#c6cfd5");
        compare(details.background.radius, 6);
        verify(details.text.includes('<b><font size="4">Punteggio</font></b>'));
        verify(details.text.includes('<font size="3" color="#666666"><i>Bonus di resistenza</i></font>'));
        verify(details.text.includes('&nbsp;&nbsp;&nbsp;&nbsp;+2 &lt;Mantello&gt; &amp; fonte'));
        verify(details.text.includes('<b>Totale: +2</b>'));
        verify(details.summaryText.includes("    +2 <Mantello> & fonte"));
        details.rows = [{kind: InfoDetails.Entry, text: "Punteggio"}];
        compare(details.text, "Punteggio");
    }
}
