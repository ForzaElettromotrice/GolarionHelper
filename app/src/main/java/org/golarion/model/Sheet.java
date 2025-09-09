package org.golarion.model;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import lombok.Getter;
import lombok.Setter;
import org.golarion.App;
import org.golarion.model.spell.Spell;
import org.golarion.model.spell.SpellList;
import org.golarion.model.spell.SpellParser;
import org.golarion.model.stats.Stats;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

@Setter
@Getter
public class Sheet
{

    private final Stats stats;
    private final SpellList spellList;

    public Sheet()
    {
        stats = new Stats();
        spellList = new SpellList();

        spellList.modifyMaxCounter(0, 3);
        spellList.modifyMaxCounter(1, 2);
        spellList.modifyMaxCounter(2, 1);
        spellList.modifyMaxCounter(3, 4);
        spellList.modifyMaxCounter(4, 1);
        spellList.modifyMaxCounter(5, 1);
        spellList.modifyMaxCounter(6, 1);
        spellList.modifyMaxCounter(7, 1);
        spellList.modifyMaxCounter(8, 1);
        spellList.modifyMaxCounter(9, 1);

        try (InputStream is = App.class.getResourceAsStream("spells.json"))
        {
            assert is != null;
            JsonElement element = JsonParser.parseReader(new InputStreamReader(is));

            JsonObject obj = element.getAsJsonObject();
            Spell spell = SpellParser.parse(obj.getAsJsonObject("Palla di Fuoco"));
            spellList.addSpell(3, spell);
            spellList.addSpell(3, spell);
            spellList.addSpell(3, spell);
            spellList.addSpell(3, spell);
            spellList.addSpell(3, spell);
        } catch (IOException ignored)
        {

        }


    }
}
