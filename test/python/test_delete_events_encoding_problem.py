
import time
from sys import platform
import sys

def get_unique_events():
    events = reader.get_events()
    unique_events = {}
    for event in events:
        if event.group not in unique_events:
            unique_events[event.group] = []
        
        if event.name not in unique_events[event.group]:
            unique_events[event.group].append(event.name)
    return unique_events

def print_groups():
    groups = reader.get_event_groups()

    print(f"Group count:{len(groups)}")
    for group in groups:
        print(f"    {group.name}")

def print_unique_events(unique_events):
    print(f"")
    for idx, group in enumerate(unique_events):
        print(f"{group}")
        for event in unique_events[group]:
            print(f"    {event}")

sys.path.insert(0, '')

if platform == "linux" or platform == "linux2":
    import build.HarmonieReader as HarmonieReader
elif platform == "darwin":
    import build.HarmonieReader as HarmonieReader
elif platform == "win32":
    import build.Release.HarmonieReader as HarmonieReader
    
filename = ""
if filename == "":
    print("ERROR: No filename specified")
    quit()

reader = HarmonieReader.HarmonieReader()
print("Test: test_delete_event_group_utf8")

### Step 1 - Display all events and groups
res = reader.open_file(filename)
print_groups()
#unique_events = get_unique_events()
#print_unique_events(unique_events)
reader.close_file()

### Step 2 - Add an event with UTF encoding
print("Adding event")
res = reader.open_file(filename)
utf_group = b'\xc3\x89cole'
utf_name = b'\xc3\x89vent'
#utf_group = b'\xc9cole' #latin1 encoding to test the encoding verification
#utf_name = b'\xc9vent' #latin1 encoding to test the encoding verification
success = reader.add_event(utf_name, utf_group, 0, 7, ["F3-A1"], 0)
if not success:
    print(reader.get_last_error())
    quit()
else:
    print("Adding success")
success = reader.save_file()
if not success:
    print(reader.get_last_error())
    quit()
else:
    print("Save success")
reader.close_file()

### Step 3 - Display all events and groups
res = reader.open_file(filename)
print_groups()
#unique_events = get_unique_events()
#print_unique_events(unique_events)
reader.close_file()

### Step 4 - Remove event by group name
print("Removing events by group...")
res = reader.open_file(filename)
#utf_group = b'\xc9cole' #latin1 encoding to test the encoding verification
#utf_name = b'\xc9vent' #latin1 encoding to test the encoding verification
success = reader.remove_events_by_name(utf_name, utf_group)
if not success:
    print(reader.get_last_error())
    quit()
else:
    print("Removed success")

reader.save_file()
reader.close_file()

### Step 5 - Display all events and groups
print(f"Opening file... :{filename}")
res = reader.open_file(filename)
print_groups()
res = reader.close_file()
