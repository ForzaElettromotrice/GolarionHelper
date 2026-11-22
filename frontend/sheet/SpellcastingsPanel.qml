pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    enum Mode { Prepared, Spontaneous, Flexible }
    enum SlotStatus { Free, Ready, Used }
    enum SpontaneousSlotStatus { CastSlotFree, CastSlotUsed }
    enum MetamagicType { Silent = 6, Still = 7 }
    required property var spellcastings
    property var spellNames: ({})
    property var spellCatalog: []
    property var spellLevelsByClass: ({})
    property var canPrepareSpell: null
    property var preparationMetamagicOptions: null
    property var castingMetamagicOptions: null
    property var canCastSpontaneousSpell: null
    property var canPrepareFlexibleSpell: null
    property var canCastFlexibleSpell: null
    property var flexiblePreparationMetamagicOptions: null
    property var flexibleCastingMetamagicOptions: null
    property var conversionSpellOptions: null
    property var preparationSlotCount: null
    property var metamagicDetails: ({})
    signal prepareSpellRequested(string castingId, var slotIds, string spellId, var metamagic)
    signal consumeSpellRequested(string castingId, var slotId, var metamagic, var replacementSpellId)
    signal castSpontaneousSpellRequested(string castingId, var slotId, string spellId, var metamagic)
    signal prepareFlexibleSpellRequested(string castingId, var preparationId, string spellId, var metamagic)
    signal clearFlexiblePreparationRequested(string castingId, var preparationId)
    signal castFlexibleSpellRequested(string castingId, var slotId, var preparationId, var metamagic, var mixingPermissionId)
    signal clearSpellSlotRequested(string castingId, var slotId)
    signal resetSpellSlotsRequested(string castingId)
    signal addSpellToRepertoireRequested(string castingId, string spellId)
    signal selectRepertoireSpellRequested(string castingId, var selection)
    property bool removingPreparation: false
    property var removalSlotId: null
    property string draggedSpellId: ""
    property string dragCastingId: ""
    property var draggedFlexiblePreparation: null
    readonly property var castings: spellcastings?.castings ?? []
    property string selectedId: ""
    readonly property var activeCasting: castings.find(entry => entry.id === selectedId) ?? castings[0] ?? null
    readonly property bool preparing: draggedSpellId !== "" && activeCasting?.id === dragCastingId
    readonly property bool canDropInRepertoire: removingPreparation || (preparing && !!draggedFlexiblePreparation)
    onActiveCastingChanged: {
        draggedSpellId = "";
        dragCastingId = "";
        draggedFlexiblePreparation = null;
        removingPreparation = false;
        removalSlotId = null;
        if (metamagicPicker?.opened) metamagicPicker.reject();
        if (repertoirePicker?.opened) repertoirePicker.reject();
    }
    spacing: 16

    function modeName(mode) {
        if (mode === SpellcastingsPanel.Prepared) return qsTr("Preparato");
        if (mode === SpellcastingsPanel.Spontaneous) return qsTr("Spontaneo");
        if (mode === SpellcastingsPanel.Flexible) return qsTr("Flessibile");
        return qsTr("Modalità non disponibile");
    }
    function slotLevels(casting) {
        if (!casting) return [];
        const levels = {};
        const groups = casting.mode === SpellcastingsPanel.Flexible ? casting.state?.slotGroups : casting.state?.groups;
        for (const group of groups ?? []) {
            for (const entry of group.slots ?? []) {
                const level = entry.slot.spellLevel;
                if (!levels[level]) levels[level] = {level: level, free: 0, ready: 0, used: 0, entries: []};
                if (casting.mode === SpellcastingsPanel.Prepared) {
                    if (entry.state === SpellcastingsPanel.Free) ++levels[level].free;
                    else if (entry.state === SpellcastingsPanel.Ready) ++levels[level].ready;
                    else if (entry.state === SpellcastingsPanel.Used) ++levels[level].used;
                } else {
                    if (entry.state === SpellcastingsPanel.CastSlotFree) ++levels[level].free;
                    else if (entry.state === SpellcastingsPanel.CastSlotUsed) ++levels[level].used;
                }
                levels[level].entries.push({slot: entry, source: group.grant?.source ?? "",
                    allowedSpellIds: group.grant?.allowedSpellIds ?? null});
            }
        }
        return Object.values(levels).sort((a, b) => a.level - b.level);
    }
    readonly property var activeLevels: slotLevels(activeCasting)
    readonly property var flexiblePreparationLevels: {
        const groups = {};
        for (const group of activeCasting?.state?.preparationGroups ?? [])
            for (const entry of group.preparations ?? []) {
                const level = entry.preparationId.spellLevel;
                if (!groups[level]) groups[level] = {level: level, entries: []};
                groups[level].entries.push(entry);
            }
        return Object.values(groups).sort((a, b) => a.level - b.level);
    }
    function sameSlot(left, right) {
        return left.groupId === right.groupId && left.spellLevel === right.spellLevel && left.index === right.index;
    }
    function isSecondary(entry) {
        return !!entry.preparationSlot && !sameSlot(entry.slot, entry.preparationSlot);
    }
    function preparationSlots(slot, spellId, applications) {
        const count = preparationSlotCount?.(activeCasting, slot, spellId, applications) ?? 1;
        if (!Number.isInteger(count) || count < 1) return null;
        if (count === 1) return [slot];
        const entries = activeLevels.find(level => level.level === slot.spellLevel)?.entries ?? [];
        const candidates = entries.filter(entry => entry.slot.state === SpellcastingsPanel.Free
            && !sameSlot(entry.slot.slot, slot)
            && (entry.allowedSpellIds === null || entry.allowedSpellIds.includes(spellId))
            && canPrepareSpell?.(activeCasting, entry.slot.slot, spellId, applications, slot));
        const ordered = candidates.filter(entry => entry.allowedSpellIds === null)
            .concat(candidates.filter(entry => entry.allowedSpellIds !== null));
        return ordered.length < count - 1 ? null : [slot].concat(ordered.slice(0, count - 1).map(entry => entry.slot.slot));
    }
    function spellName(id) { return spellNames[id] ?? id; }
    readonly property var repertoireAddOptions: {
        const options = [];
        const details = activeCasting?.repertoireDetails;
        for (const quota of details?.quotas ?? [])
            for (const key of Object.keys(quota.remainingByLevel ?? {})) {
                const remaining = quota.remainingByLevel[key];
                if (remaining <= 0) continue;
                const level = Number(key);
                options.push({quotaId: quota.grant.id, level: level, allowedIds: quota.grant.allowedSpellIds ?? null,
                    label: qsTr("%1 · Livello %2 · %3 disponibili").arg(quota.grant.source).arg(level).arg(remaining)});
            }
        if (details?.manualAdditionAllowed && !details.listGrants?.length
            && activeCasting?.mode !== SpellcastingsPanel.Spontaneous)
            options.push({quotaId: null, label: qsTr("Aggiunta manuale")});
        return options;
    }
    function repertoireCandidates(option, query) {
        if (!option || !activeCasting) return [];
        const levels = spellLevelsByClass[activeCasting.state?.spellListClass] ?? {};
        const known = new Set(option.quotaId === null ? activeCasting.repertoire ?? []
            : (activeCasting.repertoireDetails?.selections ?? []).map(entry => entry.selection.spellId));
        return spellCatalog.filter(entry => !known.has(entry.id) && levels[entry.id] !== undefined
            && (option.quotaId === null || (levels[entry.id] === option.level
                && (option.allowedIds === null || option.allowedIds.includes(entry.id))))
            && entry.name.toLocaleLowerCase().includes(query.trim().toLocaleLowerCase()))
            .sort((a, b) => levels[a.id] - levels[b.id] || a.name.localeCompare(b.name, "it"));
    }
    function slotName(entry, index) {
        const name = entry.slot.spellId ? spellName(entry.slot.spellId) : qsTr("Slot %1").arg(index + 1);
        return entry.allowedSpellIds === null ? name : qsTr("%1 · Limited").arg(name);
    }
    function slotRestrictionRows(spellIds) {
        const rows = [{kind: InfoDetails.Heading, text: qsTr("Incantesimi ammessi")}];
        for (const id of spellIds ?? [])
            rows.push({kind: InfoDetails.Entry, text: spellName(id)});
        if (rows.length === 1) rows.push({kind: InfoDetails.Note, text: qsTr("Nessun incantesimo ammesso.")});
        return rows;
    }
    function repertoireLevels(casting) {
        const levels = {};
        const spellLevels = spellLevelsByClass[casting?.state?.spellListClass] ?? {};
        const ids = new Set(casting?.repertoire ?? []);
        if (casting?.repertoireDetails?.listGrants?.length) {
            const slots = slotLevels(casting);
            for (const spell of spellCatalog) {
                const level = spellLevels[spell.id];
                if (level !== undefined && slots.some(group => group.level >= level && group.entries.some(entry =>
                    entry.allowedSpellIds === null || entry.allowedSpellIds.includes(spell.id))))
                    ids.add(spell.id);
            }
        }
        for (const id of ids) {
            const level = spellLevels[id] ?? -1;
            if (!levels[level]) levels[level] = {level: level, spells: []};
            levels[level].spells.push(id);
        }
        return Object.values(levels).sort((a, b) => (a.level < 0 ? Infinity : a.level) - (b.level < 0 ? Infinity : b.level));
    }
    readonly property var activeRepertoireLevels: repertoireLevels(activeCasting)
    function slotUsed(entry) {
        return entry.state === (activeCasting?.mode === SpellcastingsPanel.Prepared ? SpellcastingsPanel.Used : SpellcastingsPanel.CastSlotUsed);
    }
    function slotStatus(entry) {
        if (slotUsed(entry)) return qsTr("Consumato");
        if (isSecondary(entry)) return qsTr("Collegato");
        if (activeCasting?.mode === SpellcastingsPanel.Prepared)
            return entry.state === SpellcastingsPanel.Ready ? qsTr("Preparato") : qsTr("Da preparare");
        return qsTr("Disponibile");
    }
    function metamagicOptions(slot, spellId) {
        return preparationMetamagicOptions?.(activeCasting, slot, spellId) ?? [];
    }
    function metamagicName(application) {
        const name = metamagicDetails[application.type]?.name ?? application.source ?? application.grantId ?? application.id ?? "";
        const options = application.options;
        if (options?.level) return qsTr("%1 · livello %2").arg(name).arg(options.level);
        if (options?.steps) return qsTr("%1 · incrementi: %2").arg(name).arg(options.steps);
        if (options?.energy != null) {
            const energy = [qsTr("Acido"), qsTr("Freddo"), qsTr("Elettricità"), qsTr("Fuoco")][options.energy - 3];
            if (energy) return qsTr("%1 · %2 (%3)").arg(name).arg(energy).arg(options.mode === 0 ? qsTr("sostituisce") : qsTr("divide"));
        }
        return name;
    }
    function metamagicLevel(grant) {
        const adjustment = grant.levelAdjustment ?? metamagicDetails[grant.type]?.levelAdjustment;
        return typeof adjustment === "number" ? qsTr("+%1 liv.").arg(adjustment) : qsTr("Variabile");
    }
    function preparationChoices(slot, spellId) {
        const choices = [];
        if (canPrepareSpell?.(activeCasting, slot, spellId, []))
            choices.push({applications: []});
        for (const option of metamagicOptions(slot, spellId)) choices.push(option);
        return choices.map(option => ({applications: option.applications, slots: preparationSlots(slot, spellId, option.applications)}))
            .filter(option => !!option.slots);
    }
    function requestPreparation(slot, spellId, choices) {
        if (!choices.length) return;
        if (!choices.some(option => option.applications.length)) {
            prepareSpellRequested(activeCasting.id, choices[0].slots, spellId, []);
            return;
        }
        openMetamagicPicker(slot, spellId, choices, null, []);
    }
    function spontaneousChoices(entry, spellId) {
        const cast = {slot: entry.slot, spellId: spellId};
        const options = castingMetamagicOptions?.(activeCasting, cast) ?? [];
        return canCastSpontaneousSpell?.(activeCasting, entry.slot, spellId)
            ? [{applications: []}].concat(options) : options;
    }
    function requestSpontaneousCast(entry, spellId, choices) {
        if (!choices.length) return;
        if (choices.some(option => option.applications.length))
            openMetamagicPicker(entry.slot, spellId, choices, entry, []);
        else castSpontaneousSpellRequested(activeCasting.id, entry.slot, spellId, []);
    }
    function requestConsumption(entry) {
        if (activeCasting?.mode !== SpellcastingsPanel.Prepared || entry.state !== SpellcastingsPanel.Ready || !entry.spellId || isSecondary(entry))
            return;
        const options = castingMetamagicOptions?.(activeCasting, entry) ?? [];
        const replacements = (conversionSpellOptions?.(activeCasting, entry) ?? []).filter(id => id !== entry.spellId);
        if (!options.length && !replacements.length) consumeSpellRequested(activeCasting.id, entry.slot, [], null);
        else openMetamagicPicker(entry.slot, entry.spellId, [{applications: []}].concat(options), entry, replacements);
    }
    function flexiblePreparationChoices(preparationId, spellId) {
        const options = flexiblePreparationMetamagicOptions?.(activeCasting, preparationId, spellId) ?? [];
        return canPrepareFlexibleSpell?.(activeCasting, preparationId, spellId) ? [{applications: []}].concat(options) : options;
    }
    function flexibleCastingChoices(entry, preparation) {
        const options = flexibleCastingMetamagicOptions?.(activeCasting, entry.slot, preparation) ?? [];
        return canCastFlexibleSpell?.(activeCasting, entry.slot, preparation) ? [{applications: []}].concat(options) : options;
    }
    function requestFlexiblePreparation(preparationId, spellId, choices) {
        if (choices.some(choice => choice.applications.length))
            openMetamagicPicker(preparationId, spellId, choices, null, []);
        else prepareFlexibleSpellRequested(activeCasting.id, preparationId, spellId, []);
    }
    function requestFlexibleCast(entry, preparation, choices) {
        if (choices.some(choice => choice.applications.length))
            openMetamagicPicker(entry.slot, preparation.spellId, choices, entry, [], preparation.preparationId);
        else castFlexibleSpellRequested(activeCasting.id, entry.slot, preparation.preparationId, [], null);
    }
    function openMetamagicPicker(slot, spellId, options, castingEntry, replacements, flexiblePreparationId = null) {
        metamagicPicker.castingId = activeCasting.id;
        metamagicPicker.castingEntry = castingEntry;
        metamagicPicker.replacements = replacements;
        metamagicPicker.replacementSpellId = null;
        metamagicPicker.flexiblePreparationId = flexiblePreparationId;
        metamagicPicker.slotId = slot;
        metamagicPicker.spellId = spellId;
        metamagicPicker.selected = [];
        metamagicPicker.options = options;
        conversionChoice.currentIndex = 0;
        metamagicPicker.open();
    }

    Dialog {
        id: repertoirePicker
        objectName: "spellRepertoirePicker"
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Aggiungi un incantesimo")
        width: Math.min(500, (parent?.width ?? root.width) - 24)
        onAboutToShow: { repertoireSearch.clear(); learningOption.currentIndex = 0; }
        footer: DialogButtonBox {
            Button { text: qsTr("Annulla"); onClicked: repertoirePicker.reject() }
        }
        contentItem: Column {
            spacing: 8
            ComboBox {
                id: learningOption
                objectName: "spellLearningOption"
                width: parent.width
                model: root.repertoireAddOptions
                textRole: "label"
                Accessible.name: qsTr("Tipo di apprendimento")
            }
            TextField {
                id: repertoireSearch
                width: parent.width
                placeholderText: qsTr("Cerca un incantesimo…")
                ContextMenu.menu: null
            }
            ListView {
                id: repertoireList
                objectName: "spellRepertoireCatalogList"
                width: parent.width
                height: Math.min(350, (repertoirePicker.parent?.height ?? 600) * 0.45)
                clip: true
                model: root.repertoireCandidates(root.repertoireAddOptions[learningOption.currentIndex], repertoireSearch.text)
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    id: candidate
                    required property var modelData
                    objectName: "repertoireCandidate_" + modelData.id
                    width: repertoireList.width
                    contentItem: Label {
                        text: candidate.modelData.name
                        textFormat: Text.PlainText
                    }
                    onClicked: {
                        const option = root.repertoireAddOptions[learningOption.currentIndex];
                        if (!option || !root.activeCasting) return;
                        repertoirePicker.close();
                        if (option.quotaId === null)
                            root.addSpellToRepertoireRequested(root.activeCasting.id, candidate.modelData.id);
                        else root.selectRepertoireSpellRequested(root.activeCasting.id,
                            {quotaId: option.quotaId, spellLevel: option.level, spellId: candidate.modelData.id});
                    }
                }
                Label {
                    anchors.centerIn: parent
                    visible: !repertoireList.count
                    text: qsTr("Nessun incantesimo disponibile.")
                }
            }
        }
    }

    Dialog {
        id: metamagicPicker
        objectName: "preparationMetamagicPicker"
        property string castingId: ""
        property var castingEntry: null
        readonly property bool consuming: !!castingEntry
        property var replacements: []
        property var replacementSpellId: null
        property var flexiblePreparationId: null
        property var slotId
        property string spellId: ""
        property var options: []
        property var selected: []
        readonly property var combinations: options.map(option => option.applications.map(application => JSON.stringify(application)))
        readonly property var choices: {
            const entries = new Map();
            for (const option of options)
                for (const application of option.applications)
                    entries.set(JSON.stringify(application), application);
            return Array.from(entries, ([key, application]) => ({key: key, application: application}));
        }
        readonly property var matchingChoices: options.filter((option, index) => combinations[index].length === selected.length
            && selected.every(key => combinations[index].includes(key)))
        readonly property var confirmedChoice: matchingChoices[Math.max(0, mixingChoice.currentIndex)]
        function canSelect(key) {
            return selected.includes(key) || combinations.some(keys => keys.includes(key) && selected.every(item => keys.includes(item)));
        }
        function toggle(key) {
            if (selected.includes(key)) selected = selected.filter(item => item !== key);
            else if (canSelect(key)) selected = selected.concat(key);
        }
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: replacements.length ? qsTr("Lancia incantesimo") : consuming ? qsTr("Metamagia al lancio") : qsTr("Metamagia di preparazione")
        footer: DialogButtonBox {
            Button {
                text: metamagicPicker.consuming ? qsTr("Lancia") : qsTr("Prepara")
                enabled: !!metamagicPicker.confirmedChoice
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: qsTr("Annulla")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }
        onAccepted: {
            const choice = confirmedChoice;
            if (!choice || root.activeCasting?.id !== castingId) return;
            if (root.activeCasting.mode === SpellcastingsPanel.Flexible) {
                if (consuming) root.castFlexibleSpellRequested(castingId, slotId, flexiblePreparationId,
                    choice.applications, choice.mixingPermissionId ?? null);
                else root.prepareFlexibleSpellRequested(castingId, slotId, spellId, choice.applications);
            } else if (consuming && root.activeCasting.mode === SpellcastingsPanel.Spontaneous)
                root.castSpontaneousSpellRequested(castingId, slotId, spellId, choice.applications);
            else if (consuming) root.consumeSpellRequested(castingId, slotId, choice.applications, replacementSpellId);
            else root.prepareSpellRequested(castingId, choice.slots, spellId, choice.applications);
        }
        contentItem: ColumnLayout {
            Label {
                text: qsTr("%1 · slot di livello %2").arg(root.spellName(metamagicPicker.spellId)).arg(metamagicPicker.slotId?.spellLevel ?? "")
                textFormat: Text.PlainText
            }
            ComboBox {
                id: conversionChoice
                objectName: "conversionSpellChoice"
                visible: metamagicPicker.replacements.length > 0
                Layout.fillWidth: true
                Accessible.name: qsTr("Incantesimo da lanciare")
                model: [{id: null, name: qsTr("%1 (preparato)").arg(root.spellName(metamagicPicker.spellId))}]
                    .concat(metamagicPicker.replacements.map(id => ({id: id, name: root.spellName(id)})))
                textRole: "name"
                onActivated: index => {
                    const replacement = model[index].id;
                    metamagicPicker.replacementSpellId = replacement;
                    metamagicPicker.selected = [];
                    metamagicPicker.options = [{applications: []}].concat(
                        root.castingMetamagicOptions?.(root.activeCasting, metamagicPicker.castingEntry, replacement) ?? []);
                }
            }
            Repeater {
                model: metamagicPicker.choices
                CheckBox {
                    id: metamagicChoice
                    required property var modelData
                    required property int index
                    objectName: "preparationMetamagicChoice_" + index
                    Layout.fillWidth: true
                    text: root.metamagicName(modelData.application)
                    checked: metamagicPicker.selected.includes(modelData.key)
                    enabled: metamagicPicker.canSelect(modelData.key)
                    opacity: enabled ? 1 : 0.45
                    nextCheckState: function() {
                        metamagicPicker.toggle(modelData.key);
                        return metamagicPicker.selected.includes(modelData.key) ? Qt.Checked : Qt.Unchecked;
                    }
                }
            }
            ComboBox {
                id: mixingChoice
                objectName: "metamagicMixingChoice"
                visible: metamagicPicker.matchingChoices.length > 1
                Layout.fillWidth: true
                Accessible.name: qsTr("Permesso per combinare le metamagie")
                model: metamagicPicker.matchingChoices.map(choice => choice.mixingLabel ?? choice.mixingPermissionId ?? qsTr("Nessun costo aggiuntivo"))
                onModelChanged: currentIndex = count ? 0 : -1
            }
            Label {
                visible: !mixingChoice.visible && !!metamagicPicker.confirmedChoice?.mixingLabel
                text: metamagicPicker.confirmedChoice?.mixingLabel ?? ""
                textFormat: Text.PlainText
            }
        }
    }

    Label { text: qsTr("Incantesimi"); font.pixelSize: 20; font.bold: true; color: "#253947" }
    ComboBox {
        objectName: "spellcastingSelector"
        visible: root.castings.length > 1
        width: Math.min(parent.width, 320)
        model: root.castings.map(entry => ({id: entry.id, label: entry.source + " · " + root.modeName(entry.mode)}))
        textRole: "label"
        valueRole: "id"
        currentIndex: root.castings.indexOf(root.activeCasting)
        Accessible.name: qsTr("Sistema di lancio")
        onActivated: root.selectedId = currentValue
    }
    Rectangle {
        visible: !!root.activeCasting
        width: parent.width
        implicitHeight: castingHeading.height + 32
        color: "#e9eef1"
        border.color: "#c6cfd5"
        radius: 8
        Column {
            id: castingHeading
            x: 16; y: 16
            width: parent.width - 32
            spacing: 4
            Label { objectName: "activeSpellcastingName"; width: parent.width; text: root.activeCasting?.source ?? ""; textFormat: Text.PlainText; wrapMode: Text.Wrap; font.pixelSize: 28; font.bold: true; color: "#253947" }
            Label { text: root.modeName(root.activeCasting?.mode); color: "#536976" }
        }
    }
    Label { visible: !root.activeCasting; text: qsTr("Nessun sistema di lancio."); color: "#536976" }
    GridLayout {
        id: castingColumns
        readonly property real headingHeight: Math.max(repertoireHeadingRow.implicitHeight, slotHeadingRow.implicitHeight,
            preparationHeading.implicitHeight)
        visible: !!root.activeCasting
        width: parent.width
        columns: root.activeCasting?.mode === SpellcastingsPanel.Flexible
            ? (width >= 680 ? 3 : 1) : (width >= 560 ? 2 : 1)
        columnSpacing: 16
        rowSpacing: 16
        Column {
            id: repertoirePane
            objectName: "spellRepertoire"
            Layout.fillWidth: true
            Layout.preferredWidth: 280
            Layout.alignment: Qt.AlignTop
            spacing: 8
            Item {
                id: repertoireHeading
                width: repertoirePane.width
                height: castingColumns.headingHeight
                z: root.canDropInRepertoire ? 1 : 0
                RowLayout {
                    id: repertoireHeadingRow
                    width: parent.width
                    height: parent.height
                    Label {
                        objectName: "spellRepertoireHeading"
                        text: qsTr("Repertorio"); font.pixelSize: 18; font.bold: true; color: "#253947"
                    }
                    ToolButton {
                        objectName: "addRepertoireSpellButton"
                        text: "+"
                        visible: !root.activeCasting?.repertoireDetails?.listGrants?.length || root.repertoireAddOptions.length > 0
                        enabled: root.repertoireAddOptions.length > 0 && root.spellCatalog.length > 0 && !root.preparing && !root.removingPreparation
                        Accessible.name: qsTr("Aggiungi un incantesimo")
                        onClicked: repertoirePicker.open()
                    }
                    Item { Layout.fillWidth: true }
                }
            }
            Label { visible: !root.activeRepertoireLevels.length; text: qsTr("Nessun incantesimo nel repertorio."); color: "#536976" }
            Repeater {
                model: root.activeRepertoireLevels
                Column {
                    id: repertoireGroup
                    required property var modelData
                    objectName: "spellRepertoireLevel_" + modelData.level
                    width: parent.width
                    spacing: 5
                    Label { text: repertoireGroup.modelData.level < 0 ? qsTr("Livello non indicato") : qsTr("Livello %1").arg(repertoireGroup.modelData.level); font.bold: true; font.pixelSize: 16; color: "#253947" }
                    Repeater {
                        model: repertoireGroup.modelData.spells
                        Rectangle {
                            id: spellCard
                            required property string modelData
                            objectName: "repertoireSpell_" + modelData
                            width: repertoireGroup.width
                            implicitHeight: spellTitle.implicitHeight + 16
                            color: "#fafbfc"
                            border.color: "#d2dbe1"
                            radius: 5
                            Label { id: spellTitle; x: 10; y: 8; width: parent.width - 20; text: root.spellName(spellCard.modelData); textFormat: Text.PlainText; wrapMode: Text.Wrap; color: "#253947" }
                            DragHandler {
                                id: spellDrag
                                enabled: root.activeCasting?.mode === SpellcastingsPanel.Prepared
                                    ? !!(root.canPrepareSpell || root.preparationMetamagicOptions)
                                    : root.activeCasting?.mode === SpellcastingsPanel.Spontaneous
                                        && !!(root.canCastSpontaneousSpell || root.castingMetamagicOptions)
                                        || root.activeCasting?.mode === SpellcastingsPanel.Flexible
                                            && !!(root.canPrepareFlexibleSpell || root.flexiblePreparationMetamagicOptions)
                                target: null
                                onActiveChanged: {
                                    if (active) {
                                        root.dragCastingId = root.activeCasting.id;
                                        root.draggedSpellId = spellCard.modelData;
                                        spellMarker.Drag.active = true;
                                    } else {
                                        if (root.preparing && spellMarker.Drag.active) spellMarker.Drag.drop();
                                        else spellMarker.Drag.cancel();
                                        root.draggedSpellId = "";
                                        root.dragCastingId = "";
                                    }
                                }
                                onCanceled: spellMarker.Drag.cancel()
                            }
                            Item {
                                id: spellMarker
                                x: spellDrag.centroid.position.x
                                y: spellDrag.centroid.position.y
                                Drag.keys: ["prepared-spell"]
                                Drag.supportedActions: Qt.CopyAction
                                Drag.proposedAction: Qt.CopyAction
                            }
                            Rectangle {
                                parent: Overlay.overlay
                                visible: spellDrag.active && root.preparing
                                z: 1
                                readonly property point pointer: visible && parent
                                    ? spellCard.mapToItem(parent, spellDrag.centroid.position.x, spellDrag.centroid.position.y)
                                    : Qt.point(0, 0)
                                x: pointer.x + 12
                                y: pointer.y + 10
                                width: draggedTitle.implicitWidth + 20
                                height: draggedTitle.implicitHeight + 12
                                color: "#fafbfc"
                                border.color: "#52768e"
                                radius: 5
                                Label { id: draggedTitle; anchors.centerIn: parent; text: spellTitle.text; textFormat: Text.PlainText; color: "#253947" }
                            }
                        }
                    }
                }
            }
            Label { text: qsTr("Metamagie conosciute"); font.pixelSize: 18; font.bold: true; color: "#253947" }
            Label {
                visible: !(root.activeCasting?.state?.metamagic?.length)
                text: qsTr("Nessuna metamagia conosciuta.")
                color: "#536976"
            }
            Repeater {
                model: root.activeCasting?.state?.metamagic ?? []
                RowLayout {
                    id: metamagicRow
                    required property var modelData
                    width: repertoirePane.width
                    Label {
                        Layout.fillWidth: true
                        text: root.metamagicName(metamagicRow.modelData)
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        color: "#253947"
                    }
                    Label { text: root.metamagicLevel(metamagicRow.modelData); color: "#536976" }
                }
            }
            DropArea {
                id: preparationRemovalArea
                parent: repertoireHeading
                width: repertoirePane.width
                height: repertoirePane.height
                keys: ["remove-preparation", "flexible-cast"]
                onDropped: drop => {
                    if (!root.canDropInRepertoire || !root.activeCasting) return;
                    if (root.draggedFlexiblePreparation) {
                        drop.acceptProposedAction();
                        root.clearFlexiblePreparationRequested(root.activeCasting.id, root.draggedFlexiblePreparation.preparationId);
                    } else if (root.removalSlotId) {
                        drop.acceptProposedAction();
                        root.clearSpellSlotRequested(root.activeCasting.id, root.removalSlotId);
                    }
                }
                Rectangle {
                    anchors.fill: parent
                    visible: root.canDropInRepertoire
                    color: preparationRemovalArea.containsDrag ? "#e0f1f7" : "#f1f5f7"
                    border.color: "#52768e"
                    radius: 5
                    Label {
                        anchors.centerIn: parent
                        width: Math.max(0, parent.width - 16)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: root.draggedFlexiblePreparation ? qsTr("Rilascia qui per rimuovere la preparazione")
                            : qsTr("Rilascia qui per liberare lo slot")
                    }
                }
            }
        }
        Column {
            objectName: "flexiblePreparationGroups"
            visible: root.activeCasting?.mode === SpellcastingsPanel.Flexible
            Layout.fillWidth: true
            Layout.preferredWidth: 280
            Layout.alignment: Qt.AlignTop
            spacing: 8
            Label {
                id: preparationHeading
                objectName: "spellPreparationHeading"
                height: castingColumns.headingHeight
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Preparazioni")
                font.pixelSize: 18; font.bold: true; color: "#253947"
            }
            Label {
                visible: !root.flexiblePreparationLevels.length
                text: qsTr("Nessuna posizione di preparazione.")
                color: "#536976"
            }
            Repeater {
                model: root.flexiblePreparationLevels
                Column {
                    id: preparationLevel
                    required property var modelData
                    objectName: "spellPreparationLevel_" + modelData.level
                    width: parent.width
                    spacing: 5
                    Label { text: qsTr("Livello %1").arg(preparationLevel.modelData.level); font.pixelSize: 16; font.bold: true; color: "#253947" }
                    Repeater {
                        model: preparationLevel.modelData.entries
                        Rectangle {
                            id: preparationCard
                            required property var modelData
                            required property int index
                            objectName: "flexiblePreparation_" + preparationLevel.modelData.level + "_" + index
                            readonly property bool draggingRepertoire: root.preparing && !root.draggedFlexiblePreparation
                            readonly property var choices: draggingRepertoire && !modelData.spellId
                                ? root.flexiblePreparationChoices(modelData.preparationId, root.draggedSpellId) : []
                            readonly property bool acceptsSpell: choices.length > 0
                            width: preparationLevel.width
                            implicitHeight: preparationLabel.implicitHeight + 16
                            opacity: draggingRepertoire && !acceptsSpell ? 0.38 : 1
                            color: acceptsSpell ? (preparationDrop.containsDrag ? "#bcdde9" : "#e0f1f7") : "#fafbfc"
                            border.color: acceptsSpell ? "#52768e" : "#d2dbe1"
                            radius: 5
                            Label {
                                id: preparationLabel
                                x: 10; y: 8
                                width: parent.width - 20
                                text: preparationCard.modelData.spellId ? root.spellName(preparationCard.modelData.spellId) : qsTr("Preparazione %1 · Libera").arg(preparationCard.index + 1)
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                color: "#253947"
                            }
                            DropArea {
                                id: preparationDrop
                                anchors.fill: parent
                                keys: ["prepared-spell"]
                                onEntered: drag => { drag.accepted = preparationCard.acceptsSpell; }
                                onDropped: drop => {
                                    if (!preparationCard.acceptsSpell) return;
                                    drop.acceptProposedAction();
                                    root.requestFlexiblePreparation(preparationCard.modelData.preparationId, root.draggedSpellId, preparationCard.choices);
                                }
                            }
                            DragHandler {
                                id: flexibleCastDrag
                                enabled: !!preparationCard.modelData.spellId
                                target: null
                                onActiveChanged: {
                                    if (active) {
                                        root.dragCastingId = root.activeCasting.id;
                                        root.draggedFlexiblePreparation = preparationCard.modelData;
                                        root.draggedSpellId = preparationCard.modelData.spellId;
                                        flexibleMarker.Drag.active = true;
                                    } else {
                                        if (root.preparing && flexibleMarker.Drag.active) flexibleMarker.Drag.drop();
                                        else flexibleMarker.Drag.cancel();
                                        root.draggedSpellId = "";
                                        root.dragCastingId = "";
                                        root.draggedFlexiblePreparation = null;
                                    }
                                }
                                onCanceled: flexibleMarker.Drag.cancel()
                            }
                            Item {
                                id: flexibleMarker
                                x: flexibleCastDrag.centroid.position.x
                                y: flexibleCastDrag.centroid.position.y
                                Drag.keys: ["flexible-cast"]
                                Drag.supportedActions: Qt.CopyAction
                                Drag.proposedAction: Qt.CopyAction
                            }
                            Rectangle {
                                parent: Overlay.overlay
                                visible: flexibleCastDrag.active && root.preparing
                                z: 1
                                readonly property point pointer: visible && parent
                                    ? preparationCard.mapToItem(parent, flexibleCastDrag.centroid.position.x, flexibleCastDrag.centroid.position.y)
                                    : Qt.point(0, 0)
                                x: pointer.x + 12; y: pointer.y + 10
                                width: flexibleDragLabel.implicitWidth + 20
                                height: flexibleDragLabel.implicitHeight + 12
                                color: "#fafbfc"; border.color: "#52768e"; radius: 5
                                Label { id: flexibleDragLabel; anchors.centerIn: parent; text: preparationLabel.text; textFormat: Text.PlainText }
                            }
                        }
                    }
                }
            }
        }
        Column {
            objectName: "spellSlotGroups"
            Layout.fillWidth: true
            Layout.preferredWidth: root.activeCasting?.mode === SpellcastingsPanel.Flexible ? 280 : 420
            Layout.alignment: Qt.AlignTop
            spacing: 8
            RowLayout {
                id: slotHeadingRow
                width: parent.width
                height: castingColumns.headingHeight
                Label {
                    objectName: "spellSlotHeading"
                    Layout.fillWidth: true
                    text: root.activeCasting?.mode === SpellcastingsPanel.Flexible ? qsTr("Slot di lancio") : qsTr("Slot")
                    font.pixelSize: 18; font.bold: true; color: "#253947"
                }
                Button {
                    text: qsTr("Azzera slot")
                    objectName: "resetSpellSlotsButton"
                    visible: root.activeCasting?.mode === SpellcastingsPanel.Prepared
                        || root.activeCasting?.mode === SpellcastingsPanel.Spontaneous
                        || root.activeCasting?.mode === SpellcastingsPanel.Flexible
                    enabled: !root.preparing && !root.removingPreparation
                        && (root.activeLevels.some(level => level.ready + level.used > 0)
                            || root.flexiblePreparationLevels.some(level => level.entries.some(entry => !!entry.spellId)))
                    ToolTip.visible: hovered && root.activeCasting?.mode === SpellcastingsPanel.Flexible
                    ToolTip.text: qsTr("Rimuove le preparazioni e libera gli slot di lancio")
                    onClicked: root.resetSpellSlotsRequested(root.activeCasting.id)
                }
            }
            Label { visible: !root.activeLevels.length; text: qsTr("Nessuno slot disponibile."); color: "#536976" }
            Repeater {
                model: root.activeLevels
                Column {
                    id: levelCard
                    required property var modelData
                    objectName: "spellSlotLevel_" + modelData.level
                    width: parent.width
                    spacing: 5
                    RowLayout {
                        width: parent.width
                        Label { Layout.fillWidth: true; text: qsTr("Livello %1").arg(levelCard.modelData.level); font.bold: true; font.pixelSize: 16; color: "#253947" }
                        Label {
                            objectName: "spellSlotCount_" + levelCard.modelData.level
                            text: qsTr("%1 / %2 disponibili").arg(levelCard.modelData.free + levelCard.modelData.ready).arg(levelCard.modelData.entries.length)
                            color: "#365f75"
                        }
                    }
                    Repeater {
                        model: levelCard.modelData.entries
                        Rectangle {
                            id: slotCard
                            required property var modelData
                            required property int index
                            objectName: "spellSlot_" + levelCard.modelData.level + "_" + index
                            readonly property bool spontaneous: root.activeCasting?.mode === SpellcastingsPanel.Spontaneous
                            readonly property bool flexible: root.activeCasting?.mode === SpellcastingsPanel.Flexible
                            readonly property bool freeDropTarget: root.preparing && (!flexible || !!root.draggedFlexiblePreparation)
                                && modelData.slot.state === (spontaneous || flexible ? SpellcastingsPanel.CastSlotFree : SpellcastingsPanel.Free)
                            readonly property var metamagicChoices: !freeDropTarget ? [] : flexible
                                ? root.flexibleCastingChoices(modelData.slot, root.draggedFlexiblePreparation)
                                : spontaneous ? root.spontaneousChoices(modelData.slot, root.draggedSpellId)
                                    : root.preparationChoices(modelData.slot.slot, root.draggedSpellId)
                            readonly property bool acceptsPreparation: metamagicChoices.length > 0
                            width: levelCard.width
                            implicitHeight: slotRow.implicitHeight + 16 + (slotRestrictions.visible ? slotRestrictions.implicitHeight + 8 : 0)
                            opacity: !root.preparing || acceptsPreparation ? 1 : 0.38
                            color: acceptsPreparation ? (spellDrop.containsDrag ? "#bcdde9" : "#e0f1f7")
                                : root.slotUsed(modelData.slot) ? "#e9eef1" : "#fafbfc"
                            border.color: acceptsPreparation ? "#52768e" : "#d2dbe1"
                            radius: 5
                            HoverHandler { id: slotHover }
                            DropArea {
                                id: spellDrop
                                anchors.fill: parent
                                keys: slotCard.flexible ? ["flexible-cast"] : ["prepared-spell"]
                                onEntered: drag => { drag.accepted = slotCard.acceptsPreparation; }
                                onDropped: drop => {
                                    if (!slotCard.acceptsPreparation) return;
                                    drop.acceptProposedAction();
                                    if (slotCard.flexible)
                                        root.requestFlexibleCast(slotCard.modelData.slot, root.draggedFlexiblePreparation, slotCard.metamagicChoices);
                                    else if (slotCard.spontaneous)
                                        root.requestSpontaneousCast(slotCard.modelData.slot, root.draggedSpellId, slotCard.metamagicChoices);
                                    else root.requestPreparation(slotCard.modelData.slot.slot, root.draggedSpellId, slotCard.metamagicChoices);
                                }
                            }
                            Item {
                                id: slotRow
                                x: 10; y: 8
                                width: parent.width - 20
                                implicitHeight: Math.max(slotSpellLabel.implicitHeight, slotActions.implicitHeight,
                                    restrictionInfo.visible ? restrictionInfo.implicitHeight : 0)
                                height: implicitHeight
                                Label {
                                    id: slotSpellLabel
                                    objectName: "slotSpellLabel"
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: Math.min(implicitWidth, Math.max(0, slotActions.x - 8 - (restrictionInfo.visible ? 32 : 0)))
                                    text: root.slotName(slotCard.modelData, slotCard.index)
                                    textFormat: Text.PlainText
                                    wrapMode: Text.Wrap
                                    color: "#253947"
                                    ToolTip.visible: spellSlotHover.hovered && !!slotCard.modelData.source
                                    ToolTip.text: slotCard.modelData.source
                                    HoverHandler { id: spellSlotHover }
                                    DragHandler {
                                        id: removeDrag
                                        target: null
                                        enabled: root.activeCasting?.mode === SpellcastingsPanel.Prepared
                                            && slotCard.modelData.slot.state === SpellcastingsPanel.Ready && !!slotCard.modelData.slot.spellId
                                        onActiveChanged: {
                                            if (active) {
                                                root.removalSlotId = slotCard.modelData.slot.slot;
                                                root.removingPreparation = true;
                                                removalGhost.Drag.active = true;
                                            } else {
                                                if (root.removingPreparation && removalGhost.Drag.active) removalGhost.Drag.drop();
                                                else removalGhost.Drag.cancel();
                                                root.removingPreparation = false;
                                                root.removalSlotId = null;
                                            }
                                        }
                                        onCanceled: removalGhost.Drag.cancel()
                                    }
                                    Rectangle {
                                        id: removalGhost
                                        parent: Overlay.overlay
                                        z: 1
                                        visible: removeDrag.active && root.removingPreparation
                                        readonly property point pointer: root.removingPreparation && parent
                                            ? slotSpellLabel.mapToItem(parent, removeDrag.centroid.position.x, removeDrag.centroid.position.y)
                                            : Qt.point(0, 0)
                                        x: pointer.x
                                        y: pointer.y
                                        Drag.keys: ["remove-preparation"]
                                        Drag.supportedActions: Qt.MoveAction
                                        Drag.proposedAction: Qt.MoveAction
                                        width: removalName.implicitWidth + 20
                                        height: removalName.implicitHeight + 12
                                        color: "#fafbfc"
                                        border.color: "#52768e"
                                        radius: 5
                                        Label { id: removalName; x: 10; y: 6; text: slotSpellLabel.text; textFormat: Text.PlainText }
                                    }
                                }
                                InfoButton {
                                    id: restrictionInfo
                                    objectName: "slotRestrictionInfo"
                                    visible: slotCard.modelData.allowedSpellIds !== null
                                    anchors.left: slotSpellLabel.right
                                    anchors.leftMargin: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    opacity: slotHover.hovered || visualFocus ? 1 : 0
                                    Accessible.name: qsTr("Mostra gli incantesimi ammessi")
                                }
                                RowLayout {
                                    id: slotActions
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label { text: root.slotStatus(slotCard.modelData.slot); color: "#536976" }
                                    Button {
                                        objectName: "consumeSpellButton"
                                        visible: root.activeCasting?.mode === SpellcastingsPanel.Prepared
                                            && slotCard.modelData.slot.state === SpellcastingsPanel.Ready
                                            && !!slotCard.modelData.slot.spellId
                                        && !root.isSecondary(slotCard.modelData.slot)
                                        enabled: !root.preparing
                                        text: qsTr("Lancia")
                                        onClicked: root.requestConsumption(slotCard.modelData.slot)
                                    }
                                }
                            }
                            InfoDetails {
                                id: slotRestrictions
                                objectName: "slotRestrictions"
                                x: 10
                                y: slotRow.y + slotRow.height + 8
                                width: slotRow.width
                                visible: restrictionInfo.visible && restrictionInfo.checked
                                rows: root.slotRestrictionRows(slotCard.modelData.allowedSpellIds)
                            }
                        }
                    }
                }
            }
        }
    }
}
