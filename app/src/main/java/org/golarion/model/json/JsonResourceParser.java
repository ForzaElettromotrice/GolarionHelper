package org.golarion.model.json;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import lombok.NonNull;

import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class JsonResourceParser
{
    private JsonResourceParser()
    {
    }

    public static JsonObject parse(@NonNull String resourcePath)
    {
        try (InputStream inputStream = JsonResourceParser.class.getResourceAsStream(resourcePath))
        {
            if (inputStream == null)
            {
                throw new IllegalArgumentException("resource not found: " + resourcePath);
            }

            try (Reader reader = new InputStreamReader(inputStream, StandardCharsets.UTF_8))
            {
                return JsonParser.parseReader(reader).getAsJsonObject();
            }
        }
        catch (IOException exception)
        {
            throw new IllegalArgumentException("failed to read resource: " + resourcePath, exception);
        }
    }

    public static List<JsonObject> parseJsonLines(@NonNull String resourcePath)
    {
        List<JsonObject> jsonObjects = new ArrayList<>();

        try (InputStream inputStream = JsonResourceParser.class.getResourceAsStream(resourcePath))
        {
            if (inputStream == null)
            {
                throw new IllegalArgumentException("resource not found: " + resourcePath);
            }

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream, StandardCharsets.UTF_8)))
            {
                String line;
                int lineNumber = 0;

                while ((line = reader.readLine()) != null)
                {
                    lineNumber++;
                    String trimmedLine = line.trim();
                    if (trimmedLine.isBlank())
                    {
                        continue;
                    }

                    try
                    {
                        jsonObjects.add(JsonParser.parseString(trimmedLine).getAsJsonObject());
                    }
                    catch (RuntimeException exception)
                    {
                        throw new IllegalArgumentException(
                                "failed to parse jsonl resource: " + resourcePath + " at line " + lineNumber,
                                exception
                        );
                    }
                }
            }
        }
        catch (IOException exception)
        {
            throw new IllegalArgumentException("failed to read resource: " + resourcePath, exception);
        }

        return List.copyOf(jsonObjects);
    }
}
