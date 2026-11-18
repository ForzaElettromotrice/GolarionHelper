pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Golarion.Sheet

ApplicationWindow {
    id: window
    width: 780
    height: 860
    minimumWidth: 360
    minimumHeight: 480
    visible: true
    title: qsTr("Golarion — Anteprima pannelli")
    property var featCatalog: []
    property var raceCatalog: []
    property var classCatalog: []
    property var archetypeCatalog: ({})
    property var raceAlternateCatalog: ({})
    readonly property alias featPickerPopup: featPicker

    // Already-resolved View fixtures: BAB +11 and a synthetic +4 attack bonus.
    function sampleSwordAttack(bonus) {
        return {strikeName: "Spada lunga", slotName: "Arma", routineAttackBonusAdjustment: 0, progressionAttackBonusAdjustment: bonus - 15,
            strike: {source: "Spada lunga di esempio", usageAttackPenalty: 0, attackAbilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4, attackBonus: bonus, criticalConfirmationBonus: bonus}],
                defenseOptions: [{defenseType: ArmorClassPanel.Normal}],
                reach: {minimumUnits: {effectiveValue: 0}, maximumUnits: {effectiveValue: 1}},
                attackModifiers: {modifiers: []}, criticalConfirmationModifiers: {modifiers: []}, damageModifiers: {modifiers: []},
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
            if (entry.resourceName !== "skill.acrobatics") return entry;
            return Object.assign({}, entry, {classSkillBonus: 3, appliedArmorCheckPenalty: 1,
                abilityOptions: [{abilityType: AbilityScoresPanel.Dexterity, abilityModifier: 2,
                    source: "Base", totalValue: 12, modifiers: {total: 2, modifiers: [
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
                    model: [qsTr("Identità"), qsTr("Caratteristiche"), qsTr("Abilità"), qsTr("Tiri salvezza"), qsTr("Punti ferita"), qsTr("Classe Armatura"), qsTr("Iniziativa"), qsTr("Movimento"), qsTr("Attacchi"), qsTr("Manovre"), qsTr("Lingue"), qsTr("Talenti"), qsTr("Razza"), qsTr("Classi"), qsTr("Inventario")]
                    currentIndex: 14
                    objectName: "panelSelector"
                    Accessible.name: qsTr("Pannello da visualizzare")
                }
                Item { Layout.fillWidth: true }
                CheckBox { id: readOnlyToggle; text: qsTr("Sola lettura"); visible: panelSelector.currentIndex === 0 }
            }
        }
    }

    ScrollView {
        id: previewScroll
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
                        energyResistances: [{energy: DefensesPanel.Fire, effectiveValue: 10}, {energy: DefensesPanel.Cold, effectiveValue: 5}],
                        energyVulnerabilities: [{energy: DefensesPanel.Electricity, damageIncreasePercent: 50, grants: []}]
                    })
                HitPointsPanel {
                    width: parent.width
                    belowHeader: Component {
                        Column {
                            spacing: 8
                            DefensesPanel {
                                width: parent.width
                                defenses: healthPreview.sampleDefenses
                                presentation: DefensesPanel.Tags
                                maximumItemsPerRow: 3
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
            Column {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 10
                spacing: 8
                LanguagesPanel {
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
            Column {
                id: featsSection
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 11
                spacing: 8
                property string acquisitionLabel: ""
                property string targetSlotId: ""
                property bool extraAcquisition: false
                FeatsPanel {
                    id: featsPanel
                    width: parent.width
                    feats: ({slots: [
                        {id: "level1", source: "Livello 1", available: true, origin: FeatsPanel.Source, featId: "initiative"},
                        {id: "level3", source: "Livello 3", available: true, origin: FeatsPanel.Source, featId: null},
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
            Column {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 12
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
                width: parent.width
                onAddContainerRequested: definition => inventoryRequest.text = qsTr("Richiesta nuovo contenitore: %1").arg(definition.name)
                inventory: ({totalCarriedWeightGrams: 14500, equipmentSlots: Array.from({length: 14}, (_, slot) => ({slot: slot, capacity: slot === InventoryPanel.Ring ? 2 : 1,
                    itemIds: slot === InventoryPanel.Armor ? ["armor"] : slot === InventoryPanel.Shoulders ? ["cloak"] : []})),
                    items: [
                        {id: "armor", name: "Corazza di esempio", description: "Descrizione dimostrativa dell’armatura equipaggiata.", quantity: 1, intrinsicWeightGrams: 7000, effectiveWeightGrams: 7000, equipped: true, containerId: "worn"},
                        {id: "cloak", name: "Mantello di esempio", quantity: 1, intrinsicWeightGrams: 500, effectiveWeightGrams: 500, equipped: true, containerId: "worn"},
                        {id: "pack", name: "Zaino", quantity: 1, intrinsicWeightGrams: 1000, effectiveWeightGrams: 6000, ownedContainerId: "item.pack", containerId: "worn", equipped: false},
                        {id: "rope", name: "Corda", quantity: 1, intrinsicWeightGrams: 4000, effectiveWeightGrams: 4000, containerId: "item.pack", equipped: false},
                        {id: "pouch", name: "Borsello", quantity: 1, intrinsicWeightGrams: 200, effectiveWeightGrams: 1000, ownedContainerId: "item.pouch", containerId: "item.pack", equipped: false},
                        {id: "potion", name: "Pozione di esempio", quantity: 2, intrinsicWeightGrams: 800, effectiveWeightGrams: 800, containerId: "item.pouch", equipped: false},
                        {id: "sword", name: "Spada di esempio", quantity: 1, intrinsicWeightGrams: 1000, effectiveWeightGrams: 1000, containerId: "worn", equipped: false}
                    ], containers: [
                        {id: "worn", name: "Indossati", currentContentsWeightGrams: 14500, itemIds: ["armor", "cloak", "pack", "sword"]},
                        {id: "item.pack", name: "Zaino", ownerItemId: "pack", currentContentsWeightGrams: 5000, maximumContentsWeightGrams: 20000, itemIds: ["rope", "pouch"]},
                        {id: "item.pouch", name: "Borsello", ownerItemId: "pouch", currentContentsWeightGrams: 800, maximumContentsWeightGrams: 2000, itemIds: ["potion"]},
                        {id: "home", name: "Casa", currentContentsWeightGrams: 0, itemIds: []}
                    ]})
            }
                Label { id: inventoryRequest; width: parent.width; wrapMode: Text.Wrap; visible: !!text }
            }
            CombatManeuversPanel {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 9
                combatManeuvers: ({baseAttackBonus: 11, maneuvers: Array.from({length: 10}, (_, type) => ({
                    type: type,
                    bonus: {abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Strength, abilityModifier: 4,
                        totalValue: type === CombatManeuversPanel.Disarm ? 17 : 15}],
                        modifiers: {modifiers: type === CombatManeuversPanel.Disarm ? [
                            {active: true, type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, resolvedValue: 2, source: "Talento di esempio"}
                        ] : []}},
                    defense: {strengthModifier: 4, dexterityModifier: 2, appliedDexterityModifier: 2, totalValue: 27,
                        dexterityBonusSuppressed: false, dexteritySuppressions: [], modifiers: {modifiers: []}}
                }))})
            }
            MovementPanel {
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
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 6
                initiative: ({abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Dexterity,
                    abilityModifier: 3, totalValue: 7, modifiers: {total: 4, modifiers: [
                        {type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Generic, source: "Iniziativa Migliorata", active: true, resolvedValue: 4}
                    ]}}]})
            }
            ArmorClassPanel {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 5
                armorClass: ({maximumDexterityBonus: 3,
                    maximumDexterityLimits: [{source: "Armatura di esempio", effectiveValue: 3}],
                    abilitySuppressions: [], abilityOptions: [{source: "Base", abilityType: AbilityScoresPanel.Dexterity,
                        abilityModifier: 4, values: [
                            {type: ArmorClassPanel.Normal, appliedAbilityModifier: 3, totalValue: 18, modifiers: {total: 5, modifiers: [{type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Armor, source: "Armatura di esempio", active: true, resolvedValue: 5}]}},
                            {type: ArmorClassPanel.Touch, appliedAbilityModifier: 3, totalValue: 13, modifiers: {total: 0, modifiers: []}},
                            {type: ArmorClassPanel.FlatFooted, appliedAbilityModifier: 0, totalValue: 15, modifiers: {total: 5, modifiers: [{type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Armor, source: "Armatura di esempio", active: true, resolvedValue: 5}]}}
                        ]}]})
            }
            SavingThrowsPanel {
                width: previewScroll.availableWidth
                visible: panelSelector.currentIndex === 3
                // Synthetic SavingThrowView snapshots; no backend calculation here.
                savingThrows: [SavingThrowsPanel.Fortitude, SavingThrowsPanel.Reflex, SavingThrowsPanel.Will].map(type => ({
                    type: type, baseValue: 2,
                    baseContributions: {total: 2, contributions: [{id: "Classe di esempio", resolvedValue: 2}]},
                    abilityOptions: [{source: "Base", abilityType: [AbilityScoresPanel.Constitution, AbilityScoresPanel.Dexterity, AbilityScoresPanel.Wisdom][type], abilityModifier: 1, totalValue: 4}],
                    modifiers: {total: 1, modifiers: [{type: AbilityScoresPanel.Bonus, bonusType: AbilityScoresPanel.Resistance, source: "Mantello di esempio", active: true, resolvedValue: 1}]}
                }))
            }
        }
    }
}
