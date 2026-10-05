
"""
Anonymize Harmonie .sts patient/header metadata, then verify by reopening.

Usage (from repo root, after Release build of the Python module):
  python test/python/test_anonymize_subject_info.py

Set the variables in the CONFIG section below before running.
"""

import os
import sys
from sys import platform

sys.path.insert(0, "")

if platform == "linux" or platform == "linux2":
    import build.HarmonieReader as HarmonieReader
elif platform == "darwin":
    import build.HarmonieReader as HarmonieReader
elif platform == "win32":
    import build.Release.HarmonieReader as HarmonieReader


# ---------------------------------------------------------------------------
# CONFIG — edit these before running
# ---------------------------------------------------------------------------
filename = "E:\\CEAMS\\snooz_workspace\\Datasets\\MASS_SIG_STS\\Continues Files\\01-01-0001.sts"                          # Path to the source .sts file
replacement_id = "ANON0001"                # Patient ID written into the anonymized file
copy_before_anonymize = True           # True: keep source, write new files; False: overwrite/move source
rename_to_id = True                   # True: name output files after replacement_id
output_path = "E:\\CEAMS\\snooz_workspace\\Datasets\\MASS_SIG_STS\\Continues Files\\Anon"                       # Destination folder (or full .sts path). Empty = next to original.
keep_sex = True                        # True: keep gender; False: clear it
# ---------------------------------------------------------------------------


def find_sig(sts_path):
    base, _ = os.path.splitext(sts_path)
    for ext in (".sig", ".SIG"):
        candidate = base + ext
        if os.path.isfile(candidate):
            return candidate
    return None


def print_subject(label, info):
    print(f"--- {label} ---")
    print(f"  id:         {info.id}")
    print(f"  firstname:  {info.firstname}")
    print(f"  lastname:   {info.lastname}")
    print(f"  sex:        {info.sex}")
    print(f"  birth_date: {info.birth_date}")
    print(f"  age:        {info.age}")
    print(f"  height:     {info.height}")
    print(f"  weight:     {info.weight}")


def is_anonymized(info, expected_id):
    checks = [
        (info.id == expected_id, "id"),
        (info.firstname == "ANON", "firstname"),
        (info.lastname == "ANON", "lastname"),
        (info.birth_date == 0, "birth_date"),
        (info.height == 0, "height"),
        (info.weight == 0, "weight"),
    ]
    return [name for ok, name in checks if not ok]


def main():
    print("Test: test_anonymize_subject_info")

    if filename == "":
        print("ERROR: No filename specified")
        print("Set 'filename' at the top of this script before running.")
        quit(1)

    if not os.path.isfile(filename):
        print(f"ERROR: File not found: {filename}")
        quit(1)

    if not copy_before_anonymize:
        print("WARNING: copy_before_anonymize=False will overwrite/move the source files")

    reader = HarmonieReader.HarmonieReader()
    print(f"Opening: {filename}")
    if not reader.open_file(filename):
        print(f"ERROR Failed to open: {filename}")
        print(reader.get_last_error())
        quit(1)

    before = reader.get_subject_info()
    print_subject("BEFORE", before)
    original_firstname = before.firstname
    original_lastname = before.lastname

    print(f"Anonymizing with replacement_id={replacement_id} "
          f"copy={copy_before_anonymize} rename={rename_to_id} "
          f"out={output_path or '<next to original>'} ...")
    if not reader.anonymize_subject_info(replacement_id, keep_sex,
                                         copy_before_anonymize, rename_to_id,
                                         output_path):
        print(f"ERROR anonymize failed: {reader.get_last_error()}")
        quit(1)

    print("Saving file...")
    if not reader.save_file():
        print(f"ERROR save_file failed: {reader.get_last_error()}")
        quit(1)

    anon_sts = reader.get_filename()
    reader.close_file()
    print(f"Anonymized file: {anon_sts}")

    if copy_before_anonymize and not os.path.isfile(filename):
        print("ERROR source file disappeared while copying was requested")
        quit(1)

    anon_sig = find_sig(anon_sts)
    if anon_sig:
        print(f"Companion signal: {anon_sig}")
    else:
        print("WARNING: No companion .sig/.SIG next to the anonymized file")

    print("Reopening anonymized file for verification...")
    validation = HarmonieReader.HarmonieReader()
    if not validation.open_file(anon_sts):
        print(f"ERROR Failed to reopen: {anon_sts}")
        print(validation.get_last_error())
        quit(1)

    after = validation.get_subject_info()
    print_subject("AFTER", after)
    validation.close_file()

    failed = is_anonymized(after, replacement_id)
    if failed:
        print(f"ERROR anonymization incomplete for fields: {failed}")
        quit(1)

    if os.path.isfile(anon_sts + ".bak"):
        print(f"ERROR backup with original values was left behind: {anon_sts}.bak")
        quit(1)

    # Best-effort binary search: original names should not remain in the .sts
    with open(anon_sts, "rb") as f:
        data = f.read()
    for name in (original_lastname, original_firstname):
        if not name or name == "ANON":
            continue
        for encoding in ("latin-1", "utf-8"):
            try:
                needle = name.encode(encoding)
            except UnicodeEncodeError:
                continue
            if needle in data:
                print(f"WARNING: original string still present in file bytes: {name!r} ({encoding})")

    print(f"SUCCESS Subject info anonymized. Output file: {anon_sts}")
    print("DONE")


if __name__ == "__main__":
    main()
