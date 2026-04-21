// Your First C++ Program

#include <iostream>
#include <time.h>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;

    std::string filename = "";
    int montageIndex = 0;
    std::string channel = "C3-A2";
    if (filename == "") {
        std::cout << "ERROR: No filename specified" << std::endl;
        return 0;
    }

    std::cout << "Test: test_write_events_discontinuity" << std::endl;
    std::cout << "Opening file... :" << filename << std::endl;
    bool res = reader.openFile(filename);

    if (!res) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        return 0;
    } else {
        std::cout << "SUCCESS" << std::endl;
    }
        
    std::string groupName = "test_discontinuity_event";
    std::vector<std::string> channels;
    channels.push_back(channel);
    
    
    // Event inside section
    std::string eventName = "discontinuity_inside_section";
    double startTime = 500;
    double timeLength = 10;
    std::cout << "  event:\"" << eventName << "\" Start: " << startTime << " Length: " << timeLength << std::endl;
    reader.addEvent(eventName, groupName, startTime, timeLength, channels, montageIndex);

    // Event trunc
    eventName = "discontinuity_overlap_section";
    startTime = 605;
    timeLength = 10;
    std::cout << "  event:\"" << eventName << "\" Start: " << startTime << " Length: " << timeLength << std::endl;
    reader.addEvent(eventName, groupName, startTime, timeLength, channels, montageIndex);

    // Event inside discontinuity
    eventName = "discontinuity_inside_gap";
    startTime = 610;
    timeLength = 10;
    std::cout << "  event:\"" << eventName << "\" Start: " << startTime << " Length: " << timeLength << std::endl;
    reader.addEvent(eventName, groupName, startTime, timeLength, channels, montageIndex);

    // Event inside discontinuity
    eventName = "discontinuity_inside_second section";
    startTime = 2270;
    timeLength = 10;
    std::cout << "  event:\"" << eventName << "\" Start: " << startTime << " Length: " << timeLength << std::endl;
    reader.addEvent(eventName, groupName, startTime, timeLength, channels, montageIndex);

    // Save file
    std::cout << "Saving file..." << std::endl;
    reader.saveFile();
    std::cout << "Closing file..." << std::endl;
    reader.closeFile();
    
    std::cout << "DONE" << std::endl << std::endl;
    return 0;
}
