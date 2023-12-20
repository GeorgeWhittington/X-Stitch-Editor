import csv
import json
import functools


class Thread:
    """Convenience class representing a single colour of thread"""
    def __init__(self, company: str, number: int, description: str,
                 R: int, G: int, B: int):
        self.company = company
        self.number = number
        self.description = description
        self.R = R
        self.G = G
        self.B = B

    @functools.cached_property
    def colour(self):
        return self.R, self.G, self.B, 255

    @functools.cached_property
    def global_identifier(self):
        return f"{self.company}:{self.number}"


MANUFACTURERS = {}
CUSTOM_PALETTES = {}
GLOBAL_THREAD_LOOKUP = {}

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
                # TODO: Should be checking for any duplicates
                thread = Thread(
                    manufacturer, row["ID"], row["Description"],
                    int(row["Red"]), int(row["Green"]), int(row["Blue"]))
                threads[row["ID"]] = thread
                GLOBAL_THREAD_LOOKUP[thread.global_identifier] = thread

            MANUFACTURERS[manufacturer] = threads

    # TODO: Should really be checking these are valid
    CUSTOM_PALETTES = palette_json["custom_palettes"]