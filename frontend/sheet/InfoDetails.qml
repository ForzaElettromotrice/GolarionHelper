import QtQuick
import QtQuick.Controls

Label {
    id: root
    enum LineType { Heading, Group, Entry, Total, Note, Spacer }
    required property var rows
    property bool framed: true
    property bool linkifyGolarion: false
    background: InfoFrame { visible: root.framed }
    textFormat: Text.StyledText
    wrapMode: Text.Wrap
    color: "#181818"
    padding: 14
    lineHeight: 1.3

    function escapeText(value) {
        return String(value).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    function formatText(value) {
        if (!linkifyGolarion) return escapeText(value);
        const pattern = /https?:\/\/(?:www\.)?golarion\.altervista\.org\/[^\s<>"']+/g;
        const source = String(value);
        let result = "", start = 0, match;
        while ((match = pattern.exec(source)) !== null) {
            const url = match[0].replace(/[.,;:!]+$/, "");
            result += escapeText(source.slice(start, match.index)) + '<a href="' + escapeText(url) + '">' + escapeText(url) + '</a>';
            start = match.index + url.length;
        }
        return result + escapeText(source.slice(start));
    }
    readonly property string summaryText: rows.map(row => " ".repeat(row.indent ?? 0) + row.text).join("\n")
    text: rows.map(row => {
        const value = formatText(row.text);
        const indent = "&nbsp;".repeat(row.indent ?? 0);
        switch (row.kind) {
        case InfoDetails.Heading: return '<b><font size="4">' + value + '</font></b>';
        case InfoDetails.Group: return '&nbsp;&nbsp;<font size="3" color="#666666"><i>' + value + '</i></font>';
        case InfoDetails.Entry: return indent + value;
        case InfoDetails.Total: return '<b>' + value + '</b>';
        case InfoDetails.Note: return '<font color="#666666"><i>' + value + '</i></font>';
        case InfoDetails.Spacer: return "";
        default: return value;
        }
    }).join("<br>")
}
