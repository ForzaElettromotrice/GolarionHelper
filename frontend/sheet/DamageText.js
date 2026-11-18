.pragma library

function types(component, choiceMode) {
    const names = [qsTr("Contundente"), qsTr("Perforante"), qsTr("Tagliente"), qsTr("Acido"), qsTr("Freddo"), qsTr("Elettricità"), qsTr("Fuoco"), qsTr("Sonoro"), qsTr("Forza"), qsTr("Energia positiva"), qsTr("Energia negativa"), qsTr("Senza tipo")];
    return (component.types ?? []).map(type => names[type] ?? "—").join(component.typeMode === choiceMode ? " o " : " e ");
}
