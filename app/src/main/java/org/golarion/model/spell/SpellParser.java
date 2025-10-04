package org.golarion.model.spell;


import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class SpellParser
{
    public static Spell parse(JsonObject jsonObject)
    {
        String name = jsonObject.get("Nome").getAsString();
        School school = jsonObject.has("Scuola") ? School.valueOf(jsonObject.get("Scuola").getAsString().toUpperCase()) : null;
        Descriptor descriptor = jsonObject.has("Descrittore") ? Descriptor.valueOf(jsonObject.get("Descrittore").getAsJsonArray().get(0).getAsString().toUpperCase().replace(" ", "_")) : null;
        Map<GClass, Integer> classLevels = jsonObject.has("Livello") ? jsonObject.get("Livello").getAsJsonArray().asList().stream().collect(Collectors.toMap(
                e ->
                {
                    try
                    {
                        return GClass.valueOf(e.getAsJsonArray().get(0).getAsString().toUpperCase().replace(" ", "_"));
                    } catch (IllegalArgumentException ex)
                    {
                        return null;
                    }
                }
                ,
                e -> Integer.valueOf(e.getAsJsonArray().get(1).getAsString()))) : null;
        if (classLevels != null)
            classLevels.remove(null);
        Map<Domain, Integer> domain = null;
        if (jsonObject.has("Dominio"))
        {
            JsonArray domainArray = jsonObject.get("Dominio").getAsJsonArray();
            domain = domainArray.asList().stream().collect(Collectors.toMap(e ->
            {
                String domainName = e.getAsJsonArray().get(0).getAsString().toUpperCase().replace(" ", "_");
                try
                {
                    return DomainList.valueOf(domainName);
                } catch (IllegalArgumentException ex)
                {
                    return Subdomain.valueOf(domainName);
                }
            }, e -> Integer.valueOf(e.getAsJsonArray().get(1).getAsString())));
        }
        Map<Bloodline, Integer> bloodline = null;
        if (jsonObject.has("Stirpe"))
        {
            JsonArray bloodlineList = jsonObject.get("Stirpe").getAsJsonArray();
            bloodline = bloodlineList.asList().stream().collect(Collectors.toMap(
                    e -> Bloodline.valueOf(e.getAsJsonArray().get(0).getAsString().toUpperCase().toUpperCase()),
                    e -> Integer.valueOf(e.getAsJsonArray().get(1).getAsString())));
        }
        List<Components> components = jsonObject.has("Componenti") ? jsonObject.get("Componenti").getAsJsonArray().asList().stream()
                .map(e ->
                {
                    if (e.isJsonArray())
                        return new Components(e.getAsJsonArray().get(0).getAsString(), e.getAsJsonArray().get(1).getAsString());
                    return new Components(e.getAsString(), "");
                }).toList() : null;
        List<CastTime> castTime = jsonObject.has("Tempo di Lancio") ? new ArrayList<>(jsonObject.get("Tempo di Lancio").getAsJsonArray().asList().stream()
                .map(e ->
                        CastTime.valueOf(e.getAsString().toUpperCase())).toList()) : null;

        String castTimeDescription = jsonObject.has("castTimeDescription") ? jsonObject.get("castTimeDescription").getAsString() : "";

        List<GRange> range = jsonObject.has("Raggio di Azione") ? new ArrayList<>(jsonObject.get("Raggio di Azione").getAsJsonArray().asList().stream()
                .map(e ->
                        GRange.valueOf(e.getAsString().toUpperCase())).toList()) : null;

        String rangeDescription = jsonObject.has("rangeDescription") ? jsonObject.get("rangeDescription").getAsString() : "";

        String target = jsonObject.has("Bersaglio") ? jsonObject.get("Bersaglio").getAsString() : "";
        String effect = jsonObject.has("Effetto") ? jsonObject.get("Effetto").getAsString() : "";
        String area = jsonObject.has("Area") ? jsonObject.get("Area").getAsString() : "";

        List<GDuration> duration = jsonObject.has("Durata") ? new ArrayList<>(jsonObject.get("Durata").getAsJsonArray().asList().stream()
                .map(e ->
                        GDuration.valueOf(e.getAsString().toUpperCase())).toList()) : null;

        String durationDescription = jsonObject.has("durationDescription") ? jsonObject.get("durationDescription").getAsString() : "";

        List<SavingThrow> savingThrow = jsonObject.has("Tiro Salvezza") ? new ArrayList<>(jsonObject.get("Tiro Salvezza").getAsJsonArray().asList().stream()
                .map(e ->
                        SavingThrow.valueOf(e.getAsString().toUpperCase())).toList()) : null;

        String savingThrowDescription = jsonObject.has("savingThrowDescription") ? jsonObject.get("savingThrowDescription").getAsString() : "";

        List<String> spellResistance = jsonObject.has("Resistenza agli Incantesimi") ? new ArrayList<>(jsonObject.get("Resistenza agli Incantesimi").getAsJsonArray().asList().stream()
                .map(JsonElement::getAsString).toList()) : null;

        String spellResistanceDescription = jsonObject.has("spellResistanceDescription") ? jsonObject.get("spellResistanceDescription").getAsString() : "";

        String description = jsonObject.has("Descrizione") ? jsonObject.get("Descrizione").getAsString() : "";

        return new Spell(name, school, descriptor, classLevels, domain, bloodline, components, castTime, castTimeDescription, range, rangeDescription, target, effect, area, duration, durationDescription, savingThrow, savingThrowDescription, spellResistance, spellResistanceDescription, description);
    }
}
