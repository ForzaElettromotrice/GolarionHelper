package org.golarion.view.roots.sheet;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.ScrollPane;
import javafx.scene.layout.*;
import org.golarion.App;
import org.golarion.model.Sheet;
import org.golarion.model.spell.GClass;
import org.golarion.view.roots.GRoot;

import java.io.IOException;
import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.logging.Logger;

public class SpellsView implements GRoot
{
    private static final Logger logger = Logger.getLogger(SpellsView.class.getName());

    private final Sheet relatedSheet;

    private final AnchorPane root;
    private final Button returnButton;
    private final VBox spellsBox;

    private int level;

    public SpellsView(Sheet relatedSheet)
    {
        this.relatedSheet = relatedSheet;

        returnButton = new Button("Return");
        HBox hBox = new HBox(returnButton);
        hBox.setPrefSize(800, 25);
        hBox.setAlignment(Pos.CENTER_LEFT);
        hBox.setStyle("-fx-background-color: RED;");

        spellsBox = new VBox();
        spellsBox.setAlignment(Pos.CENTER);
        spellsBox.setStyle("-fx-background-color: BLUE;");

        ScrollPane scrollPane = new ScrollPane(spellsBox);
        scrollPane.setFitToWidth(true);
        scrollPane.setHbarPolicy(ScrollPane.ScrollBarPolicy.NEVER);
        scrollPane.setVbarPolicy(ScrollPane.ScrollBarPolicy.AS_NEEDED);

        VBox container = new VBox(hBox, scrollPane);
        container.setPrefSize(800, 600);
        container.setStyle("-fx-background-color: cyan;");
        VBox.setVgrow(scrollPane, Priority.ALWAYS);

        root = new AnchorPane(container);
        root.setPrefSize(800, 600);

    }

    public void update()
    {
        List<GClass> classes = relatedSheet.getClasses();

        try (InputStreamReader is = new InputStreamReader(Objects.requireNonNull(App.class.getResourceAsStream("spells.json"))))
        {
            JsonObject spells = JsonParser.parseReader(is).getAsJsonObject();
            List<Map.Entry<String, JsonElement>> filtered = spells.entrySet()
                    .stream()
                    .filter(entry ->
                    {
                        if (entry.getValue().getAsJsonObject().has("Livello"))
                            return entry.getValue().getAsJsonObject().getAsJsonArray("Livello").asList()
                                    .stream()
                                    .anyMatch(elm ->
                                    {
                                        try
                                        {
                                            return classes.contains(GClass.valueOf(elm.getAsJsonArray().asList().getFirst().getAsString().toUpperCase().replace(" ", "_")))
                                                    && elm.getAsJsonArray().asList().getLast().getAsInt() == level;
                                        } catch (IllegalArgumentException err)
                                        {
                                            logger.warning(err.getMessage());
                                            return false;
                                        }
                                    });
                        else
                        {
                            logger.warning(entry.getKey() + " need to be verified!");
                            return true;
                        }
                    }).toList();

        } catch (IOException | NullPointerException err)
        {
            logger.warning(err.getMessage());
        }


    }


    public void update(int level, Parent oldRoot)
    {
        this.level = level;
        returnButton.setOnAction(e -> root.getScene().setRoot(oldRoot));
        this.update();
    }

    @Override
    public Pane getRoot()
    {
        return root;
    }
}
