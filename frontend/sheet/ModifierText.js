.pragma library

function rows(set, penaltyKind, groupKind, entryKind) {
        if (!set) return [{kind: entryKind, text: qsTr("Dettagli dei bonus non disponibili.")}];
        const names = [qsTr("Bonus senza tipo"), qsTr("Bonus alchemico"), qsTr("Bonus di armatura"), qsTr("Bonus di armatura naturale"),
            qsTr("Bonus di circostanza"), qsTr("Bonus cognitivo"), qsTr("Bonus di competenza"), qsTr("Bonus di deviazione"), qsTr("Bonus di fortuna"),
            qsTr("Bonus intrinseco"), qsTr("Bonus morale"), qsTr("Bonus di potenziamento"), qsTr("Bonus profano"), qsTr("Bonus razziale"),
            qsTr("Bonus di resistenza"), qsTr("Bonus sacro"), qsTr("Bonus di schivare"), qsTr("Bonus di scudo"), qsTr("Bonus di taglia")];
        const groups = new Map();
        for (const entry of set.modifiers ?? []) {
            if (!entry.active) continue;
            const penalty = entry.type === penaltyKind;
            const category = penalty ? qsTr("Penalità") : names[entry.bonusType] ?? qsTr("Bonus di tipo non specificato");
            if (!groups.has(category)) groups.set(category, []);
            groups.get(category).push((penalty ? "−" : "+") + entry.resolvedValue + " " + (entry.source || qsTr("Fonte non specificata"))
                + (entry.condition ? qsTr(" · Condizione: ") + entry.condition : ""));
        }
        const lines = [];
        for (const [name, entries] of groups) {
            lines.push({kind: groupKind, text: name});
            for (const text of entries) lines.push({kind: entryKind, text: text, indent: 4});
        }
        return lines;
    }
