import json

from classes import ClassWithSpell

if __name__ == '__main__':
    classe = ClassWithSpell("Mago", {"num_known_spells": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9], "spell_slots": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9], "magic_characteristics": 10, "spontaneus": False})

    print(classe)

    with open("../data/spells.json", "r", encoding="utf-8") as f:
        spells = json.load(f)

    print(spells["Abbinamento Meticoloso"])
    classe.spell_list.add_spell(spells["Abbinamento Meticoloso"],1)

