
from sys import platform
import sys
sys.path.insert(0, '')

if platform == "linux" or platform == "linux2":
    import build.HarmonieReader as HarmonieReader
elif platform == "darwin":
    import build.HarmonieReader as HarmonieReader
elif platform == "win32":
    import build.Release.HarmonieReader as HarmonieReader

reader = HarmonieReader.HarmonieReader()
print("Test: test_read_sleep_stages")

filename = ""
if filename == "":
    print("ERROR: No filename specified")
    quit()

print(f"Opening file... :{filename}")
res = reader.open_file(filename)

if (not res):
    print(f"ERROR Failed to open the file:{filename}")
    quit()
else:
    print("SUCCESS")

print(f"Reading subject info...")
subject_info = reader.get_subject_info()
print(f"subject info:{subject_info}")
print(f"id: {subject_info.id}")
print(f"firstname: {subject_info.firstname}")
print(f"lastname: {subject_info.lastname}")
print(f"sex: {subject_info.sex}")
print(f"birthDate: {subject_info.birth_date}")
print(f"creationDate: {subject_info.creation_date}")
print(f"age: {subject_info.age}")
print(f"height: {subject_info.height}")
print(f"weight: {subject_info.weight}")
print(f"bmi: {subject_info.bmi}")
print(f"waistSize: {subject_info.waist_size}")

print("DONE")
