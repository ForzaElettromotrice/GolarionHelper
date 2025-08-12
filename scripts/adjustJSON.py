import json

import unicodedata


def main():
    with open("data/spells.json", "r", encoding = "utf-8") as f:
        data = f.read()

    data = data.replace("\\\"", "'")
    data = data.encode("utf-8").decode("unicode_escape")
    data = ''.join(c for c in unicodedata.normalize('NFD', data) if unicodedata.category(c) != 'Mn')

    data = json.loads(data)

    for spell in data.values():
        if "Scuola" in spell:
            splitted = spell["Scuola"].split("[")

            print(spell["Nome"])

            ss = splitted[0].split("(")
            school = ss[0].strip()
            subschool = ss[1].replace(")", "").strip() if len(ss) > 1 else None

            domains = [x.strip() for x in splitted[1].replace("]", "").split(",")] if splitted == 1 else []
            print(school, subschool, domains)

            spell["Scuola"] = school
            if subschool:
                spell["Sottoscuola"] = subschool
            if domains:
                spell["Dominio"] = domains

    with open("data/spells.json", "w", encoding = "utf-8") as f:
        json.dump(data, f, indent = 4)


if __name__ == '__main__':
    main()
