
import inspect
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
print("Test: test_read_channels")

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


print("Reading montages...")
montages = reader.get_montages()

if( len(montages)==0 ):
    print("ERROR No montage found")
    quit()
else:
    print(f"SUCCESS Montage count:{len(montages)}")

for m in montages:
    print(f" Montage:{m.name}")
    
    channels = reader.get_channels(m.index)
    print(f"  Channels ({len(channels)}")
    
    for ch in channels:
        print(f"     {ch.name}:{ch.unit}")
