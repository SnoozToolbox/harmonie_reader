// Your First C++ Program

#include <iostream>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    
    std::vector<std::string> filenames;
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4182.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\C5001.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4016.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4052.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4053.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4057.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4058.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4117.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4121.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4130.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4131.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4162.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4166.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4175.sts");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4176.STS");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4180.STS");
    filenames.push_back("C:\\projects\\data\\harmonie\\soraya\\sts\\P4183.STS");
    
    std::cout << "Fix events offset" << std::endl;
    for (std::string filename : filenames) {
        std::cout << "Opening file... :" << filename << std::endl;
        bool res = reader.openFile(filename);

        if (!res) {
            std::cout << "ERROR Failed to open the file:" << filename << std::endl;
            return 0;
        } else {
            std::cout << "SUCCESS" << std::endl;
        }

        std::vector<CPSGFile::EVITEM> events = reader.getEvents();
        
        // Remove all old events
        int count = 0;
        for (const auto& event: events) {
            if (event.Name.rfind("art_", 0) == 0) { // pos=0 limits the search to the prefix
                count++;
                reader.removeEventsByName(event.Name, event.GroupName);
            }
        }
        std::cout << "Events removed:" << events.size() << "\n";
        
        // Write new events
        count = 0;
        for (const auto& event: events) {
            if (event.Name.rfind("art_", 0) == 0) { // pos=0 limits the search to the prefix
                count++;

                // Fix the time based on the sample and the true sample rate
                double correctStartTime = event.StartSample / reader.m_file->GetTrueSampleFrequency();
                double correctDuration = event.SampleLength / reader.m_file->GetTrueSampleFrequency();
                reader.addEvent(event.Name, event.GroupName, correctStartTime, correctDuration, event.Channels, 0);
            }
        }
        std::cout << "Events added:" << events.size() << "\n";

            // Save file
        std::cout << "Saving file..." << std::endl;
        reader.saveFile();
        std::cout << "Closing file..." << std::endl;
        reader.closeFile();
    }
    std::cout << "DONE" << std::endl;
    return 0;
}
