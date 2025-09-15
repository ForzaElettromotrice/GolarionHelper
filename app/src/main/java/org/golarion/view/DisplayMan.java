package org.golarion.view;

import javafx.scene.Scene;
import javafx.scene.layout.AnchorPane;
import javafx.scene.layout.Pane;
import javafx.stage.Stage;
import lombok.Setter;
import org.golarion.exceptions.IllegalBuildException;
import org.golarion.view.roots.GRoot;
import org.golarion.view.roots.MainMenu;
import org.golarion.view.roots.Roots;

public class DisplayMan
{
    public static class Builder
    {
        @Setter
        private static Stage window;

        public static void build()
        {
            if (instance != null)
                return;

            if (window == null)
                throw new IllegalBuildException("Can't create DisplayMan without the main window!");

            instance = new DisplayMan(window);
        }


        private Builder()
        {
        }

    }

    /**
     * Instance of the current class
     */
    private static DisplayMan instance;

    /**
     * @return the instance of DisplayMan
     */
    public static DisplayMan getInstance()
    {
        if (instance == null) Builder.build();
        return instance;
    }

    private final Stage window;
    private final GRoot[] roots;

    /**
     * Constructor for the {@link DisplayMan} class.
     */
    private DisplayMan(Stage primaryStage)
    {
        this.window = primaryStage;
        this.window.setScene(new Scene(new AnchorPane()));
        this.roots = new GRoot[Roots.values().length];
    }

    /**
     * Loads the root view based on the provided {@link Roots} enum.
     * If the root view is already loaded, it does nothing.
     *
     * @param root the root view to load
     */
    private void load(Roots root)
    {
        if (roots[root.getIndex()] != null)
            return;

        roots[root.getIndex()] = switch (root)
        {
            case MAIN_MENU -> new MainMenu();
        };
    }

    /**
     * Displays the root view based on the provided {@link Roots} enum.
     * It first loads the root view if it hasn't been loaded yet, and then sets the
     * scene root to the loaded view's pane.
     *
     * @param root the root view to display
     */
    public void display(Roots root)
    {
        load(root);
        Pane pane = roots[root.getIndex()].getRoot();
        window.getScene().setRoot(pane);
    }

    /**
     * Displays the main window
     */
    public void show()
    {
        window.show();

    }

    /**
     * Closes the main window
     */
    public void close()
    {
        SheetMan.getInstance().closeAll();
        window.close();
    }
}
