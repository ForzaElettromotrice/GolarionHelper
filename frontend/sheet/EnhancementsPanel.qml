pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    enum EffectKind { Contribution, Bonus, Penalty }
    enum Origin { Custom, Spell, Item, Ability }
    required property var enhancements
    property var resourceNames: ({})
    property var effectResources: []
    property var expressionTargets: []
    property var bonusTypes: [{value: AbilityScoresPanel.Generic, label: qsTr("Senza tipo")}]
    property var expandedGroupIds: []
    signal addEnhancementGroupRequested(var definition)
    signal addEnhancementRequested(string groupId, var definition)
    signal removeEnhancementRequested(string groupId, string effectId)
    signal removeEnhancementGroupRequested(string groupId)
    signal setEnhancementGroupActiveRequested(string groupId, bool active)
    spacing: 8

    function isCustom(group) {
        return !!group && (group.origin == null || group.origin.type === EnhancementsPanel.Custom);
    }

    function resourceDescription(id) {
        const descriptions = {
            str: qsTr("Punteggio di Forza."),
            dex: qsTr("Punteggio di Destrezza."),
            con: qsTr("Punteggio di Costituzione."),
            int: qsTr("Punteggio di Intelligenza."),
            wis: qsTr("Punteggio di Saggezza."),
            cha: qsTr("Punteggio di Carisma."),
            "hp.max": qsTr("Punti ferita massimi."),
            baseAttackBonus: qsTr("Bonus di attacco base."),
            initiative: qsTr("Prove di iniziativa."),
            "skill.ranks.max": qsTr("Gradi di abilità disponibili."),
            "attack.all": qsTr("Tutti i tiri per colpire."),
            "damage.all": qsTr("Tutti i tiri di danno."),
            "savingThrow.all": qsTr("Tutti i tiri salvezza."),
            "savingThrow.fortitude": qsTr("Tiri salvezza su Tempra."),
            "savingThrow.reflex": qsTr("Tiri salvezza su Riflessi."),
            "savingThrow.will": qsTr("Tiri salvezza su Volontà."),
            "armorClass.all": qsTr("Tutti i tipi di Classe Armatura."),
            "armorClass.normal": qsTr("Classe Armatura totale."),
            "armorClass.touch": qsTr("Classe Armatura a contatto."),
            "armorClass.flatFooted": qsTr("Classe Armatura da impreparato.")
        };
        return Object.prototype.hasOwnProperty.call(descriptions, id) ? descriptions[id] : "";
    }

    function targetDescription(id) {
        const descriptions = {
            level: qsTr("Livello totale del personaggio."),
            bab: qsTr("Bonus di attacco base totale."),
            strMod: qsTr("Modificatore di Forza."),
            dexMod: qsTr("Modificatore di Destrezza."),
            conMod: qsTr("Modificatore di Costituzione."),
            intMod: qsTr("Modificatore di Intelligenza."),
            wisMod: qsTr("Modificatore di Saggezza."),
            chaMod: qsTr("Modificatore di Carisma."),
            intPermanent: qsTr("Punteggio di Intelligenza senza modificatori temporanei."),
            intPermanentMod: qsTr("Modificatore di Intelligenza senza modificatori temporanei."),
            intSkillRanks: qsTr("Punteggio di Intelligenza usato per calcolare i gradi di abilità."),
            intSkillRanksMod: qsTr("Modificatore di Intelligenza usato per calcolare i gradi di abilità.")
        };
        if (Object.prototype.hasOwnProperty.call(descriptions, id)) return descriptions[id];
        if (id.startsWith("class.")) {
            const type = id.split(".").slice(2).join(".");
            switch (type) {
            case "level": return qsTr("Livello nella classe indicata nell’ID.");
            case "hitPoints": return qsTr("Contributo della classe ai punti ferita.");
            case "skillRanks": return qsTr("Gradi di abilità concessi dalla classe.");
            case "bab": return qsTr("Contributo della classe al bonus di attacco base.");
            case "savingThrow.fortitude": return qsTr("Bonus base su Tempra concesso dalla classe.");
            case "savingThrow.reflex": return qsTr("Bonus base su Riflessi concesso dalla classe.");
            case "savingThrow.will": return qsTr("Bonus base su Volontà concesso dalla classe.");
            }
        }
        return resourceDescription(id) || qsTr("Valore numerico della scheda utilizzabile nelle espressioni.");
    }

    function toggleDetails(groupId) {
        expandedGroupIds = expandedGroupIds.includes(groupId)
            ? expandedGroupIds.filter(id => id !== groupId) : expandedGroupIds.concat(groupId);
    }
    function effectRows(group) {
        const effects = group.enhancements ?? [];
        const rows = effects.map(effect => {
            const definition = effect.definition ?? {};
            const name = resourceNames[definition.resource] ?? definition.description
                ?? definition.resource ?? effect.label ?? qsTr("Effetto");
            const penalty = definition.type === AbilityScoresPanel.Penalty;
            const expression = definition.expression?.trim();
            const value = effect.resolvedValue ?? (/^[+-]?\d+$/.test(expression ?? "") ? Number(expression) : null);
            let text = name;
            if (value !== null) {
                const signed = penalty ? -value : value;
                text = (signed > 0 ? "+" + signed : signed < 0 ? "−" + Math.abs(signed) : "0") + " " + name;
            } else if (expression) {
                text = qsTr("%1: %2").arg(name).arg(penalty ? qsTr("penalità (%1)").arg(expression) : expression);
            }
            if (definition.condition) text += " · " + definition.condition;
            return {kind: InfoDetails.Entry, text: text, effectId: definition.id};
        });
        if (!rows.length) rows.push({kind: InfoDetails.Note, text: qsTr("Nessun effetto nel gruppo.")});
        if (group.effectsEditable === false) rows.push({kind: InfoDetails.Note, text: qsTr("Effetti in sola lettura.")});
        return rows;
    }

    Dialog {
        id: removeGroupDialog
        objectName: "removeEnhancementGroupDialog"
        property string groupId: ""
        readonly property var group: root.enhancements?.groups?.find(entry => entry.id === groupId)
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, Math.max(0, (parent?.width ?? root.width) - 24))
        modal: true
        title: qsTr("Elimina gruppo")
        contentItem: Label {
            text: qsTr("Eliminare «%1» e tutti i suoi effetti?").arg(removeGroupDialog.group?.name ?? "")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button {
                objectName: "confirmRemoveEnhancementGroup"
                text: qsTr("Elimina")
                enabled: root.isCustom(removeGroupDialog.group)
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
        onAccepted: if (root.isCustom(group)) root.removeEnhancementGroupRequested(groupId)
    }

    Dialog {
        id: groupEditor
        objectName: "enhancementGroupEditor"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, Math.max(0, (parent?.width ?? root.width) - 24))
        modal: true
        title: qsTr("Nuovo gruppo di potenziamenti")
        onAboutToShow: { groupName.clear(); groupDescription.clear(); }
        onOpened: groupName.forceActiveFocus()
        function submit() {
            const name = groupName.text.trim();
            if (!name) return;
            const definition = {name: name, description: groupDescription.text.trim() || null, enhancements: [], active: false};
            close();
            root.addEnhancementGroupRequested(definition);
        }
        contentItem: ColumnLayout {
            Label { text: qsTr("Nome") }
            TextField {
                id: groupName
                objectName: "enhancementGroupName"
                Layout.fillWidth: true
                Accessible.name: qsTr("Nome del gruppo")
                ContextMenu.menu: null
                onAccepted: groupEditor.submit()
            }
            Label { text: qsTr("Descrizione (facoltativa)") }
            TextField {
                id: groupDescription
                objectName: "enhancementGroupDescription"
                Layout.fillWidth: true
                Accessible.name: qsTr("Descrizione del gruppo")
                ContextMenu.menu: null
                onAccepted: groupEditor.submit()
            }
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: groupEditor.reject() }
            Button {
                objectName: "createEnhancementGroupButton"
                text: qsTr("Crea")
                enabled: !!groupName.text.trim()
                onClicked: groupEditor.submit()
            }
        }
    }
    Dialog {
        id: effectEditor
        objectName: "enhancementEffectEditor"
        property string groupId: ""
        property string selectedResourceId: ""
        readonly property var group: root.enhancements?.groups?.find(entry => entry.id === groupId)
        readonly property string query: resourceSearch.text.trim().toLocaleLowerCase()
        readonly property var filteredResources: root.effectResources
            .filter(entry => entry.id.toLocaleLowerCase().includes(query) || entry.name.toLocaleLowerCase().includes(query))
            .sort((first, second) => first.name.length - second.name.length
                || first.name.localeCompare(second.name, "it") || first.id.localeCompare(second.id))
        readonly property var resource: root.effectResources.find(entry => entry.id === selectedResourceId)
        readonly property bool searching: !resource && !!resourceSearch.text.trim()
        readonly property bool contribution: resource?.kinds?.includes(EnhancementsPanel.Contribution) ?? false
        readonly property var modifierKinds: [{value: EnhancementsPanel.Bonus, label: qsTr("Bonus")},
            {value: EnhancementsPanel.Penalty, label: qsTr("Penalità")}]
            .filter(option => resource?.kinds?.includes(option.value))
        readonly property var kind: contribution ? EnhancementsPanel.Contribution : modifierKinds[effectKind.currentIndex]?.value
        readonly property bool valid: group?.effectsEditable === true && !!resource && kind != null
            && !!effectExpression.text.trim() && (kind !== EnhancementsPanel.Bonus || effectBonusType.currentValue != null)
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, Math.max(0, (parent?.width ?? root.width) - 24))
        modal: true
        title: qsTr("Aggiungi effetto")
        onAboutToShow: {
            selectedResourceId = "";
            resourceSearch.clear();
            effectKind.currentIndex = 0;
            effectBonusType.currentIndex = 0;
            effectExpression.clear();
        }
        onOpened: resourceSearch.forceActiveFocus()
        function escapeText(value) {
            return value.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
        }
        function highlightedText(value) {
            if (!query) return escapeText(value);
            const lower = value.toLocaleLowerCase();
            let start = 0;
            let match = lower.indexOf(query);
            let result = "";
            while (match >= 0) {
                result += escapeText(value.slice(start, match))
                    + '<b><font color="#1769aa">' + escapeText(value.slice(match, match + query.length)) + '</font></b>';
                start = match + query.length;
                match = lower.indexOf(query, start);
            }
            return result + escapeText(value.slice(start));
        }
        function suggestionText(entry) {
            const name = highlightedText(entry.name);
            if (!entry.name.toLocaleLowerCase().includes(query) && entry.id !== entry.name)
                return name + " · " + highlightedText(entry.id);
            return name;
        }
        function selectResource(entry) {
            selectedResourceId = entry.id;
            resourceSearch.text = entry.name;
            effectKind.currentIndex = 0;
            effectExpression.forceActiveFocus();
        }
        function submit() {
            if (!valid) return;
            const definition = {kind: kind, resource: resource.id, expression: effectExpression.text.trim()};
            if (kind !== EnhancementsPanel.Contribution) {
                definition.description = group.name;
                definition.type = kind === EnhancementsPanel.Penalty ? AbilityScoresPanel.Penalty : AbilityScoresPanel.Bonus;
                if (kind === EnhancementsPanel.Bonus) definition.bonusType = effectBonusType.currentValue;
            }
            close();
            root.addEnhancementRequested(groupId, definition);
        }
        contentItem: ColumnLayout {
            Label { text: qsTr("Risorsa") }
            TextField {
                id: resourceSearch
                objectName: "enhancementResourceSearch"
                Layout.fillWidth: true
                placeholderText: qsTr("Cerca una risorsa…")
                Accessible.name: qsTr("Cerca una risorsa")
                ContextMenu.menu: null
                onTextChanged: if (text !== effectEditor.resource?.name) effectEditor.selectedResourceId = ""
                onAccepted: {
                    if (effectEditor.searching && effectEditor.filteredResources.length === 1)
                        effectEditor.selectResource(effectEditor.filteredResources[0]);
                }
            }
            ListView {
                id: suggestions
                objectName: "enhancementResourceSuggestions"
                visible: effectEditor.searching && count > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(count, 5) * 36
                clip: true
                model: effectEditor.filteredResources
                delegate: ItemDelegate {
                    id: suggestion
                    required property var modelData
                    objectName: "enhancementResourceOption_" + modelData.id
                    width: suggestions.width
                    height: 36
                    text: modelData.name
                    contentItem: Label {
                        objectName: "highlightedResourceText"
                        textFormat: Text.StyledText
                        text: effectEditor.suggestionText(suggestion.modelData)
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    onClicked: effectEditor.selectResource(modelData)
                }
                ScrollBar.vertical: ScrollBar {}
            }
            Label {
                visible: effectEditor.searching && suggestions.count === 0
                text: qsTr("Nessuna risorsa trovata")
                color: "#536976"
            }
            Label {
                objectName: "enhancementResourceKind"
                visible: !!effectEditor.resource
                text: effectEditor.contribution ? qsTr("Contributo") : qsTr("Modificatore")
            }
            ComboBox {
                id: effectKind
                objectName: "enhancementModifierKind"
                visible: !effectEditor.contribution && effectEditor.modifierKinds.length > 1
                Layout.fillWidth: true
                textRole: "label"
                valueRole: "value"
                Accessible.name: qsTr("Bonus o penalità")
                model: effectEditor.modifierKinds
            }
            Label { visible: effectEditor.kind === EnhancementsPanel.Bonus; text: qsTr("Tipo di bonus") }
            ComboBox {
                id: effectBonusType
                visible: effectEditor.kind === EnhancementsPanel.Bonus
                Layout.fillWidth: true
                model: root.bonusTypes
                textRole: "label"
                valueRole: "value"
                Accessible.name: qsTr("Tipo di bonus")
            }
            Label { visible: effectEditor.kind != null; text: qsTr("Valore o espressione") }
            TextField {
                id: effectExpression
                objectName: "enhancementEffectExpression"
                visible: effectEditor.kind != null
                Layout.fillWidth: true
                placeholderText: qsTr("Es. 2")
                Accessible.name: qsTr("Valore o espressione")
                ContextMenu.menu: null
                onAccepted: effectEditor.submit()
            }
        }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: effectEditor.reject() }
            Button {
                objectName: "confirmEnhancementEffectButton"
                text: qsTr("Aggiungi")
                enabled: effectEditor.valid
                onClicked: effectEditor.submit()
            }
        }
    }
    RowLayout {
        width: parent.width
        Label { text: qsTr("Potenziamenti"); font.pixelSize: 20; font.bold: true; color: "#253947" }
        ToolButton {
            objectName: "addEnhancementGroupButton"
            text: "+"
            Accessible.name: qsTr("Aggiungi un gruppo di potenziamenti")
            onClicked: groupEditor.open()
        }
        Item { Layout.fillWidth: true }
    }
    Label {
        visible: !(root.enhancements?.groups?.length)
        text: qsTr("Nessun gruppo di potenziamenti.")
        color: "#536976"
    }
    Repeater {
        model: root.enhancements?.groups ?? []
        Rectangle {
            id: groupCard
            required property var modelData
            readonly property bool expanded: root.expandedGroupIds.includes(modelData.id)
            objectName: "enhancementGroup_" + modelData.id
            width: root.width
            implicitHeight: groupRow.implicitHeight + 16 + (expanded ? groupDetails.implicitHeight + 8 : 0)
                + (groupActions.visible ? groupActions.implicitHeight + 8 : 0)
            color: modelData.active ? "#eef6f1" : "#fafbfc"
            border.color: modelData.active ? "#a8c6b4" : "#d2dbe1"
            radius: 5

            RowLayout {
                id: groupRow
                x: 10; y: 8
                width: parent.width - 20
                ItemDelegate {
                    objectName: "enhancementGroupHeader_" + groupCard.modelData.id
                    Layout.fillWidth: true
                    padding: 0
                    background: Item {}
                    Accessible.name: (groupCard.expanded ? qsTr("Nascondi effetti di %1") : qsTr("Mostra effetti di %1")).arg(groupCard.modelData.name)
                    onClicked: root.toggleDetails(groupCard.modelData.id)
                    contentItem: RowLayout {
                        Label { text: groupCard.expanded ? "▾" : "▸"; color: "#536976" }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                Layout.fillWidth: true
                                text: groupCard.modelData.name
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.bold: true
                                color: "#253947"
                            }
                            Label {
                                Layout.fillWidth: true
                                visible: !!groupCard.modelData.description
                                text: groupCard.modelData.description ?? ""
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                color: "#536976"
                            }
                        }
                    }
                }
                Switch {
                    id: activationSwitch
                    objectName: "enhancementToggle_" + groupCard.modelData.id
                    Layout.preferredWidth: Math.ceil(Math.max(activeText.advanceWidth, inactiveText.advanceWidth))
                        + leftPadding + rightPadding + spacing + (indicator?.implicitWidth ?? 0)
                    TextMetrics { id: activeText; font: activationSwitch.font; text: qsTr("Attivo") }
                    TextMetrics { id: inactiveText; font: activationSwitch.font; text: qsTr("Disattivo") }
                    text: groupCard.modelData.active ? activeText.text : inactiveText.text
                    Accessible.name: qsTr("Attivazione: %1").arg(groupCard.modelData.name)
                    checked: groupCard.modelData.active
                    onToggled: {
                        const requested = checked;
                        checked = Qt.binding(() => groupCard.modelData.active);
                        root.setEnhancementGroupActiveRequested(groupCard.modelData.id, requested);
                    }
                }
            }
            InfoFrame {
                id: groupDetails
                readonly property var rows: visible ? root.effectRows(groupCard.modelData) : []
                readonly property string summaryText: rows.map(row => row.text).join("\n")
                objectName: "enhancementGroupDetails_" + groupCard.modelData.id
                x: groupRow.x
                y: groupRow.y + groupRow.height + 8
                width: groupRow.width
                implicitHeight: effectList.implicitHeight + 16
                height: implicitHeight
                visible: groupCard.expanded
                Column {
                    id: effectList
                    x: 10; y: 8
                    width: parent.width - 20
                    spacing: 3
                    Repeater {
                        model: groupDetails.rows
                        RowLayout {
                            id: effectRow
                            required property var modelData
                            width: effectList.width
                            Label {
                                Layout.fillWidth: true
                                text: effectRow.modelData.text
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.italic: effectRow.modelData.kind === InfoDetails.Note
                            }
                            ToolButton {
                                objectName: "removeEnhancementEffect_" + (effectRow.modelData.effectId ?? "")
                                visible: groupCard.modelData.effectsEditable === true && !!effectRow.modelData.effectId
                                text: "×"
                                Accessible.name: qsTr("Rimuovi effetto: %1").arg(effectRow.modelData.text)
                                onClicked: root.removeEnhancementRequested(groupCard.modelData.id, effectRow.modelData.effectId)
                            }
                        }
                    }
                }
            }
            RowLayout {
                id: groupActions
                x: groupDetails.x
                y: groupDetails.y + groupDetails.height + 8
                width: groupDetails.width
                visible: groupCard.expanded && root.isCustom(groupCard.modelData)
                Button {
                    objectName: "addEnhancementEffect_" + groupCard.modelData.id
                    visible: groupCard.modelData.effectsEditable === true
                    enabled: root.effectResources.length > 0
                    text: qsTr("+ Aggiungi effetto")
                    onClicked: {
                        effectEditor.groupId = groupCard.modelData.id;
                        effectEditor.open();
                    }
                }
                Item { Layout.fillWidth: true }
                Button {
                    objectName: "removeEnhancementGroup_" + groupCard.modelData.id
                    text: qsTr("Elimina gruppo")
                    onClicked: {
                        removeGroupDialog.groupId = groupCard.modelData.id;
                        removeGroupDialog.open();
                    }
                }
            }
        }
    }
    Label {
        visible: root.effectResources.length > 0 || root.expressionTargets.length > 0
        text: qsTr("Riferimento per gli effetti")
        font.pixelSize: 20
        font.bold: true
        color: "#253947"
    }
    Repeater {
        model: [
            {key: "targets", expression: true, title: qsTr("Valori per le espressioni (@)"),
                explanation: qsTr("Usa questi riferimenti per leggere valori della scheda, per esempio @level + 2.")},
            {kind: EnhancementsPanel.Bonus, title: qsTr("Risorse per bonus e penalità"),
                explanation: qsTr("Un bonus segue le regole di cumulo del suo tipo. Una penalità si sottrae al valore.")},
            {kind: EnhancementsPanel.Contribution, title: qsTr("Risorse per contributi"),
                explanation: qsTr("Un contributo si somma al totale della risorsa e può essere positivo o negativo.")}
        ]
        delegate: Column {
            id: section
            required property var modelData
            readonly property var resources: modelData.expression ? root.expressionTargets.map(id => ({id}))
                : root.effectResources.filter(resource => resource.kinds.includes(modelData.kind))
            width: root.width
            visible: resources.length > 0
            spacing: 6
            Button {
                id: disclosure
                objectName: "enhancementReference_" + (section.modelData.key ?? section.modelData.kind)
                width: parent.width
                checkable: true
                text: (checked ? "▾ " : "▸ ") + section.modelData.title + " (" + section.resources.length + ")"
            }
            Loader {
                width: parent.width
                active: disclosure.checked && section.visible
                visible: active
                sourceComponent: Component {
                    Column {
                        spacing: 6
                        Label {
                            width: parent.width
                            text: section.modelData.explanation
                            wrapMode: Text.Wrap
                        }
                        Repeater {
                            model: section.resources
                            Label {
                                required property var modelData
                                readonly property string description: section.modelData.expression
                                    ? root.targetDescription(modelData.id) : root.resourceDescription(modelData.id)
                                readonly property string identifier: (section.modelData.expression ? "@" : "") + modelData.id
                                objectName: "enhancementReferenceResource_" + identifier
                                width: parent.width
                                padding: 6
                                text: description ? identifier + " — " + description : identifier
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                background: InfoFrame {}
                            }
                        }
                    }
                }
            }
        }
    }
}
