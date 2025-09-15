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

    private final CharacteristicView cView;
    private final SpellListView sView;

    public SheetView(Sheet relatedSheet)
    {
        root = new AnchorPane();
        root.setPrefSize(800, 600);

        TabPane tabPane = new TabPane();
        tabPane.setPrefSize(800, 600);


        cView = new CharacteristicView(relatedSheet.getStats());
        sView = new SpellListView(relatedSheet);


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


    public void update()
    {
        cView.update();
        sView.update();
    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
