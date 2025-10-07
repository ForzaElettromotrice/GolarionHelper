package org.golarion.view.roots.sheet;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import javafx.geometry.Pos;
import javafx.scene.Parent;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.layout.*;
import org.golarion.App;
import org.golarion.model.Sheet;
import org.golarion.model.spell.GClass;
import org.golarion.model.spell.Spell;
import org.golarion.model.spell.SpellParser;
import org.golarion.view.roots.GRoot;

import java.io.IOException;
import java.io.InputStreamReader;
import java.util.List;
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
        spellsBox.setStyle("-fx-background-color: #8d8d8d;");

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
        List<Spell> filtered;

        try (InputStreamReader is = new InputStreamReader(Objects.requireNonNull(App.class.getResourceAsStream("spells.json"))))
        {
            JsonObject spells = JsonParser.parseReader(is).getAsJsonObject();
            filtered = spells.entrySet()
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
                    }).map(stringJsonElementEntry -> SpellParser.parse(stringJsonElementEntry.getValue().getAsJsonObject())).toList();

        } catch (IOException | NullPointerException err)
        {
            logger.warning(err.getMessage());
            filtered = List.of();
        }

        spellsBox.getChildren().clear();
        for (Spell entry : filtered)
        {
            Label label = new Label(entry.name());
            label.setPrefSize(800, 50);
            label.setStyle("-fx-font-size: 20px; -fx-border-color: BLACK; -fx-border-width: 1px;");

            label.setOnMouseClicked(mouseEvent -> onClick(label, entry));
            label.setUserData(false);

            spellsBox.getChildren().add(label);
        }


    }

    private void onClick(Label label, Spell spell)
    {
        if ((boolean) label.getUserData())
        {
            spellsBox.getChildren().remove(spellsBox.getChildren().indexOf(label) + 1);
            label.setUserData(false);
        } else
        {
            Label school = new Label("Scuola: " + spell.school().getName() + " " + (spell.descriptor() == null ? "" : "[" + spell.descriptor().getName() + "]"));
            Label domain = new Label();
            if (spell.domain() != null)
                domain.setText(spell.domain().entrySet().stream().map(entry -> entry.getKey() + " " + entry.getValue()).reduce((a, b) -> a + ", " + b).orElse(""));

            VBox detail = new VBox(school, domain);

            spellsBox.getChildren().add(spellsBox.getChildren().indexOf(label) + 1, detail);
            label.setUserData(true);
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
