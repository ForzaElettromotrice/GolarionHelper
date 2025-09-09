package org.golarion.view.roots.sheet;

import javafx.scene.control.Tab;
import javafx.scene.control.TabPane;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;
import org.golarion.model.Sheet;
import org.golarion.view.roots.GRoot;

public class SheetView implements GRoot
{

    private final Pane root;

    public SheetView(Sheet relatedSheet)
    {

        root = new AnchorPane();
        root.setPrefSize(800, 600);

        TabPane tabPane = new TabPane();
        tabPane.setPrefSize(800, 600);


        CharacteristicView cView = new CharacteristicView(relatedSheet.getStats());
        SpellListView sView = new SpellListView(relatedSheet.getSpellList());


        Pane principal = new Pane(cView.getRoot());
        Pane additional = new Pane();
        Pane spells = new Pane(sView.getRoot());
        Pane configuration = new Pane();

        String[] names = {"Principale", "Addizionale", "Incantesimi", "Configurazione"};
        Pane[] panes = {principal, additional, spells, configuration};
        for (int i = 0; i < names.length; i++)
        {
            Tab tab = new Tab(names[i], panes[i]);
            tab.setClosable(false);
            tabPane.getTabs().add(tab);
        }


        root.getChildren().addAll(tabPane);
    }


    @Override
    public Pane getRoot()
    {
        return root;
    }
}
