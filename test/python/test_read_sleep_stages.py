
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

print(f"Reading sleep stages")
sleep_stages = reader.get_sleep_stages()
print(f"Sleep stages count:{len(sleep_stages)}")
for s in sleep_stages:
    print(f"{s.sleep_stage}\t {s.start_time}\t {s.duration}")
