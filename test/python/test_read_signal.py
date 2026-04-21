
from sys import platform
import sys
sys.path.insert(0, '')
import matplotlib.pyplot as plt

if platform == "linux" or platform == "linux2":
    import build.HarmonieReader as HarmonieReader
elif platform == "darwin":
    import build.HarmonieReader as HarmonieReader
elif platform == "win32":
    import build.Release.HarmonieReader as HarmonieReader

reader = HarmonieReader.HarmonieReader()
print("Test: test_read_signal")

montageIndex = 0
channel_name="C3-A1"
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

print(f"Reading signal for montage:{montageIndex} channel:{channel_name}")
section_count = reader.get_signal_section_count()
for i in range(section_count):
    signal_model = reader.get_signal_section(montageIndex, channel_name, i)
    print(f"Signal size: {len(signal_model.samples)}")
    
    plt.plot(signal_model.samples)
    plt.show()
