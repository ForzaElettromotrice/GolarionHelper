package org.golarion.view;

import javafx.scene.Scene;
import javafx.stage.Stage;
import org.golarion.view.roots.GRoot;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class SheetMan
{
    /**
     * Instance of the current class
     */
    private static SheetMan instance;

    /**
     * @return the instance of SheetMan
     */
    public static SheetMan getInstance()
    {
        if (instance == null) instance = new SheetMan();
        return instance;
    }

    private final Map<UUID, Stage> stages;
    private final Map<UUID, GRoot> roots;

    /**
     * Constructor for the {@link SheetMan} class.
     */
    private SheetMan()
    {
        stages = new HashMap<>();
        roots = new HashMap<>();
    }

    public void open(String title, UUID id, GRoot root)
    {
        if (roots.containsKey(id))
            throw new IllegalArgumentException("Sheet with id " + id + " already exists!");

        Stage stage = new Stage();
        stage.setTitle(title);
        stage.setScene(new Scene(root.getRoot()));

        roots.put(id, root);
        stages.put(id, stage);

        stage.setOnCloseRequest(e -> close(id));
        stage.show();
        root.getRoot().requestFocus();
    }

    public void close(UUID id)
    {
        if (!roots.containsKey(id))
            throw new IllegalArgumentException("Sheet with id " + id + " does not exist!");

        stages.get(id).close();
        stages.remove(id);
        roots.remove(id);
    }

    public void closeAll()
    {
        for (Stage s : stages.values())
            s.close();
        stages.clear();
        roots.clear();
    }
}
