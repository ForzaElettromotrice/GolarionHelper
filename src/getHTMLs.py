import os.path

import requests
from bs4 import BeautifulSoup


def get_spell(link:str, name:str):
    if os.path.exists(f"./data/{name}.html"):
        return

    if not os.path.exists(f"../data/"):
        os.makedirs(f"../data/")

    name = name.replace("/", "_")
    page = requests.get(link)
    with open(f"../data/{name}.html", "w", encoding = "utf-8") as f:
        f.write(page.text)
    print(f"{name}.html written")



if __name__ == '__main__':

    if not os.path.exists("../data/table.html"):

        URL = "https://golarion.altervista.org/wiki/Database_Incantesimi"

        response = requests.get(URL)
        response.raise_for_status()

        with open(f"../data/table.html", "w", encoding = "utf-8") as f:
            f.write(response.text)

    with open(f"../data/table.html", "r", encoding = "utf-8") as f:
        tableHTML = f.read()

    soup = BeautifulSoup(tableHTML, 'html.parser')


    table = soup.find('table', id= "wiki_table_filter")

    for row in table.find_all('tr')[1:]:
            cells = row.find_all('td')
            if cells:
                get_spell("https://golarion.altervista.org/" + cells[0].find('a').get('href'), cells[0].text.strip())