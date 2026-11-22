import QtQuick
import QtTest
import Golarion.Sheet

TestCase {
    id: testCase
    name: "ActionsPanel"
    when: windowShown
    width: 600
    height: 400
    visible: true
    Component { id: panelComponent; ActionsPanel {} }

    function test_filtersRoutinesAndShowsOnlyBlockedReasons() {
        const panel = createTemporaryObject(panelComponent, testCase, {width: 580,
            attackRoutines: {routines: [{actionId: "strike"}]},
            actions: {categories: [
                {name: "Attacchi", actions: [{id: "strike", name: "Attacco singolo", usable: true}]},
                {id: "movement", name: "Movimento", actions: [
                    {id: "draw", name: "Estrarre", baseCost: ActionCostIcon.Move, effectiveCost: ActionCostIcon.Free, usable: true,
                        description: "Estrae un’arma.", source: "Regole base", notes: [{text: "Nota di esempio."}]},
                    {id: "run", name: "Correre", effectiveCost: ActionCostIcon.FullRound, usable: false,
                        inhibitions: [{reason: "Carico eccessivo."}]}
                ]}
            ]}});
        waitForRendering(panel);
        compare(panel.visibleCategories.length, 1);
        verify(findChild(panel, "actionCard_strike") === null);
        const draw = findChild(panel, "actionCard_draw");
        const run = findChild(panel, "actionCard_run");
        compare(findChild(draw, "actionCost").cost, ActionCostIcon.Free);
        verify(!findChild(draw, "actionInhibition").visible);
        verify(findChild(run, "actionInhibition").visible);
        compare(findChild(run, "actionInhibition").text, "Azione impedita: Carico eccessivo.");
        const info = findChild(draw, "actionInfo");
        const actionName = findChild(draw, "actionName");
        compare(actionName.width, actionName.implicitWidth);
        compare(info.x, actionName.x + actionName.width + 4);
        const details = findChild(draw, "actionDetails");
        mouseMove(panel, 2, 2);
        tryCompare(info, "opacity", 0);
        mouseMove(draw, 16, 10);
        tryCompare(info, "opacity", 1);
        info.click();
        verify(details.visible);
        compare(details.summaryText, "Estrae un’arma.\nFonte: Regole base\nNota di esempio.");
        info.click();
        verify(!details.visible);
        const header = findChild(panel, "actionCategory_movement");
        header.click();
        verify(!draw.visible && !run.visible);
        header.click();
        verify(draw.visible && run.visible);
    }
}
