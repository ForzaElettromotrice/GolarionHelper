import json
import os

import unicodedata

from logs import log_debug

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


def parse_domain(spell: dict) -> None:
    splitted = spell["Dominio"].lower().replace("dominio", "").replace("sotto", "").replace("dell'", "").replace("della", "").replace("del", "").split(",")

    out = [(x[:-1].strip(), x[-1].strip()) for x in splitted if x.strip()]
    spell["Dominio"] = out


def parse_stirpe(spell: dict) -> None:
    splitted = spell["Stirpe"].lower().replace("stirpe", "").split(",")

    out = [(x[:-1].strip(), x[-1].strip()) for x in splitted if x.strip()]
    spell["Stirpe"] = out


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


def parse_duration(spell: dict) -> None:
    duration = spell["Durata"]

    out = []

    if "giorni" in duration.lower():
        out.append("giorni")
    if "ora/livello" in duration.lower():
        out.append("ore")
    if "minuti" in duration.lower():
        out.append("minuti")
    if "minuto" in duration.lower():
        out.append("minuti")
    if "round" in duration.lower():
        out.append("round")
    if "istantanea" in duration.lower():
        out.append("istantaneo")
    if "istantaneo" in duration.lower():
        out.append("istantaneo")
    out.append(duration)

    spell["Durata"] = out


def parse_range(spell: dict) -> None:
    range_ = spell["Raggio di Azione"]

    out = []

    if "personale" in range_.lower():
        out.append("personale")
    if "contatto" in range_.lower():
        out.append("contatto")
    if "vicino" in range_.lower():
        out.append("vicino")
    if "medio" in range_.lower():
        out.append("medio")
    if "lontano" in range_.lower():
        out.append("lontano")
    if "illimitato" in range_.lower():
        out.append("illimitato")
    out.append(range_)

    spell["Raggio di Azione"] = out


def parse_saving_throw(spell: dict) -> None:
    saving_throw = spell["Tiro Salvezza"]

    out = []

    if "nessuno" in saving_throw.lower():
        out.append("nessuno")
    if "tempra" in saving_throw.lower():
        out.append("tempra")
    if "riflessi" in saving_throw.lower():
        out.append("riflessi")
    if "volonta" in saving_throw.lower():
        out.append("volonta")
    out.append(saving_throw)

    spell["Tiro Salvezza"] = out


def parse_spell_resistance(spell: dict) -> None:
    spell_resistance = spell["Resistenza agli Incantesimi"]

    out = []

    if "no " in spell_resistance.lower():
        out.append("no")
    if "si " in spell_resistance.lower():
        out.append("si")

    out.append(spell_resistance)

    spell["Resistenza agli Incantesimi"] = out


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

        if "Dominio" in spell:
            parse_domain(spell)

        if "Stirpe" in spell:
            parse_stirpe(spell)

        if SCUOLA_ELEMENTALE in spell:
            parse_school_elemental(spell)

        if "Componenti" in spell:
            parse_components(spell)

        if "Durata" in spell:
            parse_duration(spell)

        if "Raggio di Azione" in spell:
            parse_range(spell)

        if "Tiro Salvezza" in spell:
            parse_saving_throw(spell)

        if "Resistenza agli Incantesimi" in spell:
            parse_spell_resistance(spell)

        log_debug(f"Spell {spell['Nome']} processed")

    for file in os.listdir("data/html"):
        if file == "table.html":
            continue
        normalized_name = ''.join(c for c in unicodedata.normalize('NFD', file) if unicodedata.category(c) != 'Mn')
        data[normalized_name.replace(".html", "")]["Link"] = f"https://golarion.altervista.org/wiki/Incantesimi/{file.replace(".html", "").replace(" ", "_")}"

    with open("data/spells.json", "w", encoding = "utf-8") as f:
        json.dump(data, f, indent = 4)


if __name__ == '__main__':
    main()
