import json
import os
from typing import Any

from bs4 import BeautifulSoup


def makeSpell(path:str)-> dict[str:Any]:
    with open(path, "r", encoding = "utf-8") as f:
        html = f.read()

    soup = BeautifulSoup(html, 'html.parser')

    paragraph = soup.find("p").text

    spell = {
        "Nome": os.path.basename(path).replace(".html",    "")
    }

    for line in paragraph.split("\n"):
        line = line.strip()
        if not line:
            continue
        if line.startswith("Livello"):
            spell["Livello"] = [tuple(x.strip().split(" ")) for x in line.split(":")[1].split(",")]
        for _type in ["Scuola", "Dominio", "Stirpe", "Componenti", "Tempo di Lancio", "Raggio di Azione", "Effetto", "Area", "Bersaglio", "Durata", "Tiro Salvezza", "Resistenza agli Incantesimi", "Descrizione"]:
            if line.startswith(_type):
                try:
                    spell[_type] = line.split(":")[1].strip()
                except IndexError:
                    spell[_type] = line.split(_type)[1].strip()
                break
    return spell


if __name__ == '__main__':

    spells = {}

    for file in os.listdir("../data/html"):
        if file == "table.html":
            continue
        spells[file.replace(".html", "")] = makeSpell(f"../data/html/{file}")

    with open("../data/spells.json", "w", encoding = "utf-8") as f:
        json.dump(spells, f, indent = 8)
