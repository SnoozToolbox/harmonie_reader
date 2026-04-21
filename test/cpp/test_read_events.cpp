// Your First C++ Program

#include <iostream>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_read_events" << std::endl;

    std::string filename = "";
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

    std::vector<CPSGFile::EVGROUP> eventGroups = reader.getEventGroups();
    std::cout << "Event groups count:" << eventGroups.size() << "\n";
    for (int i = 0; i < eventGroups.size(); i++) {
        std::cout << "  " << eventGroups[i].Name << " index:" << i<< std::endl;
    }

    std::vector<CPSGFile::EVITEM> events = reader.getEvents();
    
    std::cout << "Events count:" << events.size() << "\n";
    int count = 0;
    for (const auto& event: events) {
        if (event.Name.compare("test_evt") == 0) {
            count++;
            //std::cout << "  " << event.Name << " (" << event.GroupName << ")  " << event.StartTime << "  " << event.TimeLength << std::endl;
        }
    }
    std::cout << count << std::endl;

    std::cout << "DONE" << std::endl;
    return 0;
}
