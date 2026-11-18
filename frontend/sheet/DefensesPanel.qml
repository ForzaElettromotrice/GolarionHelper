pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: root
    required property var defenses
    enum Presentation { All, Shields, Tags }
    property int presentation: DefensesPanel.All
    property int maximumItemsPerRow: 3
    enum DisplayKind { Immunity, Reduction, Resistance, Vulnerability }
    enum DamageType { Bludgeoning, Piercing, Slashing, Acid, Cold, Electricity, Fire, Sonic, Force, PositiveEnergy, NegativeEnergy, Untyped }
    enum BypassTrait { BludgeoningBypass, PiercingBypass, SlashingBypass, Magic, Epic, Adamantine, Silver, ColdIron, Good, Evil, Lawful, Chaotic }
    implicitWidth: 360
    implicitHeight: content.height
    FontMetrics { id: tagMetrics; font.pixelSize: 14 }
    readonly property real badgeWidth: Math.ceil(Math.max(100, ...(defenses?.immunities ?? []).map(entry => tagMetrics.advanceWidth(entry.name) + 37)))

    function energyPalette(type) {
        switch (type) {
        case DefensesPanel.Acid: return {fill: "#e6efcc", accent: "#4e621d"};
        case DefensesPanel.Cold: return {fill: "#e0f2fa", accent: "#266780"};
        case DefensesPanel.Electricity: return {fill: "#fcf1c7", accent: "#786013"};
        case DefensesPanel.Fire: return {fill: "#fbe4d9", accent: "#994326"};
        case DefensesPanel.Sonic: return {fill: "#e9e0f5", accent: "#68458b"};
        case DefensesPanel.Force: return {fill: "#f5e0ef", accent: "#8b426f"};
        case DefensesPanel.PositiveEnergy: return {fill: "#fff3df", accent: "#946016"};
        case DefensesPanel.NegativeEnergy: return {fill: "#e1e2ec", accent: "#484c70"};
        default: return {fill: "#edf1ee", accent: "#425649"};
        }
    }
    function energyName(type) {
        return [qsTr("Contundente"), qsTr("Perforante"), qsTr("Tagliente"), qsTr("Acido"), qsTr("Freddo"), qsTr("Elettricità"), qsTr("Fuoco"), qsTr("Sonoro"), qsTr("Forza"), qsTr("Energia positiva"), qsTr("Energia negativa"), qsTr("Senza tipo")][type] ?? qsTr("Non specificato");
    }
    function bypassText(bypass) {
        const names = [qsTr("contundente"), qsTr("perforante"), qsTr("tagliente"), qsTr("magia"), qsTr("epico"), qsTr("adamantio"), qsTr("argento"), qsTr("ferro freddo"), qsTr("bene"), qsTr("male"), qsTr("legge"), qsTr("caos")];
        return (bypass?.anyOf ?? []).map(alternative => {
            const terms = alternative.allOf.map(trait => names[trait] ?? qsTr("non specificato"));
            const text = terms.join(qsTr(" e "));
            return terms.length > 1 && bypass.anyOf.length > 1 ? "(" + text + ")" : text;
        }).join(qsTr(" o ")) || "—";
    }
    function contexts(grants) {
        if (!(grants?.length) || grants.some(grant => !grant.applicability)) return "";
        return [...new Set(grants.map(grant => grant.applicability))].join(qsTr(" oppure "));
    }
    function qualified(text, applicability) { return applicability ? text + " · " + applicability : text; }
    readonly property var sections: [
        {kind: DefensesPanel.Immunity, joinedList: true, title: qsTr("Immunità"), entries: (defenses?.immunities ?? []).map(entry => qualified(entry.name, contexts(entry.grants)))},
        {kind: DefensesPanel.Reduction, values: Array.from(defenses?.damageReductions ?? []).sort((left, right) => right.effectiveValue - left.effectiveValue), title: qsTr("Riduzione del danno · RD"), entries: Array.from(defenses?.damageReductions ?? []).sort((left, right) => right.effectiveValue - left.effectiveValue).map(entry => qualified(entry.effectiveValue + "/" + bypassText(entry.bypass), entry.applicability))},
        {kind: DefensesPanel.Resistance, values: defenses?.energyResistances ?? [], title: qsTr("Resistenze energetiche · RE"), entries: (defenses?.energyResistances ?? []).map(entry => energyName(entry.energy) + " " + entry.effectiveValue)},
        {kind: DefensesPanel.Vulnerability, joinedList: true, title: qsTr("Vulnerabilità"), entries: (defenses?.energyVulnerabilities ?? []).map(entry => qualified(energyName(entry.energy), contexts(entry.grants)))}
    ].filter(section => section.entries.length > 0)

    readonly property var badges: sections.reduce((result, section) => result.concat(section.entries.map((text, index) => ({
        kind: section.kind, text: text, entry: section.values?.[index]
    }))), []).filter(badge => presentation === DefensesPanel.All ||
        (presentation === DefensesPanel.Shields ? badge.entry !== undefined : badge.entry === undefined))
    Flow {
        id: content
        width: Math.min(root.width, root.maximumItemsPerRow * root.badgeWidth + Math.max(0, root.maximumItemsPerRow - 1) * spacing)
        spacing: 6
        Repeater {
            model: root.badges
            Item {
                id: badge
                required property var modelData
                readonly property bool shield: modelData.entry !== undefined
                readonly property bool reduction: modelData.kind === DefensesPanel.Reduction
                width: Math.min(root.badgeWidth, content.width)
                height: shield ? defenseShield.implicitHeight : Math.max(38, tag.implicitHeight)
                DefenseShield {
                    id: defenseShield
                    visible: badge.shield
                    width: parent.width
                    title: badge.reduction ? qsTr("RD") : qsTr("RE")
                    value: String(badge.modelData.entry?.effectiveValue ?? "")
                    inlineValue: true
                    fillColor: badge.reduction ? "#edf1ee" : root.energyPalette(badge.modelData.entry?.energy).fill
                    accentColor: badge.reduction ? "#425649" : root.energyPalette(badge.modelData.entry?.energy).accent
                    subtitle: {
                        if (!badge.reduction) return root.energyName(badge.modelData.entry?.energy);
                        const text = root.qualified(root.bypassText(badge.modelData.entry?.bypass), badge.modelData.entry?.applicability);
                        return text.charAt(0).toUpperCase() + text.slice(1);
                    }
                }
                Label {
                    id: tag
                    visible: !badge.shield
                    width: parent.width
                    height: parent.height
                    padding: 9
                    leftPadding: 28
                    text: badge.modelData.text
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: badge.modelData.kind === DefensesPanel.Vulnerability ? "#805026" : "#344d60"
                    Accessible.name: (badge.modelData.kind === DefensesPanel.Vulnerability ? qsTr("Vulnerabilità: ") : qsTr("Immunità: ")) + badge.modelData.text
                    background: Rectangle {
                        radius: 5
                        color: badge.modelData.kind === DefensesPanel.Vulnerability ? "#fbefde" : "#eaf0f5"
                    }
                    Canvas {
                        x: 8
                        width: 16
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.strokeStyle = tag.color;
                            ctx.lineWidth = 1.2;
                            ctx.lineJoin = "round";
                            ctx.lineCap = "round";
                            ctx.beginPath();
                            if (badge.modelData.kind === DefensesPanel.Vulnerability) {
                                ctx.moveTo(8, 2); ctx.lineTo(14, 14); ctx.lineTo(2, 14); ctx.closePath();
                                ctx.stroke();
                                ctx.beginPath(); ctx.moveTo(8, 6); ctx.lineTo(8, 9); ctx.stroke();
                                ctx.beginPath(); ctx.arc(8, 11.5, 0.7, 0, Math.PI * 2);
                                ctx.fillStyle = tag.color; ctx.fill();
                            } else {
                                ctx.moveTo(3, 2); ctx.lineTo(13, 2); ctx.lineTo(13, 8);
                                ctx.quadraticCurveTo(13, 11, 8, 14);
                                ctx.quadraticCurveTo(3, 11, 3, 8); ctx.closePath(); ctx.stroke();
                                ctx.beginPath(); ctx.moveTo(5.5, 7); ctx.lineTo(10.5, 7);
                                ctx.moveTo(8, 4.5); ctx.lineTo(8, 9.5); ctx.stroke();
                            }
                        }
                    }
                }
            }
        }
    }
}
