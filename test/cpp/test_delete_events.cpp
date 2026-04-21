// Your First C++ Program

#include <iostream>
#include <time.h>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_write_events" << std::endl;
    
    std::string filename = "";
    std::string channel = "C3-A2";
    int montageIndex = 0;
    if (filename == "") {
        std::cout << "ERROR: No filename specified" << std::endl;
        return 0;
    }

    std::cout << "Opening file... :" << filename << std::endl;
    bool res = reader.openFile(filename);

    if (!res) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        return 0;
    } else {
        std::cout << "SUCCESS" << std::endl;
    }

    std::vector<CPSGFile::EVITEM> events = reader.getEvents();
    std::cout << "Events count at start: " << events.size() << std::endl;

    // Adding test group
    std::string groupName = "TestGroup11";
    std::string groupDescription = "TestGroup Description";
    //std::cout << "Adding event group... " << std::endl;
    //std::cout << "  Group name: " << groupName << std::endl;
    //std::cout << "  Description:" << groupDescription << std::endl << std::endl;

    // Create the test event
    std::string event1Name = "test_event_1";
    std::string event2Name = "test_event_2";
    std::string eventDesc = "test_event_description";

    double startTime = 7;
    double stepTime = 6;
    double timeLength = 1.5;
    int event1Count = 7;
    int event2Count = 10;
    std::vector<std::string> channels;
    channels.push_back(channel);

    for (int i=0; i < event1Count; i++) {
        startTime += stepTime;
        reader.addEvent(event1Name, groupName, startTime, timeLength, channels, montageIndex);
    }

    for (int i=0; i < event2Count; i++) {
        startTime += stepTime;
        reader.addEvent(event2Name, groupName, startTime, timeLength, channels, montageIndex);
    }

    // Save file
    reader.saveFile();
    reader.closeFile();

    // Validate if the event has been created
    HarmonieReader reader_to_delete;
    res = reader_to_delete.openFile(filename);

    if (!res) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        return 0;
    }

    std::vector<CPSGFile::EVITEM> events2 = reader_to_delete.getEvents();
    std::cout << "Events count before deleting: " << events2.size() << std::endl;
    std::cout << "Deleting " << event1Count << " events" << std::endl;
    reader_to_delete.removeEventsByName(event1Name, groupName);

    // Save file
    reader_to_delete.saveFile();
    reader_to_delete.closeFile();

    // Validate if the event has been created
    HarmonieReader validation_reader;
    res = validation_reader.openFile(filename);

    std::vector<CPSGFile::EVITEM> events3 = validation_reader.getEvents();
    
    std::cout << "Events count after deleting: " << events3.size() << std::endl;

    validation_reader.removeEventsByGroup(groupName);
    validation_reader.saveFile();
    validation_reader.closeFile();

    // Validate if the event has been created
    HarmonieReader reader_to_cleanup;
    res = reader_to_cleanup.openFile(filename);

    if (!res) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        return 0;
    }

    std::vector<CPSGFile::EVITEM> events4 = reader_to_cleanup.getEvents();
    
    std::cout << "Events count after clean up: " << events4.size() << std::endl;
    std::cout << "DONE" << std::endl;
    reader_to_cleanup.closeFile();
    return 0;
}
