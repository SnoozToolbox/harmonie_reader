
import time
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
print("Test: test_write_events")

filename = ""
if filename == "":
    print("ERROR: No filename specified")
    quit()

print(f"Opening file... :{filename}")
res = reader.open_file(filename)

# Adding a test group
group_name = "TestGroup"
group_desc = "TestGroup description"

print(f"Adding event group...")
print(f"  Group name:{group_name}")
print(f"  Group desc:{group_desc}")

group_index = reader.add_event_group(group_name, group_desc)

channel = "C3-A2"
event_name = "test_delete_event_group_name_" +  str(int(time.time()))
event_desc = "test_delete_event_group_description"

start_time = 10
length = 1.5
montage_index = 0
number_of_events_to_create = 10

print(f"Adding {number_of_events_to_create} events  name:{event_name}")
channels = []
channels.append(channel)

for i in range(number_of_events_to_create):
    print(f"    event:{event_name} start:{start_time}")
    reader.add_event(event_name, group_name, start_time, length, channels, montage_index)
    start_time = start_time + 5

print("Saving file...")
reader.save_file()
print("Closing file...")
reader.close_file()

print(f"Reopening file...:{filename}")

reader_to_delete = HarmonieReader.HarmonieReader()
reader_to_delete.open_file(filename)

print("List of events before deleting:")
events = reader_to_delete.get_events()

for e in events:
    print(f"    {e.name}")

print(f"Deleting event group:{group_name}")
reader_to_delete.remove_events_by_group(group_name)

print("Saving file...")
reader_to_delete.save_file()

print("Closing file...")
reader_to_delete.close_file()

print("Checking if the events were really deleted...")
validation_reader = HarmonieReader.HarmonieReader()
validation_reader.open_file(filename)
events = validation_reader.get_events()

for e in events:
    print(f"    {e.name}")

for e in events:
    if e.name == event_name:
        print("FAILED The event was not removed")
        validation_reader.close_file()
        quit()

print("SUCCESS The events were correctly removed.")
validation_reader.close_file()
print("DONE")
