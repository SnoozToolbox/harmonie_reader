
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

montage_index = 0
channel_name=""
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

event_name = "test_write_events_name_" +  str(int(time.time()))
event_desc = "test_write_events_description"

start_time = 10
length = 1.5
number_of_events_to_create = 10

print(f"Adding {number_of_events_to_create} events  name:{event_name}")
channels = []
channels.append(channel_name)

for i in range(number_of_events_to_create):
    print(f"    event:{event_name} start:{start_time}")
    reader.add_event(event_name, group_name, start_time, length, channels, montage_index)
    start_time = start_time + 5

print("Saving file...")
reader.save_file()
print("Closing file...")
reader.close_file()

print("Checking if all events were created...")

validation_reader = HarmonieReader.HarmonieReader()
validation_reader.open_file(filename)

events = validation_reader.get_events()

number_of_events_created = 0
for e in events:
    if e.name == event_name:
        number_of_events_created = number_of_events_created + 1

if number_of_events_created == number_of_events_to_create:
    print("SUCCESS All events were properly created")
else:
    print("ERROR The new events were not all created")

validation_reader.close_file()
print("DONE")