
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
print("Test: test_read_events")

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

print("Reading events...")
events = reader.get_events()

if( len(events)==0 ):
    print("ERROR No events found")
    quit()
else:
    print(f"SUCCESS events count:{len(events)}")

print(f"Events count:{len(events)}")
for e in events:
    if e.name == "test_event":
        print(f" {e.name} ({e.group}) {e.start_time} {e.duration}")
    
event_groups = reader.get_event_groups()
print(f"Event groups count:{len(event_groups)}")
for g in event_groups:
    print(f" {g.name}")
    