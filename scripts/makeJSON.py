import json
import os

from bs4 import BeautifulSoup

from logs import log_debug


def make_spell(path: str) -> dict[str, str | list]:
    with open(path, "r", encoding = "utf-8") as f:
        html = f.read()

    soup = BeautifulSoup(html, 'html.parser')

    paragraph = soup.find("p").text

    spell: dict[str, str | list] = {
        "Nome": os.path.basename(path).replace(".html", ""),
        "Link": f"https://golarion.altervista.org/wiki/Incantesimi/{os.path.basename(path).replace(".html", "").replace(" ", "_")}"
    }

    for line in paragraph.split("\n"):
        line = line.strip()
        if not line:
            continue
        if line.startswith("Livello"):
            spell["Livello"] = [[x.strip()[:-2], x.strip()[-1]] for x in line.split(":")[1].split(",")]
            for i, (class_name, level) in enumerate(spell["Livello"][::-1]):
                if "/" in class_name:
                    spell["Livello"] += [(x.strip(), level) for x in class_name.split("/")]
                    spell["Livello"].remove([class_name, level])
                if class_name == "Sciamano 5 Stregone":
                    spell["Livello"] += [("Sciamano", "5"), ("Stregone", "5")]
                    spell["Livello"].remove(["Sciamano 5 Stregone", level])

        for _type in ["Scuola:", "Scuola Elementale", "Dominio", "Stirpe", "Componenti", "Tempo di Lancio", "Raggio di Azione", "Effetto", "Area", "Bersaglio", "Durata", "Tiro Salvezza", "Resistenza agli Incantesimi", "Descrizione"]:
            if line.startswith(_type):
                try:
                    spell[_type.replace(":", "")] = line.split(":")[1].strip()
                except IndexError:
                    spell[_type.replace(":", "")] = line.split(_type)[1].strip()
                break
    log_debug(f"Spell {spell['Nome']} processed")
    return spell


if __name__ == '__main__':

    spells = { }

    for file in os.listdir("data/html"):
        if file == "table.html":
            continue
        spells[file.replace(".html", "")] = make_spell(f"data/html/{file}")

    with open("data/spells.json", "w", encoding = "utf-8") as f:
        json.dump(spells, f, indent = 8)
