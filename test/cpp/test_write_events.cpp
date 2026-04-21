// Your First C++ Program

#include <iostream>
#include <time.h>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_write_events" << std::endl;

    std::string filename = "";
    int montageIndex = 0;
    std::string channel = "C3-A2";
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
    //std::string eventName = "test_write_events_name_" + std::to_string(time(NULL));
    std::string eventName = "test_event";
    std::string eventDesc = "test_write_events_description";

    double startTime = 10;
    double timeLength = 5;
    int numberOfeventsToCreate = 10;
    std::cout << "Adding events..." << eventName << std::endl;

    std::vector<std::string> channels;
    channels.push_back(channel);

    for (int i=0; i < numberOfeventsToCreate; i++) {
        std::cout << "  event:\"" << eventName << "\" Start: " << startTime << " Length: " << timeLength <<std::endl;
        reader.addEvent(eventName, groupName, startTime, timeLength, channels, montageIndex);
        startTime += 5;
    }

    // Save file
    std::cout << "Saving file..." << std::endl;
    reader.saveFile();
    std::cout << "Closing file..." << std::endl;
    reader.closeFile();
    // Validate if the event has been created
    std::cout << "Checking if all events were created..." << std::endl;
    HarmonieReader validation_reader;
    res = validation_reader.openFile(filename);

    std::vector<CPSGFile::EVITEM> events = validation_reader.getEvents();
    
    int numberOfEventsCreated = 0;
    for (const auto& event: events) {
        if( event.Name == eventName) {
            numberOfEventsCreated++;
        }
    }
    
    if (numberOfeventsToCreate == numberOfEventsCreated) {
        std::cout << "SUCCESS All events were properly created" << std::endl;    
    } else {
        std::cout << "ERROR The new event was NOT found" << std::endl;    
    }
    
    validation_reader.closeFile();

    std::cout << "DONE" << std::endl << std::endl;
    return 0;
}
