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

    // Adding test group
    std::string groupName = "TestGroup";
    std::string groupDescription = "TestGroup Description";
    std::cout << "Adding event group... " << std::endl;
    std::cout << "  Group name: " << groupName << std::endl;
    std::cout << "  Description:" << groupDescription << std::endl << std::endl;

    // Create the test event
    
    std::string eventName = "test_event";
    std::string eventDesc = "test_event_description";

    double startTime = 7;
    double timeLength = 1.5;
    
    for (int i=0; i < 10; i++) {
        std::cout << "Adding event:\"" << eventName << "\" Start: " << startTime << " Length: " << timeLength <<std::endl;
        startTime += 6;
        std::vector<std::string> channels;
        channels.push_back(channel);

        // Add it 
        reader.addEvent(eventName, groupName, startTime, timeLength, channels, montageIndex);
    }

    // Save file
    std::cout << "Saving file..." << std::endl;
    reader.saveFile();
    std::cout << "Closing file..." << std::endl;
    reader.closeFile();

    // Validate if the event has been created
    HarmonieReader reader_to_delete;

    std::cout << "Reopening file... :" << filename << std::endl;
    res = reader_to_delete.openFile(filename);

    if (!res) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        return 0;
    } else {
        std::cout << "SUCCESS" << std::endl;
    }

    std::cout << "List of events before deleting:" << std::endl;
    std::vector<CPSGFile::EVITEM> events = reader_to_delete.getEvents();
    
    for (const auto& event: events) {
        std::cout << "  " << event.Name << std::endl;
    }

    std::cout << "Deleting event group: " << groupName << std::endl;
    reader_to_delete.removeEventsByGroup(groupName);

    // Save file
    std::cout << "Saving file..." << std::endl;
    reader_to_delete.saveFile();
    std::cout << "Closing file..." << std::endl << std::endl;
    reader_to_delete.closeFile();

    // Validate if the event has been created
    std::cout << "Checking if the event are really deleted..." << std::endl;
    HarmonieReader validation_reader;
    res = validation_reader.openFile(filename);

    events = validation_reader.getEvents();
    
    std::cout << "List of events after deleting:" << std::endl;
    
    for (const auto& event: events) {
        std::cout << "  \"" << event.Name << "\"" << std::endl;
    }

    for (const auto& event: events) {
        if( event.Name == eventName) {
            std::cout << "FAILED The event was not removed" << std::endl;
            validation_reader.closeFile();
            return 0;
        }
    }
    
    std::cout << "SUCCESS The events were correctly removed." << std::endl;

    std::cout << "Closing file..." << std::endl << std::endl;
    validation_reader.closeFile();

    std::cout << "DONE" << std::endl;
    return 0;
}
