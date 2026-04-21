#  memray run ./test/python/test_performance.py
#
from sys import platform
import gc
import sys
sys.path.insert(0, '')
from memory_profiler import profile

if platform == "linux" or platform == "linux2":
    import build.HarmonieReader as HarmonieReader
elif platform == "darwin":
    import build.HarmonieReader as HarmonieReader
elif platform == "win32":
    import build.Release.HarmonieReader as HarmonieReader
    
@profile
def main():

    print(f"Test performance")
    montageIndex = 0
    channels=["C3-A1"]
    filename = ""
    if filename == "":
        print("ERROR: No filename specified")
        quit()

    for j in range(100):
        for i in range(10):
            print(f"{j} {i} Testing")
            reader = HarmonieReader.HarmonieReader()
            print(f"Opening file... :{filename}")
            res = reader.open_file(filename)
            section_count = reader.get_signal_section_count()
            for i in range(section_count):
                signal_models = reader.get_signal_section(montageIndex, channels, i)
                for signal_model in signal_models:
                    print(f"Signal size: {len(signal_model.samples)}")
            reader.get_montages()
            reader.get_events()
            reader.get_event_groups()
            reader.get_extensions()
            reader.get_extensions_filters()
            reader.get_sleep_stages()
            reader.get_subject_info()
            reader.get_recording_start_time()
            reader.close_file()
            del reader
            print("Closed file")
            gc.disable()
            gc.collect()
            gc.enable()
    
    print("Done")

main()