import json
import os

import unicodedata


def main():
    with open("app/src/main/resources/org/golarion/spells.json", "r", encoding = "utf-8") as f:
        data = f.read()

    data = data.replace("\\\"", "'")
    data = data.encode("utf-8").decode("unicode_escape")
    data = ''.join(c for c in unicodedata.normalize('NFD', data) if unicodedata.category(c) != 'Mn')

    data = json.loads(data)

    for spell in data.values():
        #     print(spell["Nome"])
        #     if "Tempo di Lancio" in spell:
        #         spell["castTimeDescription"] = spell["Tempo di Lancio"][-1]
        #         spell["Tempo di Lancio"] = spell["Tempo di Lancio"][:-1]
        #     if "Raggio di Azione" in spell:
        #         spell["rangeDescription"] = spell["Raggio di Azione"][-1]
        #         spell["Raggio di Azione"] = spell["Raggio di Azione"][:-1]
        #     if "Durata" in spell:
        #         spell["durationDescription"] = spell["Durata"][-1]
        #         spell["Durata"] = spell["Durata"][:-1]
        #     if "Tiro Salvezza" in spell:
        #         spell["savingThrowDescription"] = spell["Tiro Salvezza"][-1]
        #         spell["Tiro Salvezza"] = spell["Tiro Salvezza"][:-1]
        if "Resistenza agli Incantesimi" in spell:
            spell["spellResistanceDescription"] = spell["Resistenza agli Incantesimi"][-1]
            spell["Resistenza agli Incantesimi"] = spell["Resistenza agli Incantesimi"][:-1]

    for file in os.listdir("data/html"):
        if file == "table.html":
            continue
        normalized_name = ''.join(c for c in unicodedata.normalize('NFD', file) if unicodedata.category(c) != 'Mn')
        data[normalized_name.replace(".html", "")]["Link"] = f"https://golarion.altervista.org/wiki/Incantesimi/{file.replace(".html", "").replace(" ", "_")}"

    with open("app/src/main/resources/org/golarion/spells.json", "w", encoding = "utf-8") as f:
        json.dump(data, f, indent = 4)


if __name__ == '__main__':
    main()
