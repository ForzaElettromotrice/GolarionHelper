pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.Sheet

ApplicationWindow {
    id: window
    width: 1100
    height: 860
    minimumWidth: 360
    minimumHeight: 480
    visible: true
    title: qsTr("Golarion — Anteprima pannelli")
    property var featCatalog: []
    property var conditionCatalog: []
    property var actionsSnapshot: ({categories: []})
    property var actionRoutinesSnapshot: ({routines: []})
    property var raceCatalog: []
    property var classCatalog: []
    property var archetypeCatalog: ({})
    property var raceAlternateCatalog: ({})
    property var expressionTargetCatalog: ["bab", "level", "str", "strMod"]
    readonly property var featPickerPopup: standaloneFeats.item?.picker ?? null
    readonly property var homeBleedingExamples: [
        {id: "cut", source: qsTr("Ferita da lama"), parameter: qsTr("3 PF")},
        {id: "claw", source: qsTr("Artiglio"), parameter: qsTr("1d4 PF")}
    ]
    readonly property int homeNegativeLevelCount: 2
    readonly property var homeConditionExamples: {
        const result = [];
        for (const condition of conditionCatalog) {
            const stages = condition.stages ?? [];
            const multiple = stages.length > 1;
            const effects = [];
            for (const stage of stages.length ? stages : [condition]) {
                for (const effect of stage.effects ?? []) effects.push(effect);
                for (const derived of stage.derivedConditions ?? []) {
                    const child = conditionCatalog.find(entry => entry.id === derived.id);
                    const name = child?.stages?.find(entry => entry.severity === derived.severity)?.name ?? child?.name;
                    if (name) effects.push(qsTr("Comporta anche: %1").arg(name));
                }
                const card = {
                    id: multiple ? condition.id + "_" + stage.severity : condition.id,
                    iconId: condition.id,
                    name: stage.name,
                    effects: [...new Set(effects)]
                };
                if (condition.id === "bleeding") {
                    for (const entry of homeBleedingExamples) {
                        result.push(Object.assign({}, card, {
                            id: card.id + "_" + entry.id,
                            name: qsTr("%1 · %2").arg(card.name).arg(entry.parameter),
                            effects: [qsTr("Fonte: %1").arg(entry.source),
                                qsTr("Danno per turno: %1").arg(entry.parameter), ...card.effects]
                        }));
                    }
                } else if (condition.id === "negativeLevels") {
                    result.push(Object.assign({}, card, {
                        name: qsTr("%1 · %2").arg(condition.name).arg(homeNegativeLevelCount),
                        effects: [qsTr("Effetti per ogni livello negativo:"), ...card.effects]
                    }));
                } else {
                    result.push(card);
                }
            }
        }
        return result;
    }

    function reorderPreviewAttack(attackId, beforeAttackId) {
        const current = previewAttacks.attacks.attacks;
        const attack = current.find(entry => entry.id === attackId);
        if (!attack) return;
        const next = current.filter(entry => entry.id !== attackId);
        const destination = beforeAttackId === null ? next.length : next.findIndex(entry => entry.id === beforeAttackId);
        if (destination < 0) return;
        next.splice(destination, 0, attack);
        previewAttacks.attacks = {attacks: next};
    }
    function renamePreviewAttack(attackId, name) {
        previewAttacks.attacks = {attacks: previewAttacks.attacks.attacks.map(attack =>
            attack.id === attackId ? Object.assign({}, attack, {name: name}) : attack)};
    }

    // Already-resolved View fixtures: BAB +11 and a synthetic +4 attack bonus.
    function sampleSwordAttack(bonus) {
        return {strikeName: "Spada lunga", slotName: "Arma", routineAttackBonusAdjustment: 0, progressionAttackBonusAdjustment: bonus - 15,
            strike: {source: "Spada lunga di esempio", usageAttackPenalty: 0, attackAbilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, attackBonus: bonus, criticalConfirmationBonus: bonus}],
                defenseOptions: [{defenseType: ArmorClassPanel.Normal}],
                reach: {minimumUnits: {effectiveValue: 0}, maximumUnits: {effectiveValue: 1}},
                attackModifiers: {modifiers: [], conditionalTotals: [
                    {condition: qsTr("Contro i non morti"), value: 2},
                    {condition: qsTr("Quando sei abbagliato"), value: -1}
                ]},
                criticalConfirmationModifiers: {modifiers: [], conditionalTotals: [{condition: qsTr("Contro i giganti"), value: 2}]},
                damageModifiers: {modifiers: [], conditionalTotals: [{condition: qsTr("Contro i non morti"), value: 1}]},
                damageComponents: [{source: "Spada lunga", effectiveDice: {expression: "1d8"}, types: [DefensesPanel.Slashing], typeMode: AttackDetails.All}],
                criticalProfile: {threatMinimum: 19, multiplier: 2}},
            normalDamage: {abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, abilityContribution: 4, bonus: 4}], components: [
                {occurrences: 1, component: {effectiveDice: {diceCount: 1, dieSize: 8, expression: "1d8"}, types: [DefensesPanel.Slashing], typeMode: AttackDetails.All}}
            ]}, criticalDamage: {abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, abilityContribution: 8, bonus: 8}], components: [
                {occurrences: 2, component: {effectiveDice: {diceCount: 1, dieSize: 8, expression: "1d8"}, types: [DefensesPanel.Slashing], typeMode: AttackDetails.All}}
            ]}};
    }

    // Synthetic natural attacks compatible with the occupied weapon hand.
    function sampleNaturalAttack(name, dieSize, types) {
        return {strikeName: name, slotId: "natural",
            strike: {usageAttackPenalty: -5, attackAbilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, attackBonus: 10, criticalConfirmationBonus: 10}],
                damageComponents: [{source: name, effectiveDice: {expression: "1d" + dieSize}, types: types, typeMode: AttackDetails.All}],
                criticalProfile: {threatMinimum: 20, multiplier: 2}},
            normalDamage: {abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, abilityContribution: 2, bonus: 2}], components: [
                {occurrences: 1, component: {effectiveDice: {diceCount: 1, dieSize: dieSize, expression: "1d" + dieSize}, types: types, typeMode: AttackDetails.All}}
            ]}, criticalDamage: {abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, abilityContribution: 4, bonus: 4}], components: [
                {occurrences: 2, component: {effectiveDice: {diceCount: 1, dieSize: dieSize, expression: "1d" + dieSize}, types: types, typeMode: AttackDetails.All}}
            ]}};
    }

    property var sampleIdentity: ({
        name: "Arianna", playerName: "Giulia", alignment: IdentityPanel.NeutralGood,
        deity: null, homeland: "Sandpoint", gender: "Donna", age: 27,
        heightCentimeters: 172, weightGrams: 64125, hair: "Castani", eyes: "Verdi",
        appearance: "Una cicatrice sottile attraversa il sopracciglio sinistro."
    })

    // Synthetic fixture only; the panel displays the supplied values unchanged.
    property var sampleAbilities: [18, 13, 14, 10, 12, 10].map((score, type) => ({
        type: type, baseValue: score - 2, totalValue: score, modifier: Math.floor((score - 10) / 2),
        modifiers: { total: 2, conditionalTotals: [], modifiers: [
            { type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, source: "Oggetto di esempio", resolvedValue: 4, active: true },
            { type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement, source: "Incantesimo di esempio", resolvedValue: 2, active: true },
            { type: AbilityScoresPanel.Penalty, source: "Affaticamento", resolvedValue: 2, active: true }
        ] }
    }))

    property var sampleAbilityTypes: [1, 5, 3, 1, 5, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 1, 1, 4, 5, 5, 4, 3, 0, 4, 4, 5, 1, 3, 0, 4, 5, 3, 1]
    property var sampleTrainedOnly: [false, true, false, false, false, false, true, true, true, true, true, true, true, true, true, true, false, true, false, false, false, false, false, true, false, false, true, false, true, true, false, false, true, false, false]
    // Representative names from the backend; all numbers below are synthetic.
    property var sampleSkills: [
        { name: "Acrobazia", resourceName: "skill.acrobatics" },
        { name: "Addestrare Animali", resourceName: "skill.handleAnimal" },
        { name: "Artigianato", resourceName: "skill.craft" },
        { name: "Artista della Fuga", resourceName: "skill.escapeArtist" },
        { name: "Camuffare", resourceName: "skill.disguise" },
        { name: "Cavalcare", resourceName: "skill.ride" },
        { name: "Conoscenze (arcane)", resourceName: "skill.knowledge.arcana" },
        { name: "Conoscenze (dungeon)", resourceName: "skill.knowledge.dungeoneering" },
        { name: "Conoscenze (ingegneria)", resourceName: "skill.knowledge.engineering" },
        { name: "Conoscenze (geografia)", resourceName: "skill.knowledge.geography" },
        { name: "Conoscenze (storia)", resourceName: "skill.knowledge.history" },
        { name: "Conoscenze (locali)", resourceName: "skill.knowledge.local" },
        { name: "Conoscenze (natura)", resourceName: "skill.knowledge.nature" },
        { name: "Conoscenze (nobiltà)", resourceName: "skill.knowledge.nobility" },
        { name: "Conoscenze (piani)", resourceName: "skill.knowledge.planes" },
        { name: "Conoscenze (religioni)", resourceName: "skill.knowledge.religion" },
        { name: "Diplomazia", resourceName: "skill.diplomacy" },
        { name: "Disattivare Congegni", resourceName: "skill.disableDevice" },
        { name: "Furtività", resourceName: "skill.stealth" },
        { name: "Guarire", resourceName: "skill.heal" },
        { name: "Intimidire", resourceName: "skill.intimidate" },
        { name: "Intrattenere", resourceName: "skill.perform" },
        { name: "Intuizione", resourceName: "skill.senseMotive" },
        { name: "Linguistica", resourceName: "skill.linguistics" },
        { name: "Nuotare", resourceName: "skill.swim" },
        { name: "Percezione", resourceName: "skill.perception" },
        { name: "Professione", resourceName: "skill.profession" },
        { name: "Raggirare", resourceName: "skill.bluff" },
        { name: "Rapidità di Mano", resourceName: "skill.sleightOfHand" },
        { name: "Sapienza Magica", resourceName: "skill.spellcraft" },
        { name: "Scalare", resourceName: "skill.climb" },
        { name: "Sopravvivenza", resourceName: "skill.survival" },
        { name: "Utilizzare Congegni Magici", resourceName: "skill.useMagicDevice" },
        { name: "Valutare", resourceName: "skill.appraise" },
        { name: "Volare", resourceName: "skill.fly" }
    ].map((entry, index) => Object.assign({}, entry, {
        type: index, ranks: [2, 21, 26].includes(index) ? 0 : index % 4, classSkill: index % 3 === 0,
        trainedOnly: sampleTrainedOnly[index], usable: !sampleTrainedOnly[index] || index % 4 > 0,
        demoGrantedRanks: index === 0 ? 6 : 0, effectiveRanks: index === 0 ? 6 : index % 4,
        classSkillBonus: index % 3 === 0 && (index % 4 > 0 || index === 0) ? 3 : 0,
        appliedArmorCheckPenalty: 0,
        rankGrants: index === 0 ? [{source: "Oggetto di esempio", ranks: 6}] : [],
        abilityOptions: [{abilityType: sampleAbilityTypes[index], abilityModifier: 2, source: "Base",
            totalValue: 2 + index % 4 + (index % 3 === 0 && index % 4 > 0 ? 3 : 0),
            modifiers: {total: 0, modifiers: []}}]
    }))
    // Canonical names mirror the backend; scores remain synthetic demo values.
    property var sampleSpecializations: [
        {type: SkillsPanel.Craft, specializationId: "alchemy", name: "Alchimia", resourceName: "skill.craft.alchemy"},
        {type: SkillsPanel.Craft, specializationId: "armor", name: "Armature", resourceName: "skill.craft.armor"},
        {type: SkillsPanel.Craft, specializationId: "baskets", name: "Ceste", resourceName: "skill.craft.baskets"},
        {type: SkillsPanel.Craft, specializationId: "books", name: "Libri", resourceName: "skill.craft.books"},
        {type: SkillsPanel.Craft, specializationId: "bows", name: "Archi", resourceName: "skill.craft.bows"},
        {type: SkillsPanel.Craft, specializationId: "calligraphy", name: "Calligrafia", resourceName: "skill.craft.calligraphy"},
        {type: SkillsPanel.Craft, specializationId: "carpentry", name: "Carpenteria", resourceName: "skill.craft.carpentry"},
        {type: SkillsPanel.Craft, specializationId: "cloth", name: "Tessitura", resourceName: "skill.craft.cloth"},
        {type: SkillsPanel.Craft, specializationId: "clothing", name: "Sartoria", resourceName: "skill.craft.clothing"},
        {type: SkillsPanel.Craft, specializationId: "glass", name: "Vetreria", resourceName: "skill.craft.glass"},
        {type: SkillsPanel.Craft, specializationId: "jewelry", name: "Oreficeria", resourceName: "skill.craft.jewelry"},
        {type: SkillsPanel.Craft, specializationId: "leather", name: "Lavorare Pellami", resourceName: "skill.craft.leather"},
        {type: SkillsPanel.Craft, specializationId: "locks", name: "Ferramenta", resourceName: "skill.craft.locks"},
        {type: SkillsPanel.Craft, specializationId: "paintings", name: "Pittura", resourceName: "skill.craft.paintings"},
        {type: SkillsPanel.Craft, specializationId: "pottery", name: "Ceramica", resourceName: "skill.craft.pottery"},
        {type: SkillsPanel.Craft, specializationId: "sculptures", name: "Scultura", resourceName: "skill.craft.sculptures"},
        {type: SkillsPanel.Craft, specializationId: "ships", name: "Costruire Navi", resourceName: "skill.craft.ships"},
        {type: SkillsPanel.Craft, specializationId: "shoes", name: "Riparare Scarpe", resourceName: "skill.craft.shoes"},
        {type: SkillsPanel.Craft, specializationId: "stonemasonry", name: "Lavori in Muratura", resourceName: "skill.craft.stonemasonry"},
        {type: SkillsPanel.Craft, specializationId: "traps", name: "Costruire Trappole", resourceName: "skill.craft.traps"},
        {type: SkillsPanel.Craft, specializationId: "weapons", name: "Armi", resourceName: "skill.craft.weapons"},
        {type: SkillsPanel.Perform, specializationId: "singing", name: "Canto", resourceName: "skill.perform.singing"},
        {type: SkillsPanel.Perform, specializationId: "comedy", name: "Commedia", resourceName: "skill.perform.comedy"},
        {type: SkillsPanel.Perform, specializationId: "dance", name: "Danza", resourceName: "skill.perform.dance"},
        {type: SkillsPanel.Perform, specializationId: "oratory", name: "Oratoria", resourceName: "skill.perform.oratory"},
        {type: SkillsPanel.Perform, specializationId: "acting", name: "Recitazione", resourceName: "skill.perform.acting"},
        {type: SkillsPanel.Perform, specializationId: "stringInstruments", name: "Strumenti a Corda", resourceName: "skill.perform.stringInstruments"},
        {type: SkillsPanel.Perform, specializationId: "windInstruments", name: "Strumenti a Fiato", resourceName: "skill.perform.windInstruments"},
        {type: SkillsPanel.Perform, specializationId: "percussionInstruments", name: "Strumenti a Percussione", resourceName: "skill.perform.percussionInstruments"},
        {type: SkillsPanel.Perform, specializationId: "keyboardInstruments", name: "Strumenti a Tastiera", resourceName: "skill.perform.keyboardInstruments"},
        {type: SkillsPanel.Profession, specializationId: "animalBreeder", name: "Allevatore", resourceName: "skill.profession.animalBreeder"},
        {type: SkillsPanel.Profession, specializationId: "architect", name: "Architetto", resourceName: "skill.profession.architect"},
        {type: SkillsPanel.Profession, specializationId: "lawyer", name: "Avvocato", resourceName: "skill.profession.lawyer"},
        {type: SkillsPanel.Profession, specializationId: "boatman", name: "Barcaiolo", resourceName: "skill.profession.boatman"},
        {type: SkillsPanel.Profession, specializationId: "librarian", name: "Bibliotecario", resourceName: "skill.profession.librarian"},
        {type: SkillsPanel.Profession, specializationId: "brewer", name: "Birraio", resourceName: "skill.profession.brewer"},
        {type: SkillsPanel.Profession, specializationId: "forester", name: "Boscaiolo", resourceName: "skill.profession.forester"},
        {type: SkillsPanel.Profession, specializationId: "hunter", name: "Cacciatore", resourceName: "skill.profession.hunter"},
        {type: SkillsPanel.Profession, specializationId: "caravaner", name: "Carovaniere", resourceName: "skill.profession.caravaner"},
        {type: SkillsPanel.Profession, specializationId: "tanner", name: "Conciatore", resourceName: "skill.profession.tanner"},
        {type: SkillsPanel.Profession, specializationId: "constable", name: "Conestabile", resourceName: "skill.profession.constable"},
        {type: SkillsPanel.Profession, specializationId: "accountant", name: "Contabile", resourceName: "skill.profession.accountant"},
        {type: SkillsPanel.Profession, specializationId: "farmer", name: "Contadino", resourceName: "skill.profession.farmer"},
        {type: SkillsPanel.Profession, specializationId: "courtier", name: "Cortigiano", resourceName: "skill.profession.courtier"},
        {type: SkillsPanel.Profession, specializationId: "cook", name: "Cuoco", resourceName: "skill.profession.cook"},
        {type: SkillsPanel.Profession, specializationId: "herbalist", name: "Erborista", resourceName: "skill.profession.herbalist"},
        {type: SkillsPanel.Profession, specializationId: "pharmacist", name: "Farmacista", resourceName: "skill.profession.pharmacist"},
        {type: SkillsPanel.Profession, specializationId: "baker", name: "Fornaio", resourceName: "skill.profession.baker"},
        {type: SkillsPanel.Profession, specializationId: "gardener", name: "Giardiniere", resourceName: "skill.profession.gardener"},
        {type: SkillsPanel.Profession, specializationId: "gambler", name: "Giocatore d'Azzardo", resourceName: "skill.profession.gambler"},
        {type: SkillsPanel.Profession, specializationId: "guide", name: "Guida", resourceName: "skill.profession.guide"},
        {type: SkillsPanel.Profession, specializationId: "engineer", name: "Ingegnere", resourceName: "skill.profession.engineer"},
        {type: SkillsPanel.Profession, specializationId: "midwife", name: "Levatrice", resourceName: "skill.profession.midwife"},
        {type: SkillsPanel.Profession, specializationId: "innkeeper", name: "Locandiere", resourceName: "skill.profession.innkeeper"},
        {type: SkillsPanel.Profession, specializationId: "butcher", name: "Macellaio", resourceName: "skill.profession.butcher"},
        {type: SkillsPanel.Profession, specializationId: "sailor", name: "Marinaio", resourceName: "skill.profession.sailor"},
        {type: SkillsPanel.Profession, specializationId: "merchant", name: "Mercante", resourceName: "skill.profession.merchant"},
        {type: SkillsPanel.Profession, specializationId: "miner", name: "Minatore", resourceName: "skill.profession.miner"},
        {type: SkillsPanel.Profession, specializationId: "miller", name: "Mugnaio", resourceName: "skill.profession.miller"},
        {type: SkillsPanel.Profession, specializationId: "shepherd", name: "Pastore", resourceName: "skill.profession.shepherd"},
        {type: SkillsPanel.Profession, specializationId: "fisherman", name: "Pescatore", resourceName: "skill.profession.fisherman"},
        {type: SkillsPanel.Profession, specializationId: "scribe", name: "Scrivano", resourceName: "skill.profession.scribe"},
        {type: SkillsPanel.Profession, specializationId: "steward", name: "Siniscalco", resourceName: "skill.profession.steward"},
        {type: SkillsPanel.Profession, specializationId: "soldier", name: "Soldato", resourceName: "skill.profession.soldier"},
        {type: SkillsPanel.Profession, specializationId: "stableMaster", name: "Stalliere", resourceName: "skill.profession.stableMaster"},
        {type: SkillsPanel.Profession, specializationId: "woodcutter", name: "Taglialegna", resourceName: "skill.profession.woodcutter"}
    ].map(entry => Object.assign({}, entry, {ranks: 0, effectiveRanks: 0, classSkill: false,
        trainedOnly: entry.type === SkillsPanel.Profession, usable: entry.type !== SkillsPanel.Profession,
        classSkillBonus: 0, appliedArmorCheckPenalty: 0, rankGrants: [],
        abilityOptions: [{abilityType: sampleAbilityTypes[entry.type], abilityModifier: 0, source: "Base",
            totalValue: 0, modifiers: {total: 0, modifiers: []}}]}))
    property int nextSampleSpecializationId: 1

    function detailedSampleSkills() {
        return sampleSkills.concat(sampleSpecializations).map(entry => {
            // Detailed synthetic fixture for inspecting the panel, not rule evaluation.
            if (entry.resourceName === "skill.perception") {
                return Object.assign({}, entry, {abilityOptions: entry.abilityOptions.map(option => Object.assign({}, option, {
                    modifiers: Object.assign({}, option.modifiers, {conditionalTotals: [{condition: qsTr("Per individuare trappole"), value: 2}]})
                }))});
            }
            if (entry.resourceName !== "skill.acrobatics") return entry;
            return Object.assign({}, entry, {classSkillBonus: 3, appliedArmorCheckPenalty: 1,
                abilityOptions: [{abilityType: AbilityScoresPanel.Dexterity, abilityModifier: 2,
                    source: "Base", totalValue: 12, modifiers: {total: 2, conditionalTotals: [
                        {condition: qsTr("Per saltare"), value: 2}, {condition: qsTr("Su superfici scivolose"), value: -2}
                    ], modifiers: [
                        {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Enhancement,
                            source: "Effetto di esempio", resolvedValue: 2, active: true}
                    ]}}]});
        });
    }

    // Background clicks end text editing without intercepting other controls.
    TapHandler {
        function handlesOutsideClicks(item) { return item.handlesOutsideClicks === true; }
        onPressedChanged: {
            const editor = window.activeFocusItem as TextField;
            if (!pressed || !editor) return;
            if (handlesOutsideClicks(editor)) return;
            const position = editor.mapFromItem(parent, point.position.x, point.position.y);
            if (!editor.contains(position)) editor.focus = false;
        }
    }

    header: ToolBar {
        ColumnLayout {
            width: parent.width
            Label { text: qsTr("Anteprima con dati di esempio · nessun salvataggio"); Layout.fillWidth: true; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: panelSelector
                    model: [qsTr("Identità"), qsTr("Caratteristiche"), qsTr("Abilità"), qsTr("Tiri salvezza"), qsTr("Punti ferita"), qsTr("Classe Armatura"), qsTr("Iniziativa"), qsTr("Movimento"), qsTr("Attacchi"), qsTr("Manovre"), qsTr("Lingue"), qsTr("Talenti"), qsTr("Razza"), qsTr("Classi"), qsTr("Inventario"), qsTr("Monete"), qsTr("Incantesimi"), qsTr("Incantesimi concessi"), qsTr("Potenziamenti"), qsTr("Note"), qsTr("Condizioni"), qsTr("Promemoria"), qsTr("Scelte aperte"), qsTr("Azioni"), qsTr("Scheda · Home")]
                    currentIndex: 24
                    objectName: "panelSelector"
                    Accessible.name: qsTr("Pannello da visualizzare")
                }
                Item { Layout.fillWidth: true }
                CheckBox { id: readOnlyToggle; text: qsTr("Sola lettura"); visible: panelSelector.currentIndex === 0 }
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: panelSelector.currentIndex === 24
        sourceComponent: Component {
            SheetView {
                id: previewSheet
                objectName: "homePreview"
                currentTab: 1
                identity: window.sampleIdentity
                notes: ""
                onAttackRoutineRenameRequested: (attackId, name) => window.renamePreviewAttack(attackId, name)
                onAttackRoutineReorderRequested: (attackId, beforeAttackId) => window.reorderPreviewAttack(attackId, beforeAttackId)
                raceTabPage: racePreviewPanel
                spellsTabPage: Component {
                    Column {
                        spacing: 24
                        property alias castingArea: castingArea
                        property alias grantsArea: grantsArea
                        Loader { id: castingArea; width: parent.width; sourceComponent: castingPreviewPage }
                        Loader { id: grantsArea; width: parent.width; sourceComponent: grantedPreviewPage }
                    }
                }
                featsTabPage: Component {
                    TalentsTab {
                        languagesPage: languagesPreview
                        featsPage: featsPreview
                        tabForChoice: type => previewSheet.tabForChoice(type)
                        onOpenChoiceRequested: type => previewSheet.openChoiceTab(type)
                        reminders: ({messages: [qsTr("Ricorda di aggiornare le note della scheda.")], choices: [
                            {type: PendingChoicesPanel.RacialOption, description: qsTr("Completa la scelta razziale"), remaining: 1},
                            {type: PendingChoicesPanel.Language, description: qsTr("Puoi imparare un’altra lingua grazie all’Intelligenza."), remaining: 1},
                            {type: PendingChoicesPanel.FeatSlot, description: qsTr("Scegli il talento di livello 3 prima di avanzare."), remaining: 1}
                        ]})
                    }
                }
                inventoryTabPage: Component {
                    InventoryPanel {
                        objectName: "sheetInventoryPreview"
                        inventory: inventoryPanel.inventory
                        encumbrance: inventoryPanel.encumbrance
                        onAddContainerRequested: definition => inventoryPanel.addContainerRequested(definition)
                        onAddCatalogItemRequested: inventoryPanel.addCatalogItemRequested()
                        onAddCustomItemRequested: definition => inventoryPanel.addCustomItemRequested(definition)
                        onMoveItemRequested: (itemId, containerId) => inventoryPanel.moveItemRequested(itemId, containerId)
                        onItemQuantityChangeRequested: (itemId, quantity) => inventoryPanel.itemQuantityChangeRequested(itemId, quantity)
                        onMergeCoinStacksRequested: (sourceId, targetId) => inventoryPanel.mergeCoinStacksRequested(sourceId, targetId)
                        onSplitCoinStackRequested: (itemId, quantity) => inventoryPanel.splitCoinStackRequested(itemId, quantity)
                        onRestoreItemUsagePoolRequested: (itemId, poolId, amount) => inventoryPanel.restoreItemUsagePoolRequested(itemId, poolId, amount)
                        onRefillItemUsagePoolRequested: (itemId, poolId) => inventoryPanel.refillItemUsagePoolRequested(itemId, poolId)
                        onReorderItemRequested: (itemId, beforeId) => inventoryPanel.reorderItemRequested(itemId, beforeId)
                        onMoveAndReorderItemRequested: (itemId, containerId, beforeId) => inventoryPanel.moveAndReorderItemRequested(itemId, containerId, beforeId)
                        onEquipItemRequested: (itemId, replacedId, sourceId) => inventoryPanel.equipItemRequested(itemId, replacedId, sourceId)
                        onUnequipItemRequested: (itemId, containerId) => inventoryPanel.unequipItemRequested(itemId, containerId)
                        encumbranceBelowItems: true
                        equipmentFooter: Component {
                            Column {
                                spacing: 8
                                MoneyPanel {
                                    objectName: "sheetInventoryMoneyPreview"
                                    width: parent.width
                                    money: inventoryPanel.inventory?.money
                                    containers: inventoryPanel.inventory?.containers ?? []
                                    onAddMoneyRequested: (denomination, quantity, containerId) => moneyPreview.addMoneyRequested(denomination, quantity, containerId)
                                    onConvertMoneyRequested: (from, to, fromQuantity, toQuantity, sourceId, destinationId) =>
                                        moneyPreview.convertMoneyRequested(from, to, fromQuantity, toQuantity, sourceId, destinationId)
                                }
                                Label {
                                    width: parent.width
                                    visible: text.length > 0
                                    text: [inventoryRequest.text, moneyRequest.text].filter(Boolean).join("\n")
                                    wrapMode: Text.Wrap
                                }
                            }
                        }
                    }
                }
                combat: ({
                    attacks: previewAttacks.attacks, attackRoutines: previewAttacks.attackRoutines,
                    actions: previewAttacks.actions, baseAttackBonus: previewAttacks.baseAttackBonus,
                    combatManeuvers: previewManeuvers.combatManeuvers,
                    listedActions: window.actionsSnapshot, listedActionRoutines: window.actionRoutinesSnapshot
                })
                home: ({
                    abilities: window.sampleAbilities, skills: skillsPreview.skills,
                    spentRanks: skillsPreview.spentRanks, maxRanks: skillsPreview.maxRanks,
                    savingThrows: previewSaves.savingThrows, initiative: previewInitiative.initiative,
                    movement: previewMovement.movement,
                    activeConditions: window.homeConditionExamples,
                    armorClass: previewArmor.armorClass, hitPoints: previewHitPoints.hitPoints,
                    defenses: healthPreview.sampleDefenses
                })
            }
        }
    }

    ScrollView {
        id: previewScroll
        visible: panelSelector.currentIndex !== 24
        anchors.fill: parent
        contentWidth: availableWidth
        Column {
            width: previewScroll.availableWidth
            IdentityPanel {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 0
                identity: window.sampleIdentity
                readOnly: readOnlyToggle.checked
                onIdentityChangeRequested: proposedIdentity => { window.sampleIdentity = proposedIdentity; }
            }
            AbilityScoresPanel {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 1
                abilities: window.sampleAbilities
                // Fixture-only acceptance with a fixed +2 contribution.
                onBaseValueChangeRequested: (type, value) => {
                    window.sampleAbilities = window.sampleAbilities.map(entry => entry.type !== type ? entry : ({
                        type: type, baseValue: value, totalValue: value + 2,
                        modifier: Math.floor((value + 2 - 10) / 2), modifiers: entry.modifiers
                    }));
                }
            }
            SkillsPanel {
                id: skillsPreview
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 2
                skills: window.detailedSampleSkills()
                spentRanks: window.sampleSkills.concat(window.sampleSpecializations).reduce((total, entry) => total + (entry.ranks ?? 0), 0)
                maxRanks: 60
                onSpecializationRemoveRequested: (type, id) => {
                    window.sampleSkills = window.sampleSkills.filter(entry => !(entry.custom === true && entry.type === type && entry.specializationId === id));
                }
                onRanksChangeRequested: (resource, ranks) => {
                    // Synthetic acceptance only: real totals must come from the sheet.
                    const update = entries => entries.map(entry => entry.resourceName !== resource ? entry : Object.assign({}, entry, {
                        ranks: ranks, effectiveRanks: Math.max(ranks, entry.demoGrantedRanks ?? 0)
                    }));
                    window.sampleSkills = update(window.sampleSkills);
                    window.sampleSpecializations = update(window.sampleSpecializations);
                }
                onSpecializationAddRequested: (type, name) => {
                    // In-memory preview only; real IDs and validation belong to the controller/backend.
                    const id = "preview_" + window.nextSampleSpecializationId++;
                    const resource = type === SkillsPanel.Craft ? "skill.craft." : type === SkillsPanel.Perform ? "skill.perform." : "skill.profession.";
                    window.sampleSkills = window.sampleSkills.concat([{
                        type: type, specializationId: id, resourceName: resource + id, name: name, custom: true,
                        ranks: 0, effectiveRanks: 0, classSkill: false, usable: type !== SkillsPanel.Profession,
                        trainedOnly: type === SkillsPanel.Profession, classSkillBonus: 0, appliedArmorCheckPenalty: 0,
                        abilityOptions: [{abilityType: window.sampleAbilityTypes[type], abilityModifier: 0,
                            source: "Base", totalValue: 0, modifiers: {total: 0, modifiers: []}}]
                    }]);
                }
            }
            Column {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 4
                id: healthPreview
                readonly property var sampleEnergies: [
                    DefensesPanel.Fire, DefensesPanel.Cold, DefensesPanel.Electricity, DefensesPanel.Acid,
                    DefensesPanel.Sonic, DefensesPanel.Force, DefensesPanel.PositiveEnergy, DefensesPanel.NegativeEnergy
                ]
                property var sampleDefenses: ({
                        immunities: ["Veleno", "Malattie", "Sonno", "Paralisi", "Paura", "Stordimento", "Affaticamento"].map(name => ({name: name, grants: []})),
                        damageReductions: [
                            {effectiveValue: 15, bypass: {anyOf: [{allOf: [DefensesPanel.Magic]}]}, applicability: null},
                            {effectiveValue: 10, bypass: {anyOf: [{allOf: [DefensesPanel.Silver]}]}, applicability: null},
                            {effectiveValue: 10, bypass: {anyOf: [{allOf: [DefensesPanel.ColdIron]}]}, applicability: null},
                            {effectiveValue: 8, bypass: {anyOf: [{allOf: [DefensesPanel.Adamantine]}]}, applicability: null},
                            {effectiveValue: 5, bypass: {anyOf: [{allOf: [DefensesPanel.Good]}]}, applicability: null},
                            {effectiveValue: 5, bypass: {anyOf: [{allOf: [DefensesPanel.Magic, DefensesPanel.Silver]}]}, applicability: null},
                            {effectiveValue: 3, bypass: {anyOf: []}, applicability: null}
                        ],
                        energyResistances: sampleEnergies.map((energy, index) => ({energy: energy, effectiveValue: index === 0 ? 10 : 5})),
                        energyVulnerabilities: sampleEnergies.map(energy => ({energy: energy, grants: []}))
                    })
                HitPointsPanel {
                    id: previewHitPoints
                    width: parent.width
                    belowHeader: Component {
                        Column {
                            spacing: 8
                            DefensesPanel {
                                width: parent.width
                                defenses: healthPreview.sampleDefenses
                                presentation: DefensesPanel.Tags
                            }
                            DefensesPanel {
                                width: parent.width
                                defenses: healthPreview.sampleDefenses
                                presentation: DefensesPanel.Shields
                            }
                        }
                    }
                    hitPoints: ({baseMax: 0, max: 32, current: 24, nonLethal: 3,
                        maxContributions: {contributions: [{id: "Dadi vita", resolvedValue: 24}, {id: "Costituzione", resolvedValue: 8}]},
                        temporary: {total: 5, pools: [{id: "Incantesimo di esempio", remaining: 5, duration: null}]}})
                    onDamageRequested: (amount, lethality) => hpRequest.text = qsTr("Richiesta danni: ") + amount + (lethality === HitPointsPanel.NonLethal ? qsTr(" non letali") : "")
                    onHealRequested: amount => hpRequest.text = qsTr("Richiesta cura: ") + amount
                    onTemporaryAddRequested: (sourceId, amount) => hpRequest.text = qsTr("Richiesta PF temporanei: ") + amount + " — " + sourceId
                    onTemporaryRemoveRequested: sourceId => hpRequest.text = qsTr("Richiesta rimozione: ") + sourceId
                }
                Label { id: hpRequest; width: parent.width; wrapMode: Text.Wrap; text: qsTr("Demo: i comandi mostrano la richiesta, senza modificare i PF.") }
            }
            Column {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 8
                AttacksPanel {
                    id: previewAttacks
                    width: parent.width
                    baseAttackBonus: ({total: 11})
                    attacks: ({attacks: [
                        {id: "standard", name: "Attacco singolo", actionId: "demo.standard", complete: true, slots: [
                            {id: "strike", name: "Attacco", selectionMode: AttacksPanel.ChooseOne,
                                assignedGrantId: "sword", effectiveGrantIds: ["sword"], candidates: [
                                    {grantId: "sword", name: "Spada lunga", accepted: true}, {grantId: "dagger", name: "Pugnale", accepted: true},
                                    {grantId: "bite", name: "Morso", accepted: true}]}
                        ], calculatedAttacks: [window.sampleSwordAttack(15)]},
                        {id: "full", name: "Attacco completo", actionId: "demo.full", slots: [
                            {id: "weapon", name: "Arma", selectionMode: AttacksPanel.ChooseOne,
                                assignedGrantId: "sword", effectiveGrantIds: ["sword"], candidates: [
                                    {grantId: "sword", name: "Spada lunga", accepted: true}, {grantId: "dagger", name: "Pugnale", accepted: true},
                                    {grantId: "bite", name: "Morso", accepted: false}]},
                            {id: "natural", name: "Attacchi naturali", selectionMode: AttacksPanel.AllMatching,
                                effectiveGrantIds: ["bite", "tail"], candidates: [
                                    {grantId: "bite", name: "Morso"}, {grantId: "tail", name: "Colpo di coda"}
                                ]}
                        ], calculatedAttacks: [window.sampleSwordAttack(15), window.sampleSwordAttack(10), window.sampleSwordAttack(5),
                            window.sampleNaturalAttack("Morso", 6, [DefensesPanel.Bludgeoning, DefensesPanel.Piercing, DefensesPanel.Slashing]),
                            window.sampleNaturalAttack("Colpo di coda", 8, [DefensesPanel.Bludgeoning])]}
                    ]})
                    actions: ({categories: [{actions: [
                        {id: "demo.standard", effectiveCost: ActionCostIcon.Standard},
                        {id: "demo.full", effectiveCost: ActionCostIcon.FullRound, usable: true, inhibitions: []}
                    ]}]})
                    attackRoutines: ({routines: [
                        {id: "base.standardAttack", name: "Attacco singolo", usable: true},
                        {id: "base.fullAttack", name: "Attacco completo", usable: true, failureReasons: []},
                        {id: "base.twoWeaponFighting", name: "Combattere con due armi", usable: true, failureReasons: []}
                    ]})
                    onAddRequested: routineId => {
                        routineRequest.text = qsTr("Richiesta di creazione: ") + routineId + qsTr(". Demo: la scheda non viene modificata.");
                        routineRequest.visible = true;
                    }
                    onRemoveRequested: attackId => {
                        routineRequest.text = qsTr("Richiesta di rimozione: ") + attackId + qsTr(". Demo: la scheda non viene modificata.");
                        routineRequest.visible = true;
                    }
                    onReorderRequested: (attackId, beforeAttackId) => window.reorderPreviewAttack(attackId, beforeAttackId)
                    onRenameRequested: (attackId, name) => window.renamePreviewAttack(attackId, name)
                    onAssignRequested: (attackId, slotId, strikeGrantId) => {
                        routineRequest.text = qsTr("Richiesta di assegnazione: ") + strikeGrantId + " → " + attackId + "/" + slotId
                            + qsTr(". Demo: la scheda non viene modificata.");
                        routineRequest.visible = true;
                    }
                }
                Label {
                    id: routineRequest
                    visible: false
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: ""
                }
            }
            Loader {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 10
                active: visible
                sourceComponent: languagesPreview
            }
            Component {
                id: languagesPreview
                Column {
                    property bool prominentHeading: false
                    spacing: 8
                    LanguagesPanel {
                        prominentHeading: parent.prominentHeading
                        width: parent.width
                        languageNames: ({common: "Comune", elven: "Elfico", draconic: "Draconico", dwarven: "Nanico", orc: "Orchesco", sylvan: "Silvano"})
                        extraLanguageIds: ["common", "elven", "draconic", "dwarven", "orc", "sylvan"]
                        languages: ({spoken: [
                            {id: "common", grantedSources: ["Razza di esempio"], learnedFrom: null},
                            {id: "elven", grantedSources: [], learnedFrom: LanguagesPanel.Intelligence},
                            {id: "draconic", grantedSources: [], learnedFrom: LanguagesPanel.Linguistics}
                        ], remainingIntelligence: 1, remainingLinguistics: 2,
                        options: [
                            {origin: LanguagesPanel.Intelligence, languageIds: ["common", "elven", "dwarven", "orc"]},
                            {origin: LanguagesPanel.Linguistics, languageIds: ["common", "draconic", "elven", "dwarven", "orc", "sylvan"]}
                        ]})
                        onAddRequested: (languageId, origin) => languageRequest.text = qsTr("Richiesta di apprendimento: ") + languageName(languageId)
                            + " (" + originName(origin) + qsTr("). Demo: la scheda non viene modificata.")
                        onGrantRequested: languageId => languageRequest.text = qsTr("Richiesta lingua extra: ") + languageName(languageId)
                            + qsTr(". Nessun posto consumato. Demo: la scheda non viene modificata.")
                        onCustomLearnRequested: name => languageRequest.text = qsTr("Richiesta lingua personalizzata da Linguistica: ") + name
                            + qsTr(". Consuma un posto. Demo: la scheda non viene modificata.")
                        onCustomGrantRequested: name => languageRequest.text = qsTr("Richiesta lingua extra personalizzata: ") + name
                            + qsTr(". Nessun posto consumato. Demo: la scheda non viene modificata.")
                        onRemoveRequested: languageId => languageRequest.text = qsTr("Richiesta di rimozione: ") + languageName(languageId)
                            + qsTr(". Demo: la scheda non viene modificata.")
                    }
                    Label { id: languageRequest; width: parent.width; wrapMode: Text.Wrap; text: ""; visible: text.length > 0 }
                }
            }
            Loader {
                id: standaloneFeats
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 11
                active: visible
                sourceComponent: featsPreview
            }
            Component {
                id: featsPreview
                Column {
                    id: featsSection
                    property bool prominentHeading: false
                    readonly property alias picker: featPicker
                    spacing: 8
                    property string acquisitionLabel: ""
                    property string targetSlotId: ""
                    property bool extraAcquisition: false
                    FeatsPanel {
                        id: featsPanel
                        prominentHeading: featsSection.prominentHeading
                        width: parent.width
                        feats: ({slots: [
                            {id: "level3", source: "Livello 3", available: true, origin: FeatsPanel.Source, featId: null},
                            {id: "level1", source: "Livello 1", available: true, origin: FeatsPanel.Source, featId: "initiative"},
                            {id: "fighter", source: "Guerriero · talento bonus", available: true, origin: FeatsPanel.Source, allowedType: FeatsPanel.Combat, featId: "focus"},
                            {id: "extra", source: "Talento extra", available: true, origin: FeatsPanel.Extra, featId: "custom"}
                        ], feats: [
                            {slotId: "level1", id: "initiative", name: "Iniziativa Migliorata", description: "Descrizione di esempio: migliora la capacità di reagire all’inizio del combattimento.", types: [FeatsPanel.Combat], choices: [], active: true},
                            {slotId: "fighter", id: "focus", name: "Arma Focalizzata", description: "Descrizione di esempio: beneficio legato all’arma scelta.", types: [FeatsPanel.Combat], active: false,
                                choices: [{id: "weapon", prompt: "Arma scelta", selectedOptions: [{id: "sword", name: "Spada lunga"}]}]},
                            {slotId: "extra", id: "custom", name: "Talento personalizzato di esempio", description: "Talento extra, acquisito senza occupare uno slot ordinario né controllare i prerequisiti.", types: [FeatsPanel.General], choices: [], active: true}
                        ]})
                        onSelectRequested: slotId => {
                            featsSection.targetSlotId = slotId;
                            featsSection.extraAcquisition = false;
                            featsSection.acquisitionLabel = qsTr("Scelta per lo slot: ") + slotId;
                            featPicker.open();
                        }
                        onExtraRequested: {
                            featsSection.targetSlotId = "";
                            featsSection.extraAcquisition = true;
                            featsSection.acquisitionLabel = qsTr("Talento extra · senza slot né prerequisiti");
                            featPicker.open();
                        }
                        onRemoveRequested: slotId => featRequest.text = qsTr("Richiesta rimozione talento da ") + slotId + qsTr("; lo slot rimane.")
                        onRemoveExtraSlotRequested: slotId => featRequest.text = qsTr("Richiesta rimozione slot extra e talento: ") + slotId
                    }
                    FeatCatalogPanel {
                        id: featPicker
                        objectName: "featPicker"
                        catalog: window.featCatalog
                        acquisitionLabel: featsSection.acquisitionLabel
                        slotId: featsSection.targetSlotId
                        extra: featsSection.extraAcquisition
                        onAddRequested: (slotId, featId) => {
                            featRequest.text = qsTr("Richiesta aggiunta: ") + featId + " → " + slotId
                                + qsTr(". Demo: acquisizione e scelte da collegare al controller.");
                        }
                        onExtraRequested: featId => {
                            featRequest.text = qsTr("Richiesta talento extra: ") + featId
                                + qsTr(". Demo: acquisizione e scelte da collegare al controller.");
                        }
                    }
                    Label { id: featRequest; width: parent.width; wrapMode: Text.Wrap; text: ""; visible: text.length > 0 }
                }
            }
            Loader {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 12
                active: visible
                sourceComponent: racePreviewPanel
            }
            Component {
                id: racePreviewPanel
                Column {
                    spacing: 10
                    RacePanel {
                        objectName: "racePanel"
                        width: parent.width
                        catalog: window.raceCatalog
                        alternateFeatures: race?.race ? window.raceAlternateCatalog[race.race.id] ?? [] : []
                        onSetRaceRequested: raceId => raceRequest.text = qsTr("Richiesta cambio razza: ") + raceId
                            + qsTr(". Demo: la scheda non viene modificata.")
                        onSelectAlternateTraitRequested: featureId => raceRequest.text = qsTr("Richiesta selezione tratto alternativo: ") + featureId
                            + qsTr(". Demo: la scheda non viene modificata.")
                        onRemoveAlternateTraitRequested: featureId => raceRequest.text = qsTr("Richiesta rimozione tratto alternativo: ") + featureId
                            + qsTr(". Demo: la scheda non viene modificata.")
                        onChangeChoiceRequested: (elementId, choiceId, optionIds) => raceRequest.text = qsTr("Richiesta scelta razziale: ") + elementId + "/" + choiceId
                            + " → " + optionIds.join(", ") + qsTr(". Demo: la scheda non viene modificata.")
                        race: ({race: {id: "human", name: "Umano"}, activeElements: [
                            {id: "human.type", name: "Tipo", description: "Umanoide con sottotipo Umano.", choices: [], missingChoices: []},
                            {id: "human.size", name: "Taglia Media", description: "Nessun bonus o penalità derivante dalla taglia.", choices: [], missingChoices: []},
                            {id: "human.baseSpeed", name: "Velocità Normale", description: "La velocità base sul terreno è 9 metri.", choices: [], missingChoices: []},
                            {id: "human.dualTalent", name: "Doppia Dote", description: "Seleziona due punteggi di caratteristica e ottieni bonus razziale +2 a entrambi.", removable: true,
                                choices: [], missingChoices: [{id: "abilities", prompt: "Scegli due caratteristiche diverse", blocksAdvancement: true,
                                    selectionCount: 2, options: [{id: "strength", name: "Forza"}, {id: "dexterity", name: "Destrezza"},
                                    {id: "constitution", name: "Costituzione"}, {id: "intelligence", name: "Intelligenza"},
                                    {id: "wisdom", name: "Saggezza"}, {id: "charisma", name: "Carisma"}]}]}
                        ]})
                    }
                    Label { id: raceRequest; width: parent.width; wrapMode: Text.Wrap; text: ""; visible: text.length > 0 }
                }
            }
            Column {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 13
                spacing: 10
            ClassesPanel {
                id: classPanel
                width: parent.width
                catalog: window.classCatalog
                archetypeCatalog: window.archetypeCatalog
                onAddArchetypeRequested: (classType, archetypeId) => classRequest.text = qsTr("Richiesta archetipo: %1 / %2").arg(classType).arg(archetypeId)
                onRemoveArchetypeRequested: (classType, archetypeId) => classRequest.text = qsTr("Richiesta rimozione archetipo: %1 / %2").arg(classType).arg(archetypeId)
                onFavoredClassRequested: (classType, favored) => classRequest.text = qsTr("Richiesta classe preferita: %1 (%2)").arg(classType).arg(favored ? qsTr("seleziona") : qsTr("rimuovi"))
                onHitDieResultRequested: (classType, classLevel, result) => classRequest.text = qsTr("Richiesta PF: classe %1, livello %2, risultato %3").arg(classType).arg(classLevel).arg(result)
                onFavoredBonusRequested: (classType, classLevel, optionId) => classRequest.text = qsTr("Richiesta bonus classe preferita: %1, livello %2, %3").arg(classType).arg(classLevel).arg(optionId)
                onAddLevelRequested: classType => classRequest.text = qsTr("Richiesta +1 livello alla classe: %1").arg(classType)
                onRemoveLevelRequested: classType => classRequest.text = qsTr("Richiesta −1 livello alla classe: %1").arg(classType)
                onSelectOptionRequested: (classType, acquisitionId, choiceId, optionId) => classRequest.text = qsTr("Richiesta selezione: %1 / %2 / %3").arg(acquisitionId).arg(choiceId).arg(optionId)
                onUnselectOptionRequested: (classType, acquisitionId, choiceId, optionId) => classRequest.text = qsTr("Richiesta rimozione: %1 / %2 / %3").arg(acquisitionId).arg(choiceId).arg(optionId)
                classes: ({level: 5, favoredClasses: {selectedClasses: [ClassesPanel.Wizard], classes: [
                    {type: ClassesPanel.Wizard, favored: true, selections: [{classLevel: 1, optionId: "example.both", active: true}, {classLevel: 2, optionId: "base.hitPoint", active: true}], options: [
                        {id: "base.hitPoint", name: "+1 PF", description: "Un punto ferita aggiuntivo.", available: true, benefitTypes: [ClassesPanel.HitPoint], selectionsPerBenefit: 1, selections: 1},
                        {id: "base.skillRank", name: "+1 grado", description: "Un grado di abilità aggiuntivo.", available: true, benefitTypes: [ClassesPanel.SkillRank], selectionsPerBenefit: 1, selections: 0},
                        {id: "example.both", name: "PF e grado (esempio)", description: "Esempio di opzione concessa da un talento: entrambi i benefici.", available: true, benefitTypes: [ClassesPanel.HitPoint, ClassesPanel.SkillRank], selectionsPerBenefit: 1, selections: 1},
                        {id: "example.other", name: "Bonus alternativo (esempio)", description: "Un beneficio particolare descritto dall’opzione.", available: true, benefitTypes: [ClassesPanel.Other], selectionsPerBenefit: 3, selections: 0}
                    ]}, {type: ClassesPanel.Rogue, favored: false, selections: [], options: []}
                ]}, classes: [
                    {type: ClassesPanel.Wizard, name: "Mago", level: 3, hitDie: 6, hitDieResults: {1: 6, 2: 4}, archetypes: [{id: "preview.example", name: "Archetipo di esempio", description: "Descrizione dimostrativa dell’archetipo selezionato."}], features: [
                        {id: "wizard.bond", name: "Legame Arcano", description: "Descrizione di esempio del legame arcano.", acquisitions: [
                            {id: "bond1", level: 1, active: true, choices: [{id: "bond", prompt: "Legame scelto", selectionCount: 1, remainingSelections: 0,
                                options: [{id: "familiar", name: "Famiglio", description: "Opzione selezionata nell’esempio.", selected: true}]}]}
                        ]},
                        {id: "wizard.school", name: "Scuola Arcana", description: "Descrizione di esempio della specializzazione del mago.", acquisitions: [
                            {id: "school1", level: 1, active: true, choices: [{id: "school", prompt: "Scegli la scuola arcana", selectionCount: 1, remainingSelections: 1, blocksAdvancement: true,
                                options: [{id: "abjuration", name: "Abiurazione", selected: false}, {id: "conjuration", name: "Evocazione", selected: false}]}]}
                        ]},
                        {id: "wizard.future", name: "Privilegio futuro", description: "Non deve apparire tra i privilegi ottenuti.", acquisitions: [{id: "future5", level: 5, active: false, choices: []}]}
                    ]},
                    {type: ClassesPanel.Rogue, name: "Ladro", level: 2, hitDie: 8, hitDieResults: {1: 5}, archetypes: [], features: [
                        {id: "rogue.sneakAttack", name: "Attacco Furtivo", description: "Descrizione di esempio del privilegio di attacco furtivo.", acquisitions: [{id: "sneak1", level: 1, active: true, choices: []}]},
                        {id: "rogue.talent", name: "Dote da Ladro", description: "Descrizione di esempio delle doti da ladro.", acquisitions: [
                            {id: "talent2", level: 2, active: true, choices: [{id: "talent", prompt: "Scegli una dote", selectionCount: 1, remainingSelections: 1, blocksAdvancement: true,
                                options: [{id: "example", name: "Dote di esempio", selected: false}]}]}
                        ]}
                    ]}
                ]})
            }
                Label { id: classRequest; width: parent.width; wrapMode: Text.Wrap; visible: !!text }
            }
            Column {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 14
                spacing: 10
            InventoryPanel {
                id: inventoryPanel
                objectName: "inventoryPreview"
                width: parent.width
                encumbrance: ({totalWeightGrams: inventory?.totalCarriedWeightGrams, lightLoadMaxGrams: 10000,
                    mediumLoadMaxGrams: 20000, heavyLoadMaxGrams: 30000, category: InventoryPanel.Medium,
                    effects: {maximumDexterityBonus: 3, armorCheckPenalty: 3, reducesMovement: true}})
                function updateUsagePool(itemId, poolId, change) {
                    inventory = Object.assign({}, inventory, {items: inventory.items.map(item => item.id !== itemId ? item :
                        Object.assign({}, item, {usagePools: item.usagePools.map(pool => pool.id !== poolId ? pool :
                            Object.assign({}, pool, {remaining: Math.max(0, Math.min(pool.capacity, change(pool)))})
                        )})
                    )});
                }
                onRestoreItemUsagePoolRequested: (itemId, poolId, amount) =>
                    updateUsagePool(itemId, poolId, pool => pool.remaining + amount)
                onRefillItemUsagePoolRequested: (itemId, poolId) =>
                    updateUsagePool(itemId, poolId, pool => pool.capacity)
                onAddContainerRequested: definition => inventoryRequest.text = qsTr("Richiesta nuovo contenitore: %1").arg(definition.name)
                onAddCatalogItemRequested: inventoryRequest.text = qsTr("Richiesta aggiunta oggetto dal catalogo")
                onAddCustomItemRequested: definition => inventoryRequest.text = qsTr("Richiesta creazione oggetto personalizzato: %1").arg(definition.name)
                onMoveItemRequested: (itemId, containerId) => inventoryRequest.text = qsTr("Richiesta spostamento: %1 → %2").arg(itemId).arg(containerId)
                onItemQuantityChangeRequested: (itemId, quantity) => inventoryRequest.text = qsTr("Richiesta quantità: %1 → %2").arg(itemId).arg(quantity)
                onMergeCoinStacksRequested: (sourceItemId, targetItemId) => {
                    const updated = JSON.parse(JSON.stringify(inventoryPanel.inventory));
                    const source = updated.items.find(entry => entry.id === sourceItemId);
                    const target = updated.items.find(entry => entry.id === targetItemId);
                    target.quantity += source.quantity;
                    for (const field of ["intrinsicWeightGrams", "effectiveWeightGrams", "intrinsicVolumeMilliliters", "effectiveVolumeMilliliters"])
                        target[field] += source[field];
                    updated.items = updated.items.filter(entry => entry.id !== sourceItemId);
                    const container = updated.containers.find(entry => entry.id === source.containerId);
                    container.itemIds = container.itemIds.filter(id => id !== sourceItemId);
                    inventoryPanel.inventory = updated;
                }
                onSplitCoinStackRequested: (itemId, quantity) => {
                    const updated = JSON.parse(JSON.stringify(inventoryPanel.inventory));
                    const source = updated.items.find(entry => entry.id === itemId);
                    const newStack = Object.assign({}, source, {quantity: quantity});
                    let suffix = 1;
                    while (updated.items.some(entry => entry.id === itemId + ".split." + suffix)) ++suffix;
                    newStack.id = itemId + ".split." + suffix;
                    for (const field of ["intrinsicWeightGrams", "effectiveWeightGrams", "intrinsicVolumeMilliliters", "effectiveVolumeMilliliters"]) {
                        if (source[field] == null) continue;
                        newStack[field] = source[field] / source.quantity * quantity;
                        source[field] -= newStack[field];
                    }
                    source.quantity -= quantity;
                    updated.items.push(newStack);
                    const container = updated.containers.find(entry => entry.id === source.containerId);
                    container.itemIds.splice(container.itemIds.indexOf(itemId) + 1, 0, newStack.id);
                    inventoryPanel.inventory = updated;
                }
                onReorderItemRequested: (itemId, beforeItemId) => {
                    const updated = JSON.parse(JSON.stringify(inventoryPanel.inventory));
                    const container = updated.containers.find(entry => entry.itemIds.includes(itemId));
                    const order = container.itemIds.filter(id => id !== itemId);
                    order.splice(beforeItemId === null ? order.length : order.indexOf(beforeItemId), 0, itemId);
                    container.itemIds = order;
                    inventoryPanel.inventory = updated;
                }
                onMoveAndReorderItemRequested: (itemId, containerId, beforeItemId) => {
                    const updated = JSON.parse(JSON.stringify(inventoryPanel.inventory));
                    const oldContainer = updated.containers.find(entry => entry.itemIds.includes(itemId));
                    const destination = updated.containers.find(entry => entry.id === containerId);
                    const moved = updated.items.find(entry => entry.id === itemId);
                    const measures = [
                        ["effectiveWeightGrams", "currentContentsWeightGrams", "ignoresContentsWeight"],
                        ["effectiveVolumeMilliliters", "currentContentsVolumeMilliliters", "ignoresContentsVolume"]
                    ];
                    for (const [effective, current, ignores] of measures) {
                        const amount = moved[effective] ?? moved[current === "currentContentsWeightGrams" ? "intrinsicWeightGrams" : "intrinsicVolumeMilliliters"] ?? 0;
                        for (const [start, delta] of [[oldContainer, -amount], [destination, amount]]) {
                            let container = start;
                            while (container && delta !== 0) {
                                container[current] = (container[current] ?? 0) + delta;
                                if (!container.ownerItemId) {
                                    if (effective === "effectiveWeightGrams" && container.contributesToCarriedWeight)
                                        updated.totalCarriedWeightGrams += delta;
                                    break;
                                }
                                if (container[ignores]) break;
                                const owner = updated.items.find(entry => entry.id === container.ownerItemId);
                                owner[effective] = (owner[effective] ?? 0) + delta;
                                container = updated.containers.find(entry => entry.id === owner.containerId);
                            }
                        }
                    }
                    oldContainer.itemIds = oldContainer.itemIds.filter(id => id !== itemId);
                    destination.itemIds.splice(beforeItemId === null ? destination.itemIds.length : destination.itemIds.indexOf(beforeItemId), 0, itemId);
                    moved.containerId = containerId;
                    inventoryPanel.inventory = updated;
                }
                onUnequipItemRequested: (itemId, containerId) => inventoryRequest.text = qsTr("Richiesta disequipaggiamento: %1 → %2").arg(itemId).arg(containerId)
                onEquipItemRequested: (itemId, replacedItemId, sourceContainerId) => inventoryRequest.text = replacedItemId
                    ? qsTr("Richiesta scambio: %1 sostituisce %2 → %3 (altrimenti Indossati)").arg(itemId).arg(replacedItemId).arg(sourceContainerId)
                    : qsTr("Richiesta equipaggiamento: %1").arg(itemId)
                inventory: ({money: {containers: [{containerId: "home", amounts: {gold: 13}}], total: {copper: 0, silver: 0, gold: 13, platinum: 0, weightGrams: 130}}, totalCarriedWeightGrams: 13710, equipmentSlots: Array.from({length: 14}, (_, slot) => ({slot: slot, capacity: slot === InventoryPanel.Ring ? 2 : 1,
                    itemIds: slot === InventoryPanel.Armor ? ["armor"] : slot === InventoryPanel.Shoulders ? ["cloak"] : []})),
                    items: [
                        {id: "armor", name: "Corazza di esempio", description: "Descrizione dimostrativa dell’armatura equipaggiata.", quantity: 1, intrinsicWeightGrams: 7000, effectiveWeightGrams: 7000, equipped: true, containerId: "worn"},
                        {id: "cloak", name: "Mantello di esempio", quantity: 1, intrinsicWeightGrams: 500, effectiveWeightGrams: 500, equipped: true, containerId: "worn"},
                        {id: "pack", name: "Zaino", quantity: 1, intrinsicWeightGrams: 1000, effectiveWeightGrams: 5200, ownedContainerId: "item.pack", containerId: "worn", equipped: false},
                        {id: "rope", name: "Corda", quantity: 1, canStack: true, intrinsicWeightGrams: 4000, effectiveWeightGrams: 4000, containerId: "item.pack", equipped: false},
                        {id: "pouch", name: "Borsello speciale (esempio)", description: "Esempio dimostrativo di contenitore il cui contenuto non aggiunge peso.", quantity: 1, intrinsicWeightGrams: 200, effectiveWeightGrams: 200, ownedContainerId: "item.pouch", containerId: "item.pack", equipped: false},
                        {id: "potion", name: "Pozione di esempio", quantity: 2, intrinsicWeightGrams: 800, intrinsicVolumeMilliliters: 500, effectiveWeightGrams: 800, containerId: "item.pouch", equipped: false},
                        {id: "sword", name: "Spada di esempio", quantity: 1, intrinsicWeightGrams: 1000, effectiveWeightGrams: 1000, containerId: "worn", equipped: false},
                        {id: "ring", name: "Anello di esempio", quantity: 1, intrinsicWeightGrams: 10, effectiveWeightGrams: 10, equipmentSlot: InventoryPanel.Ring, containerId: "worn", equipped: false},
                        {id: "staff", name: "Bastone di esempio", quantity: 1, intrinsicWeightGrams: 1000, effectiveWeightGrams: 1000,
                            containerId: "home", equipped: false, usagePools: [{id: "bastone", capacity: 10, remaining: 7}]},
                        {id: "gold-a", itemDefinitionId: "goldCoin", canStack: true, name: "Moneta d’oro", quantity: 5, intrinsicWeightGrams: 50, effectiveWeightGrams: 50, intrinsicVolumeMilliliters: 0, effectiveVolumeMilliliters: 0, containerId: "home", equipped: false},
                        {id: "gold-b", itemDefinitionId: "goldCoin", canStack: true, name: "Moneta d’oro", quantity: 8, intrinsicWeightGrams: 80, effectiveWeightGrams: 80, intrinsicVolumeMilliliters: 0, effectiveVolumeMilliliters: 0, containerId: "home", equipped: false}
                    ], containers: [
                        {id: "worn", name: "Indossati", contributesToCarriedWeight: true, currentContentsWeightGrams: 13710, currentContentsVolumeMilliliters: 6000, itemIds: ["armor", "cloak", "pack", "sword", "ring"]},
                        {id: "item.pack", name: "Zaino", ownerItemId: "pack", currentContentsWeightGrams: 4200, maximumContentsWeightGrams: 20000, currentContentsVolumeMilliliters: 3500, maximumContentsVolumeMilliliters: 12000, itemIds: ["rope", "pouch"]},
                        {id: "item.pouch", name: "Borsello speciale (esempio)", ownerItemId: "pouch", ignoresContentsWeight: true, currentContentsWeightGrams: 800, maximumContentsWeightGrams: 2000, currentContentsVolumeMilliliters: 500, maximumContentsVolumeMilliliters: 1000, itemIds: ["potion"]},
                        {id: "home", name: "Casa", contributesToCarriedWeight: false, currentContentsWeightGrams: 1130, currentContentsVolumeMilliliters: 0, itemIds: ["gold-a", "gold-b", "staff"]}
                    ]})
            }
                Label { id: inventoryRequest; width: parent.width; wrapMode: Text.Wrap; visible: !!text }
            }
            MoneyPanel {
                id: moneyPreview
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 15
                money: inventoryPanel.inventory?.money
                containers: inventoryPanel.inventory?.containers ?? []
                onAddMoneyRequested: (denomination, quantity, containerId) => moneyRequest.text = qsTr("Richiesta aggiunta monete: %1 → %2").arg(quantity).arg(containerId)
                onConvertMoneyRequested: (fromDenomination, toDenomination, fromQuantity, toQuantity, sourceId, destinationId) =>
                    moneyRequest.text = qsTr("Richiesta conversione: %1 → %2 monete (%3 → %4)").arg(fromQuantity).arg(toQuantity).arg(sourceId).arg(destinationId)
            }
            Label { id: moneyRequest; width: previewScroll.availableWidth; wrapMode: Text.Wrap; visible: panelSelector.currentIndex === 15 && !!text }
            Loader {
                id: standaloneCasting
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 16
                sourceComponent: castingPreviewPage
            }
            Component {
                id: castingPreviewPage
                SpellcastingsPanel {
                    id: spellsPreview
                    objectName: "spellcastingsPreview"
                    width: parent?.width ?? 0
                    property int silentCastingUses: 1
                    property int mixingUses: 1
                    canPrepareSpell: (casting, slot, spellId) => {
                        const level = spellsPreview.spellLevelsByClass[casting.state.spellListClass]?.[spellId];
                        return level !== undefined && level <= slot.spellLevel
                            && (slot.groupId !== "restricted" || spellId === "shield");
                    }
                    preparationSlotCount: (casting, slot, spellId) => casting.id === "wizard" && spellId === "shield" ? 2 : 1
                    conversionSpellOptions: (casting, entry) => casting.id === "cleric" && entry.slot.spellLevel === 1
                        ? ["cureLightWounds"] : []
                    preparationMetamagicOptions: (casting, slot, spellId) => {
                        if (spellId !== "magicMissile" || slot.spellLevel < 2 || !spellsPreview.canPrepareSpell(casting, slot, spellId))
                            return [];
                        return casting.state.metamagic.map(grant => ({applications: [{type: grant.type, grantId: grant.id, options: {}}]}));
                    }
                    castingMetamagicOptions: (casting, entry) => {
                        if (casting.id === "bard" && entry.spellId === "sleep" && entry.slot.spellLevel >= 2)
                            return [{applications: [{type: SpellcastingsPanel.Still, grantId: "preview-bard-still", options: {}}]}];
                        if (casting.id !== "wizard" || spellsPreview.silentCastingUses === 0
                            || !["shield", "mirrorImage", "magicMissile"].includes(entry.spellId)
                            || entry.metamagic?.some(application => application.type === SpellcastingsPanel.Silent))
                            return [];
                        return [{applications: [{type: SpellcastingsPanel.Silent, grantId: "preview-casting-silent", options: {}}]}];
                    }
                    canCastSpontaneousSpell: (casting, slot, spellId) => spellsPreview.canPrepareSpell(casting, slot, spellId)
                    canPrepareFlexibleSpell: (casting, preparation, spellId) => spellsPreview.canPrepareSpell(casting, preparation, spellId)
                    canCastFlexibleSpell: (casting, slot, preparation) => {
                        const base = spellsPreview.spellLevelsByClass[casting.state.spellListClass]?.[preparation.spellId];
                        return base !== undefined && base + (preparation.metamagic?.length ?? 0) <= slot.spellLevel;
                    }
                    flexiblePreparationMetamagicOptions: (casting, slot, spellId) => slot.spellLevel >= 2 && spellId === "shield"
                        ? [{applications: [{type: SpellcastingsPanel.Silent, grantId: "preview-arcanist-silent"}]}] : []
                    flexibleCastingMetamagicOptions: (casting, slot, preparation) => {
                        if (preparation.spellId !== "shield") return [];
                        const mixed = !!preparation.metamagic?.length;
                        if (slot.spellLevel < (mixed ? 3 : 2) || (mixed && spellsPreview.mixingUses <= 0)) return [];
                        const applications = [{type: SpellcastingsPanel.Still, grantId: "preview-arcanist-still"}];
                        return mixed ? [{applications: applications, mixingPermissionId: "preview-mixing",
                            mixingLabel: qsTr("Combinazione metamagica · 1 utilizzo")}] : [{applications: applications}];
                    }
                    metamagicDetails: ({
                        [SpellcastingsPanel.Silent]: {name: qsTr("Incantesimi Silenziosi"), levelAdjustment: 1},
                        [SpellcastingsPanel.Still]: {name: qsTr("Incantesimi Immobili"), levelAdjustment: 1}
                    })
                    function linkedSlotIds(slotId) {
                        const entries = [];
                        for (const level of activeLevels)
                            for (const entry of level.entries) entries.push(entry.slot);
                        const primary = entries.find(entry => sameSlot(entry.slot, slotId))?.preparationSlot ?? slotId;
                        return entries.filter(entry => sameSlot(entry.preparationSlot ?? entry.slot, primary)).map(entry => entry.slot);
                    }
                    function updateSlots(castingId, slotIds, changes, resetPreparations = false) {
                        spellcastings = {castings: castings.map(casting => {
                            if (casting.id !== castingId) return casting;
                            const state = Object.assign({}, casting.state);
                            const groups = casting.mode === SpellcastingsPanel.Flexible ? "slotGroups" : "groups";
                            state[groups] = state[groups].map(group => Object.assign({}, group, {
                                slots: group.slots.map(entry => !slotIds || slotIds.some(slotId => sameSlot(entry.slot, slotId))
                                    ? Object.assign({}, entry, changes) : entry)
                            }));
                            if (resetPreparations) {
                                state.preparationGroups = state.preparationGroups.map(group => Object.assign({}, group, {
                                    preparations: group.preparations.map(entry => Object.assign({}, entry, {
                                        spellId: null, metamagic: [], evaluation: null
                                    }))
                                }));
                            }
                            return Object.assign({}, casting, {state: state});
                        })};
                    }
                    onPrepareSpellRequested: (castingId, slotIds, spellId, metamagic) =>
                        updateSlots(castingId, slotIds, {state: SpellcastingsPanel.Ready, spellId: spellId,
                            metamagic: metamagic, preparationSlot: slotIds[0]})
                    onConsumeSpellRequested: (castingId, slotId, metamagic, replacementSpellId) => {
                        if (metamagic.length) --silentCastingUses;
                        if (slotId.spellLevel !== 0) updateSlots(castingId, linkedSlotIds(slotId), {state: SpellcastingsPanel.Used});
                    }
                    onCastSpontaneousSpellRequested: (castingId, slotId, spellId, metamagic) =>
                        updateSlots(castingId, [slotId], {state: SpellcastingsPanel.CastSlotUsed})
                    function updatePreparation(castingId, preparationId, spellId, metamagic = []) {
                        spellcastings = {castings: castings.map(casting => casting.id !== castingId ? casting :
                            Object.assign({}, casting, {state: Object.assign({}, casting.state, {
                                preparationGroups: casting.state.preparationGroups.map(group => Object.assign({}, group, {
                                    preparations: group.preparations.map(entry => sameSlot(entry.preparationId, preparationId)
                                        ? Object.assign({}, entry, {spellId: spellId, metamagic: metamagic}) : entry)
                                }))
                            })}))};
                    }
                    onPrepareFlexibleSpellRequested: (castingId, preparationId, spellId, metamagic) =>
                        updatePreparation(castingId, preparationId, spellId, metamagic)
                    onClearFlexiblePreparationRequested: (castingId, preparationId) =>
                        updatePreparation(castingId, preparationId, null)
                    onCastFlexibleSpellRequested: (castingId, slotId, preparationId, metamagic, mixingPermissionId) => {
                        if (mixingPermissionId) --mixingUses;
                        updateSlots(castingId, [slotId], {state: SpellcastingsPanel.CastSlotUsed})
                    }
                    onClearSpellSlotRequested: (castingId, slotId) =>
                        updateSlots(castingId, linkedSlotIds(slotId), {state: SpellcastingsPanel.Free, spellId: null, metamagic: [], preparationSlot: null})
                    onResetSpellSlotsRequested: castingId => {
                        if (activeCasting.mode === SpellcastingsPanel.Flexible)
                            updateSlots(castingId, null, {state: SpellcastingsPanel.CastSlotFree}, true);
                        else if (activeCasting.mode === SpellcastingsPanel.Spontaneous)
                            updateSlots(castingId, null, {state: SpellcastingsPanel.CastSlotFree});
                        else
                            updateSlots(castingId, null, {state: SpellcastingsPanel.Free, spellId: null, metamagic: [], preparationSlot: null});
                    }
                    function learnSpell(castingId, spellId, selection) {
                        spellcastings = {castings: castings.map(casting => {
                            if (casting.id !== castingId) return casting;
                            const details = Object.assign({}, casting.repertoireDetails);
                            if (selection) {
                                details.quotas = details.quotas.map(quota => quota.grant.id !== selection.quotaId ? quota :
                                    Object.assign({}, quota, {remainingByLevel: Object.assign({}, quota.remainingByLevel, {
                                        [selection.spellLevel]: quota.remainingByLevel[selection.spellLevel] - 1
                                    })}));
                                details.selections = (details.selections ?? []).concat({selection: selection, active: true});
                            } else {
                                details.manualSpellIds = (details.manualSpellIds ?? []).concat(spellId);
                            }
                            return Object.assign({}, casting, {repertoireDetails: details,
                                repertoire: casting.repertoire.includes(spellId) ? casting.repertoire : casting.repertoire.concat(spellId)});
                        })};
                    }
                    onAddSpellToRepertoireRequested: (castingId, spellId) => learnSpell(castingId, spellId, null)
                    onSelectRepertoireSpellRequested: (castingId, selection) => learnSpell(castingId, selection.spellId, selection)
                    spellCatalog: Object.keys(spellNames).map(id => ({id: id, name: spellNames[id]}))
                    spellNames: ({"light": "Luce", "detectMagic": "Individuazione del Magico", "mageHand": "Mano Magica",
                        "magicMissile": "Dardo Incantato", "shield": "Scudo", "mirrorImage": "Immagine Speculare", "sleep": "Sonno",
                        "bless": "Benedizione", "cureLightWounds": "Cura Ferite Leggere", "grease": "Unto", "message": "Messaggio",
                        "zoneOfTruth": "Zona di Verità"})
                    spellLevelsByClass: ({
                        [ClassesPanel.Wizard]: {light: 0, detectMagic: 0, mageHand: 0, magicMissile: 1, shield: 1, mirrorImage: 2, grease: 1, message: 0},
                        [ClassesPanel.Bard]: {light: 0, sleep: 1, cureLightWounds: 1, grease: 1, message: 0},
                        [ClassesPanel.Cleric]: {bless: 1, cureLightWounds: 1},
                        [ClassesPanel.Paladin]: {bless: 1, cureLightWounds: 1, zoneOfTruth: 2},
                        [ClassesPanel.Arcanist]: {light: 0, shield: 1, magicMissile: 1, grease: 1, message: 0}
                    })
                    spellcastings: ({castings: [
                        {id: "wizard", source: "Mago", repertoire: ["light", "detectMagic", "mageHand", "magicMissile", "shield", "mirrorImage"], mode: SpellcastingsPanel.Prepared,
                            repertoireDetails: {manualAdditionAllowed: true}, state: {spellListClass: ClassesPanel.Wizard, groups: [{slots: [
                            {slot: {groupId: "base", spellLevel: 0, index: 0}, state: 1, spellId: "light"},
                            {slot: {groupId: "base", spellLevel: 0, index: 1}, state: 1, spellId: "detectMagic"},
                            {slot: {groupId: "base", spellLevel: 0, index: 2}, state: 1, spellId: "mageHand"},
                            {slot: {groupId: "base", spellLevel: 1, index: 0}, state: 1, spellId: "shield"},
                            {slot: {groupId: "base", spellLevel: 1, index: 1}, state: 2, spellId: "magicMissile"},
                            {slot: {groupId: "base", spellLevel: 1, index: 2}, state: 0},
                            {slot: {groupId: "base", spellLevel: 1, index: 3}, state: 0},
                            {slot: {groupId: "base", spellLevel: 2, index: 0}, state: 1, spellId: "mirrorImage"},
                            {slot: {groupId: "base", spellLevel: 2, index: 1}, state: 2, spellId: "mirrorImage"},
                            {slot: {groupId: "base", spellLevel: 2, index: 2}, state: 0}
                        ]}, {grant: {source: "Slot di esempio: solo Scudo", allowedSpellIds: ["shield"]}, slots: [
                            {slot: {groupId: "restricted", spellLevel: 1, index: 0}, state: 0}
                        ]}], metamagic: [
                            {id: "preview-silent", type: SpellcastingsPanel.Silent},
                            {id: "preview-still", type: SpellcastingsPanel.Still}
                        ]}},
                        {id: "cleric", source: "Chierico", repertoire: ["bless", "cureLightWounds"], mode: SpellcastingsPanel.Prepared,
                            state: {spellListClass: ClassesPanel.Cleric, groups: [{slots: [
                                {slot: {groupId: "base", spellLevel: 1, index: 0}, state: 1, spellId: "bless"}
                            ]}]}},
                        {id: "paladin", source: "Paladino", repertoire: [], mode: SpellcastingsPanel.Prepared,
                            repertoireDetails: {manualAdditionAllowed: true, listGrants: [{id: "paladin-list", source: "Paladino"}]},
                            state: {spellListClass: ClassesPanel.Paladin, groups: [{slots: [
                                {slot: {groupId: "base", spellLevel: 1, index: 0}, state: SpellcastingsPanel.Free}
                            ]}]}},
                        {id: "bard", source: "Bardo", repertoire: ["light", "sleep"], mode: SpellcastingsPanel.Spontaneous,
                            repertoireDetails: {manualAdditionAllowed: false, quotas: [
                                {grant: {id: "bard-known", source: "Incantesimi conosciuti"}, remainingByLevel: {1: 1}}
                            ], selections: [
                                {selection: {quotaId: "bard-known", spellLevel: 1, spellId: "sleep"}, active: true}
                            ]},
                            state: {spellListClass: ClassesPanel.Bard, groups: [{slots: [
                            {slot: {groupId: "base", spellLevel: 1, index: 0}, state: SpellcastingsPanel.CastSlotFree},
                            {slot: {groupId: "base", spellLevel: 1, index: 1}, state: SpellcastingsPanel.CastSlotUsed},
                            {slot: {groupId: "base", spellLevel: 2, index: 0}, state: SpellcastingsPanel.CastSlotFree}
                        ]}], metamagic: [{id: "preview-bard-still", type: SpellcastingsPanel.Still}]}},
                        {id: "arcanist", source: "Arcanista", repertoire: ["light", "shield", "magicMissile"], mode: SpellcastingsPanel.Flexible,
                            repertoireDetails: {manualAdditionAllowed: true},
                            state: {spellListClass: ClassesPanel.Arcanist, preparationGroups: [{preparations: [
                                {preparationId: {groupId: "base", spellLevel: 0, index: 0}},
                                {preparationId: {groupId: "base", spellLevel: 1, index: 0}, spellId: "shield"},
                                {preparationId: {groupId: "base", spellLevel: 1, index: 1}},
                                {preparationId: {groupId: "base", spellLevel: 2, index: 0}}
                            ]}], slotGroups: [{slots: [
                                {slot: {groupId: "base", spellLevel: 0, index: 0}, state: SpellcastingsPanel.CastSlotFree},
                                {slot: {groupId: "base", spellLevel: 1, index: 0}, state: SpellcastingsPanel.CastSlotFree},
                                {slot: {groupId: "base", spellLevel: 1, index: 1}, state: SpellcastingsPanel.CastSlotFree},
                                {slot: {groupId: "base", spellLevel: 2, index: 0}, state: SpellcastingsPanel.CastSlotFree},
                                {slot: {groupId: "base", spellLevel: 3, index: 0}, state: SpellcastingsPanel.CastSlotFree}
                            ]}], metamagic: [
                                {id: "preview-arcanist-silent", type: SpellcastingsPanel.Silent},
                                {id: "preview-arcanist-still", type: SpellcastingsPanel.Still}
                            ]}},
                    ]})
                }
            }
            Loader {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 17
                sourceComponent: grantedPreviewPage
            }
            Component {
                id: grantedPreviewPage
                Column {
                    spacing: 8
                    GrantedSpellsPanel {
                        id: grantedSpellsPreview
                        objectName: "grantedSpellsPreview"
                        width: parent.width
                        property int ancestryUses: 1
                        readonly property var staffPool: inventoryPanel.item("staff")?.usagePools?.[0]
                        spellNames: ({light: qsTr("Luce"), detectMagic: qsTr("Individuazione del Magico"),
                            mageArmor: qsTr("Armatura Magica"), magicMissile: qsTr("Dardo Incantato")})
                        grantedSpells: [
                            {id: "ancestry", spellId: "light", source: qsTr("Tratto razziale di esempio"), usage: GrantedSpellsPanel.Limited,
                                capacity: 1, remaining: ancestryUses, cost: 1},
                            {id: "ability", spellId: "detectMagic", source: qsTr("Capacità di esempio"), usage: GrantedSpellsPanel.AtWill},
                            {id: "amulet", spellId: "mageArmor", source: qsTr("Amuleto di esempio"), usage: GrantedSpellsPanel.Constant},
                            {id: "staff", spellId: "magicMissile", source: qsTr("Bastone di esempio"), usage: GrantedSpellsPanel.SharedPool,
                                capacity: staffPool?.capacity, remaining: staffPool?.remaining, cost: 2}
                        ]
                        onCastGrantedSpellRequested: grantId => {
                            const grant = grantedSpells.find(entry => entry.id === grantId);
                            if (!grant || !canCast(grant)) return;
                            if (grant.usage === GrantedSpellsPanel.Limited) ancestryUses -= grant.cost;
                            if (grant.usage === GrantedSpellsPanel.SharedPool)
                                inventoryPanel.updateUsagePool("staff", "bastone", pool => pool.remaining - grant.cost);
                            grantedSpellStatus.text = qsTr("Lanciato: %1").arg(spellNames[grant.spellId]);
                        }
                    }
                    Label {
                        id: grantedSpellStatus
                        width: parent.width
                        visible: grantedSpellsPreview.visible && !!text
                        wrapMode: Text.Wrap
                    }
                }
            }
            EnhancementsPanel {
                objectName: "enhancementsPreview"
                expressionTargets: window.expressionTargetCatalog
                property int nextGroupId: 1
                property int nextEffectId: 1
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 18
                resourceNames: ({attack: qsTr("Tiri per colpire"), fearSave: qsTr("Tiri salvezza contro paura"),
                    str: qsTr("Forza"), dex: qsTr("Destrezza"), "hp.max": qsTr("Punti ferita massimi")})
                effectResources: ["str", "dex", "hp.max"].map(id => ({id: id, name: resourceNames[id],
                    kinds: id === "hp.max" ? [EnhancementsPanel.Contribution] : [EnhancementsPanel.Bonus, EnhancementsPanel.Penalty]}))
                bonusTypes: [{value: AbilityScoresPanel.Generic, label: qsTr("Senza tipo")},
                    {value: AbilityScoresPanel.Morale, label: qsTr("Morale")},
                    {value: AbilityScoresPanel.Enhancement, label: qsTr("Potenziamento")}]
                enhancements: ({groups: [
                    {id: "bless", name: qsTr("Benedizione"), active: true, effectsEditable: false, origin: {type: EnhancementsPanel.Spell}, enhancements: [
                        {definition: {resource: "attack", type: AbilityScoresPanel.Bonus, expression: "1"}, resolvedValue: 1},
                        {definition: {resource: "fearSave", type: AbilityScoresPanel.Bonus, expression: "1"}, resolvedValue: 1}
                    ]},
                    {id: "fatigue", name: qsTr("Affaticamento"), active: false, effectsEditable: true, enhancements: [
                        {definition: {id: "strength-penalty", resource: "str", type: AbilityScoresPanel.Penalty, expression: "2"}},
                        {definition: {id: "dexterity-penalty", resource: "dex", type: AbilityScoresPanel.Penalty, expression: "2"}}
                    ]}
                ]})
                onAddEnhancementGroupRequested: definition => {
                    const group = Object.assign({id: "preview-enhancement-" + nextGroupId++, effectsEditable: true}, definition);
                    enhancements = {groups: enhancements.groups.concat(group)};
                }
                onAddEnhancementRequested: (groupId, definition) => {
                    const group = enhancements.groups.find(entry => entry.id === groupId);
                    if (!group?.effectsEditable) return;
                    const effect = {definition: Object.assign({id: "preview-effect-" + nextEffectId++}, definition)};
                    enhancements = {groups: enhancements.groups.map(entry =>
                        entry.id === groupId ? Object.assign({}, entry, {enhancements: entry.enhancements.concat(effect)}) : entry)};
                }
                onRemoveEnhancementRequested: (groupId, effectId) => {
                    enhancements = {groups: enhancements.groups.map(group => group.id === groupId && group.effectsEditable
                        ? Object.assign({}, group, {enhancements: group.enhancements.filter(effect => effect.definition.id !== effectId)}) : group)};
                }
                onRemoveEnhancementGroupRequested: groupId => {
                    enhancements = {groups: enhancements.groups.filter(group => group.id !== groupId || !isCustom(group))};
                }
                onSetEnhancementGroupActiveRequested: (groupId, active) => {
                    enhancements = {groups: enhancements.groups.map(group =>
                        group.id === groupId ? Object.assign({}, group, {active: active, effectsEditable: !active && isCustom(group)}) : group)};
                }
            }
            NotesPanel {
                objectName: "notesPreview"
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 19
                notes: qsTr("Chiedere informazioni sulla torre al prossimo viaggio.")
                onNotesChangeRequested: updatedNotes => { notes = updatedNotes; }
            }
            ConditionsPanel {
                id: conditionsPreview
                objectName: "conditionsPreview"
                property int nextConditionEntryId: 1
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 20
                automaticConditionIds: ["dead", "disabled"]
                function demoSeverity(condition, entries) {
                    if (condition.id === "negativeLevels" || condition.id === "fear") {
                        const total = entries.reduce((sum, entry) => sum + entry.severity, 0);
                        return condition.id === "fear" ? Math.min(condition.stages.length, total) : total;
                    }
                    return entries.reduce((highest, entry) => Math.max(highest, entry.severity), 0);
                }
                conditions: { return {conditions: window.conditionCatalog.map(condition => {
                    const automatic = condition.id === "staggered";
                    return Object.assign({}, condition, {effectiveSeverity: automatic ? 1 : 0,
                        entries: automatic ? [{id: "preview-staggered", origin: ConditionsPanel.SourceControlled, severity: 1}] : []});
                })}; }
                onAddConditionRequested: definition => {
                    conditions = {conditions: conditions.conditions.map(condition => {
                        if (condition.id !== definition.conditionId) return condition;
                        const entries = condition.entries.concat({
                            id: "preview-manual-" + nextConditionEntryId++, origin: ConditionsPanel.Manual, severity: definition.severity,
                            parameter: definition.parameter
                        });
                        return Object.assign({}, condition, {effectiveSeverity: demoSeverity(condition, entries), entries});
                    })};
                }
                onRemoveConditionsRequested: ids => {
                    conditions = {conditions: conditions.conditions.map(condition => {
                        const entries = condition.entries.filter(entry => !ids.includes(entry.id));
                        if (entries.length === condition.entries.length) return condition;
                        return Object.assign({}, condition, {entries: entries,
                            effectiveSeverity: demoSeverity(condition, entries)});
                    })};
                }
            }
            RemindersPanel {
                objectName: "remindersPreview"
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 21
                reminders: {
                    const bleeding = conditionsPreview.conditions.conditions.find(condition => condition.id === "bleeding");
                    return {messages: (bleeding?.entries ?? []).map(entry =>
                        qsTr("Sanguinante: %1 all’inizio del turno.").arg(entry.parameter))};
                }
            }
            PendingChoicesPanel {
                objectName: "pendingChoicesPreview"
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 22
                pendingChoices: {
                    const bard = (standaloneCasting.item?.castings ?? []).find(casting => casting.id === "bard");
                    const spellChoices = bard?.repertoireDetails?.quotas?.[0]?.remainingByLevel?.[1] ?? 0;
                    return [
                        {type: PendingChoicesPanel.FeatSlot, choiceReference: {slotId: "level3"}, description: qsTr("Scegli un talento di livello 3"), remaining: 1, blocksAdvancement: true},
                        {type: PendingChoicesPanel.Repertoire, choiceReference: {castingId: "bard", quotaId: "bard-known", spellLevel: 1},
                            description: qsTr("Scegli un incantesimo conosciuto da Bardo di livello 1"), remaining: spellChoices, blocksAdvancement: true},
                        {type: PendingChoicesPanel.SkillRanks, choiceReference: {}, description: qsTr("Assegna i gradi abilità"),
                            remaining: skillsPreview.maxRanks - skillsPreview.spentRanks, blocksAdvancement: false}
                    ].filter(choice => choice.remaining > 0);
                }
                onOpenChoiceRequested: (type, reference) => {
                    switch (type) {
                    case PendingChoicesPanel.Alignment: panelSelector.currentIndex = 0; break;
                    case PendingChoicesPanel.SkillRanks: panelSelector.currentIndex = 2; break;
                    case PendingChoicesPanel.Language: panelSelector.currentIndex = 10; break;
                    case PendingChoicesPanel.FeatSlot: panelSelector.currentIndex = 11; break;
                    case PendingChoicesPanel.Race:
                    case PendingChoicesPanel.RacialOption: panelSelector.currentIndex = 12; break;
                    case PendingChoicesPanel.ClassOption:
                    case PendingChoicesPanel.ClassHitDie:
                    case PendingChoicesPanel.FavoredClass:
                    case PendingChoicesPanel.FavoredClassBonus: panelSelector.currentIndex = 13; break;
                    case PendingChoicesPanel.PreparedSpell:
                    case PendingChoicesPanel.FlexiblePreparation:
                    case PendingChoicesPanel.Repertoire:
                        if (standaloneCasting.item) standaloneCasting.item.selectedId = reference.castingId;
                        panelSelector.currentIndex = 16;
                        break;
                    }
                }
            }
            ActionsPanel {
                objectName: "actionsPreview"
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 23
                attackRoutines: window.actionRoutinesSnapshot
                actions: window.actionsSnapshot
            }
            CombatManeuversPanel {
                id: previewManeuvers
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 9
                combatManeuvers: ({baseAttackBonus: 11, maneuvers: Array.from({length: 10}, (_, type) => ({
                    type: type,
                    bonus: {abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4,
                        totalValue: type === CombatManeuversPanel.Disarm ? 17 : 15}],
                        modifiers: {modifiers: type === CombatManeuversPanel.Disarm ? [
                            {active: true, type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, resolvedValue: 2, source: "Talento di esempio"}
                        ] : [], conditionalTotals: type === CombatManeuversPanel.Disarm
                            ? [{condition: qsTr("Contro creature più grandi"), value: 2}] : []}},
                    defense: {strengthModifier: 4, dexterityModifier: 2, appliedDexterityModifier: 2, totalValue: 27,
                        dexterityBonusSuppressed: false, dexteritySuppressions: [], modifiers: {modifiers: [],
                            conditionalTotals: type === CombatManeuversPanel.Disarm
                                ? [{condition: qsTr("Contro creature più grandi"), value: 1}]
                                : type === CombatManeuversPanel.Trip ? [{condition: qsTr("Su terreno scivoloso"), value: -2}] : []}}
                }))})
            }
            MovementPanel {
                id: previewMovement
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 7
                movement: ({grants: [
                    {id: "land", source: "Razza di esempio", type: MovementPanel.Land, baseUnits: 6, effectiveUnits: 6, usable: true,
                        modifiers: {total: 0, modifiers: []}, run: {supported: true, usable: true, effectiveMultiplier: 4, distanceUnits: 24}},
                    {id: "fly", source: "Incantesimo di esempio", type: MovementPanel.Fly, baseUnits: 12, effectiveUnits: 12, usable: true,
                        maneuverability: MovementPanel.Good, modifiers: {total: 0, modifiers: []}, run: {supported: false}}
                ]})
            }
            InitiativePanel {
                id: previewInitiative
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 6
                initiative: ({abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Dexterity,
                    abilityModifier: 3, totalValue: 7, modifiers: {total: 4, modifiers: [
                        {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, source: "Iniziativa Migliorata", active: true, resolvedValue: 4}
                    ]}}]})
            }
            ArmorClassPanel {
                id: previewArmor
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 5
                armorClass: ({maximumDexterityBonus: 3,
                    maximumDexterityLimits: [{source: "Armatura di esempio", effectiveValue: 3}],
                    abilitySuppressions: [], abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Dexterity,
                        abilityModifier: 4, values: [
                            {type: ArmorClassPanel.Normal, appliedAbilityModifier: 3, totalValue: 18, modifiers: {total: 5,
                                conditionalTotals: [{condition: qsTr("Contro i giganti"), value: 4}, {condition: qsTr("In carica"), value: -2}],
                                modifiers: [{type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Armor, source: "Armatura di esempio", active: true, resolvedValue: 5}]}},
                            {type: ArmorClassPanel.Touch, appliedAbilityModifier: 3, totalValue: 13, modifiers: {total: 0, modifiers: []}},
                            {type: ArmorClassPanel.FlatFooted, appliedAbilityModifier: 0, totalValue: 15, modifiers: {total: 5, modifiers: [{type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Armor, source: "Armatura di esempio", active: true, resolvedValue: 5}]}}
                        ]}]})
            }
            SavingThrowsPanel {
                id: previewSaves
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 3
                // Synthetic SavingThrowView snapshots; no backend calculation here.
                savingThrows: [SavingThrowsPanel.Fortitude, SavingThrowsPanel.Reflex, SavingThrowsPanel.Will].map(type => ({
                    type: type, baseValue: 2,
                    baseContributions: {total: 2, contributions: [{id: "Classe di esempio", resolvedValue: 2}]},
                    abilityOptions: [{source: "Base", abilityType: [AbilityScoresPanel.Constitution, AbilityScoresPanel.Dexterity, AbilityScoresPanel.Wisdom][type], abilityModifier: 1, totalValue: 4}],
                    modifiers: {total: 1, modifiers: [{type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Resistance, source: "Mantello di esempio", active: true, resolvedValue: 1}],
                        conditionalTotals: type === SavingThrowsPanel.Fortitude
                            ? [{condition: qsTr("Contro il veleno"), value: 2}, {condition: qsTr("Contro le malattie"), value: 1}]
                            : type === SavingThrowsPanel.Will ? [{condition: qsTr("Contro la paura"), value: 2}] : []}
                }))
            }
        }
    }
}
