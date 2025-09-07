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
                {
                    try
                    {
                        return CastTime.valueOf(e.getAsString().toUpperCase());
                    } catch (IllegalArgumentException ignored)
                    {
                        return null;
                    }
                }).toList()) : null;
        if (castTime != null && castTime.getLast() == null)
            castTime.removeLast();

        String castTimeDescription = jsonObject.has("Tempo di Lancio") ? jsonObject.get("Tempo di Lancio").getAsJsonArray().asList().getLast().getAsString() : "";

        List<GRange> range = jsonObject.has("Raggio di Azione") ? new ArrayList<>(jsonObject.get("Raggio di Azione").getAsJsonArray().asList().stream()
                .map(e ->
                {
                    try
                    {
                        return GRange.valueOf(e.getAsString().toUpperCase());
                    } catch (IllegalArgumentException ignored)
                    {
                        return null;
                    }
                }).toList()) : null;
        if (range != null && range.getLast() == null)
            range.removeLast();
        String rangeDescription = jsonObject.has("Raggio di Azione") ? jsonObject.get("Raggio di Azione").getAsJsonArray().asList().getLast().getAsString() : "";

        String target = jsonObject.has("Bersaglio") ? jsonObject.get("Bersaglio").getAsString() : "";
        String effect = jsonObject.has("Effetto") ? jsonObject.get("Effetto").getAsString() : "";
        String area = jsonObject.has("Area") ? jsonObject.get("Area").getAsString() : "";

        List<GDuration> duration = jsonObject.has("Durata") ? new ArrayList<>(jsonObject.get("Durata").getAsJsonArray().asList().stream()
                .map(e ->
                {
                    try
                    {
                        return GDuration.valueOf(e.getAsString().toUpperCase());
                    } catch (IllegalArgumentException ignored)
                    {
                        return null;
                    }
                }).toList()) : null;
        if (duration != null && duration.getLast() == null)
            duration.removeLast();
        String durationDescription = jsonObject.has("Durata") ? jsonObject.get("Durata").getAsJsonArray().asList().getLast().getAsString() : "";

        List<SavingThrow> savingThrow = jsonObject.has("Tiro Salvezza") ? new ArrayList<>(jsonObject.get("Tiro Salvezza").getAsJsonArray().asList().stream()
                .map(e ->
                {
                    try
                    {
                        return SavingThrow.valueOf(e.getAsString().toUpperCase());
                    } catch (IllegalArgumentException ignored)
                    {
                        return null;
                    }
                }).toList()) : null;
        if (savingThrow != null && savingThrow.getLast() == null)
            savingThrow.removeLast();
        String savingThrowDescription = jsonObject.has("Tiro Salvezza") ? jsonObject.get("Tiro Salvezza").getAsJsonArray().asList().getLast().getAsString() : "";

        List<String> spellResistance = jsonObject.has("Resistenza agli Incantesimi") ? new ArrayList<>(jsonObject.get("Resistenza agli Incantesimi").getAsJsonArray().asList().stream()
                .map(JsonElement::getAsString).toList()) : null;
        if (spellResistance != null)
            spellResistance.removeLast();

        String spellResistanceDescription = jsonObject.has("Resistenza agli Incantesimi") ? jsonObject.get("Resistenza agli Incantesimi").getAsJsonArray().asList().getLast().getAsString() : "";

        String description = jsonObject.has("Descrizione") ? jsonObject.get("Descrizione").getAsString() : "";

        return new Spell(name, school, descriptor, classLevels, domain, bloodline, components, castTime, castTimeDescription, range, rangeDescription, target, effect, area, duration, durationDescription, savingThrow, savingThrowDescription, spellResistance, spellResistanceDescription, description);
    }
}
