package org.golarion.model.spell;


import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class SpellParser
{
    public static Spell parse(JsonObject jsonObject)
    {
        String name = jsonObject.get("Nome").getAsString();
        School school = School.valueOf(jsonObject.get("Scuola").getAsString().toUpperCase());
//        Descriptor descriptor = Descriptor.valueOf(jsonObject.get("descriptor").getAsString().toUpperCase());
//        Map<GClass, Integer> classLevels = GClass.parseClassLevels(jsonObject.get("Livello").getAsJsonObject());
        DomainList domain = DomainList.valueOf(jsonObject.get("Dominio").getAsString().toUpperCase());
//        Bloodline bloodline = Bloodline.valueOf(jsonObject.get("Stirpe").getAsString().toUpperCase());
        String components = jsonObject.get("Componenti").getAsString();
        String castTime = jsonObject.get("Tempo di Lancio").getAsString().toUpperCase();
        String range = jsonObject.get("Raggio di Azione").getAsString().toUpperCase();
//        String target = jsonObject.get("Bersaglio").getAsString();
//        String effect = jsonObject.get("Effetto").getAsString();
        String area = jsonObject.get("Area").getAsString();
        String duration = jsonObject.get("Durata").getAsString();
        String savingThrow = jsonObject.get("Tiro Salvezza").getAsString();
        String spellResistance = jsonObject.get("Resistenza agli Incantesimi").getAsString();
        String description = jsonObject.get("Descrizione").getAsString();

        JsonArray array = jsonObject.getAsJsonArray("Livello");

        for (JsonElement jsonElement : array)
        {
            for (JsonElement element : jsonElement.getAsJsonArray())
            {
                System.out.println(element.getAsString());
            }
        }

        return new Spell(name, school, null, null, domain, null, components, castTime, range,
                null, null, area, duration, savingThrow, spellResistance, description);
    }
}
