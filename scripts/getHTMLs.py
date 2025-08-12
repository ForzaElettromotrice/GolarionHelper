import os.path

import requests
from bs4 import BeautifulSoup

from logs import log_debug

BASE_PATH = "data/html"

start = False


def get_spell(link: str, name: str):
    global start
    print(name)

    if name == "Nebbia Solida":
        start = True

    if not start:
        return

    if os.path.exists(f"{BASE_PATH}/{name}.html"):
        return

    if not os.path.exists(BASE_PATH):
        os.makedirs(BASE_PATH)

    name = name.replace("/", "_")
    page = requests.get(link)
    with open(f"{BASE_PATH}/{name}.html", "w", encoding = "utf-8") as f:
        f.write(page.text)
    log_debug(f"{name}.html written")


if __name__ == '__main__':

    if not os.path.exists(f"{BASE_PATH}/table.html"):

        URL = "https://golarion.altervista.org/wiki/Database_Incantesimi"

        response = requests.get(URL)
        response.raise_for_status()

        if not os.path.exists(BASE_PATH):
            os.makedirs(BASE_PATH)

        with open(f"{BASE_PATH}/table.html", "w", encoding = "utf-8") as f:
            f.write(response.text)

    with open(f"{BASE_PATH}/table.html", "r", encoding = "utf-8") as f:
        tableHTML = f.read()

    soup = BeautifulSoup(tableHTML, 'html.parser')

    table = soup.find('table', id = "wiki_table_filter")

    for row in table.find_all('tr')[1:]:
        cells = row.find_all('td')
        if cells:
            get_spell("https://golarion.altervista.org/" + cells[0].find('a').get('href'), cells[0].text.strip())
