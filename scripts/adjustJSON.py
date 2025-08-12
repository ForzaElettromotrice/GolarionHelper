import json
import os

import unicodedata

SCUOLA_ELEMENTALE = "Scuola Elementale"


def parse_school(spell: dict) -> None:
    splitted = spell["Scuola"].split("[")

    ss = splitted[0].split("(")
    school = ss[0].strip()
    subschool = ss[1].replace(")", "").strip() if len(ss) > 1 else None

    descriptors = [x.strip().lower() for x in splitted[1].replace("]", "").replace("(solo per quanto riguarda il fetore)", "").replace("vedi testo", "").replace(";", "").replace(" o ", ",").split(",") if x.strip()] if len(splitted) > 1 else []

    spell["Scuola"] = school
    if subschool:
        spell["Sottoscuola"] = subschool
    if descriptors:
        spell["Descrittore"] = descriptors


def parse_school_elemental(spell: dict) -> None:
    splitted = spell[SCUOLA_ELEMENTALE].split(",")

    spell[SCUOLA_ELEMENTALE] = [x.strip().lower() for x in splitted if x.strip()]


def parse_components(spell: dict) -> None:
    components = []

    if "V" in spell["Componenti"]:
        components.append("V")
    if "S" in spell["Componenti"]:
        components.append("S")
    if "M " in spell["Componenti"]:
        components.append(("M", spell["Componenti"][spell["Componenti"].find("M") + 1:].split(")")[0][2:]))
    if "F " in spell["Componenti"]:
        components.append(("F", spell["Componenti"][spell["Componenti"].find("F") + 1:].split(")")[0][2:]))
    if "M/FD" in spell["Componenti"]:
        components.append(("M/FD", spell["Componenti"][spell["Componenti"].find("M/FD") + 4:].split(")")[0][2:]))
    if "F/FD," in spell["Componenti"]:
        components.append(("F/FD", ""))
    if "F/FD " in spell["Componenti"]:
        components.append(("F/FD", spell["Componenti"][spell["Componenti"].find("F/FD") + 4:].split(")")[0][2:]))
    spell["Componenti"] = components


def main():
    with open("data/spells.json", "r", encoding = "utf-8") as f:
        data = f.read()

    data = data.replace("\\\"", "'")
    data = data.encode("utf-8").decode("unicode_escape")
    data = ''.join(c for c in unicodedata.normalize('NFD', data) if unicodedata.category(c) != 'Mn')

    data = json.loads(data)

    for spell in data.values():
        if "Scuola" in spell:
            parse_school(spell)

        if SCUOLA_ELEMENTALE in spell:
            parse_school_elemental(spell)

        if "Componenti" in spell:
            parse_components(spell)

    for file in os.listdir("data/html"):
        if file == "table.html":
            continue
        normalized_name = ''.join(c for c in unicodedata.normalize('NFD', file) if unicodedata.category(c) != 'Mn')
        data[normalized_name.replace(".html", "")]["Link"] = f"https://golarion.altervista.org/wiki/Incantesimi/{file.replace(".html", "").replace(" ", "_")}"

    # with open("data/spells.json", "w", encoding = "utf-8") as f:
    #     json.dump(data, f, indent = 4)


if __name__ == '__main__':
    main()
