
"""
Anonymize Harmonie .sts patient/header metadata, then verify by reopening.

Usage (from repo root, after Release build of the Python module):
  python test/python/test_anonymize_subject_info.py path\\to\\recording.sts
  python test/python/test_anonymize_subject_info.py path\\to\\recording.sts SUBJ001

Works on a copy (*_anon.sts) so the original is left unchanged.
Also copies the companion .sig/.SIG if present.
Deletes the .bak created by save_file().
"""

import os
import shutil
import sys
from sys import platform

sys.path.insert(0, "")

if platform == "linux" or platform == "linux2":
    import build.HarmonieReader as HarmonieReader
elif platform == "darwin":
    import build.HarmonieReader as HarmonieReader
elif platform == "win32":
    import build.Release.HarmonieReader as HarmonieReader


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


def is_anonymized(info, replacement_id):
    checks = [
        (info.id == replacement_id, "id"),
        (info.firstname == "ANON", "firstname"),
        (info.lastname == "ANON", "lastname"),
        (info.birth_date == 0, "birth_date"),
        (info.height == 0, "height"),
        (info.weight == 0, "weight"),
    ]
    failed = [name for ok, name in checks if not ok]
    return failed


def main():
    print("Test: test_anonymize_subject_info")

    filename = ""
    replacement_id = "ANON"
    if len(sys.argv) >= 2:
        filename = sys.argv[1]
    if len(sys.argv) >= 3:
        replacement_id = sys.argv[2]

    # Or set a path here when running from the IDE:
    # filename = r"C:\path\to\recording.sts"

    if filename == "":
        print("ERROR: No filename specified")
        print("Usage: python test/python/test_anonymize_subject_info.py <file.sts> [replacement_id]")
        quit(1)

    if not os.path.isfile(filename):
        print(f"ERROR: File not found: {filename}")
        quit(1)

    base, ext = os.path.splitext(filename)
    anon_sts = base + "_anon" + ext

    print(f"Copying {filename} -> {anon_sts}")
    shutil.copy2(filename, anon_sts)

    sig_src = find_sig(filename)
    if sig_src:
        anon_sig = base + "_anon" + os.path.splitext(sig_src)[1]
        print(f"Copying companion signal {sig_src} -> {anon_sig}")
        shutil.copy2(sig_src, anon_sig)
    else:
        print("WARNING: No companion .sig/.SIG found (signal read not required for this test)")

    reader = HarmonieReader.HarmonieReader()
    print(f"Opening copy: {anon_sts}")
    if not reader.open_file(anon_sts):
        print(f"ERROR Failed to open: {anon_sts}")
        print(reader.get_last_error())
        quit(1)

    before = reader.get_subject_info()
    print_subject("BEFORE", before)
    original_lastname = before.lastname
    original_firstname = before.firstname

    print(f"Anonymizing with replacement_id={replacement_id} ...")
    if not reader.anonymize_subject_info(replacement_id, True):
        print(f"ERROR anonymize failed: {reader.get_last_error()}")
        quit(1)

    print("Saving file...")
    if not reader.save_file():
        print("ERROR save_file failed")
        quit(1)
    reader.close_file()

    bak = anon_sts + ".bak"
    if os.path.isfile(bak):
        print(f"Removing backup with original PHI: {bak}")
        os.remove(bak)

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

    # Best-effort binary search: original names should not remain in .sts
    if original_lastname and original_lastname not in ("ANON",):
        with open(anon_sts, "rb") as f:
            data = f.read()
        # Harmonie stores Latin-1 text; try common encodings
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
