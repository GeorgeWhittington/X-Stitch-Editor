import csv
import json


class Thread:
    """Convenience class representing a single colour of thread"""
    def __init__(self, company, number, description, R, G, B):
        self.company = company
        self.number = number
        self.description = description
        self.R = R
        self.G = G
        self.B = B

    @property
    def colour(self):
        return self.R, self.G, self.B, 255


MANUFACTURERS = {}
CUSTOM_PALETTES = {}

with open("assets/palettes.json") as palette_file:
    palette_json = json.load(palette_file)
    for filename in palette_json["manufacturers"]:
        if not filename.endswith(".csv"):
            print("WARNING: Invalid manufacturer file in palettes.json")
            continue

        with open(f"assets/{filename}", newline='') as csv_file:
            manufacturer = filename[:-4]
            threads = {}

            reader = csv.DictReader(csv_file)
            for row in reader:
                threads[row["ID"]] = Thread(
                    manufacturer, row["ID"], row["Description"],
                    int(row["Red"]), int(row["Green"]), int(row["Blue"]))

            MANUFACTURERS[manufacturer] = threads

    # TODO: Should really be checking these are valid
    CUSTOM_PALETTES = palette_json["custom_palettes"]