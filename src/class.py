from collections import namedtuple
from typing import Any

Spell = namedtuple("Spell", ["name", "level", "school", "domain", "stirpe", "components",
                             "casting_time", "range_", "effect", "area", "target", "duration",
                             "saving_throw", "spell_resistance", "description"])

class Class:
    def __init__(self, name: str):
        self._name = name

    def __repr__(self):
        return f"Classe({self._name})"


class ClassWithSpell(Class):
    def __init__(self, name: str, spells_specs:dict[str, Any]):
        super().__init__(name)
        self.spell_list = SpellList(spells_specs["num_known_spells"], spells_specs["spell_slots"],
                                     spells_specs["magic_characteristics"], spells_specs["spontaneus"])

    def __repr__(self):
        return f"ClasseMagica({self._name}, {self.spell_list})"


class SpellList:
    def __init__(self, known_spells: list[int], spell_slots: list[int], magic_characteristics: int, spontaneus: bool):
        self.__num_known_spells = known_spells
        self.__spell_slots = spell_slots
        self.__magic_characteristics = magic_characteristics
        self.__spells:list[list[Spell]] = []
        self.__spontaneus = spontaneus

    def update_num_known_spells(self, known_spells: list[int]):
        self.__num_known_spells = known_spells

    def update_spell_slots(self, spell_slots: list[int]):
        self.__spell_slots = spell_slots

    def add_spell(self, spell: Spell):
        if self.__spontaneus and self.__num_known_spells[spell.level] == len(self.__spells[spell.level]):
            raise ValueError("Non puoi aggiungere più incantesimi di quelli conosciuti!")

        self.__spells[spell.level].append(spell)

    def __repr__(self):
        return f"SpellList({"Spontaneo" if self.__spontaneus else "Preparato"})"


