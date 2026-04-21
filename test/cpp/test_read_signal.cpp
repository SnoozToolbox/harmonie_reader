// Your First C++ Program

#include <iostream>
#include <iomanip>
#include "../src/harmonie_reader.h"
#include <vector>

using namespace Harmonie;
void convert_seconds(double seconds);

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_read_signal" << std::endl;

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

    std::cout << "Reading signal..." << std::endl;
    //std::vector<double> signal_512 = reader.getSignal(0,"T6"); // 512
    int sectionCount = reader.getSignalSectionCount();
    std::cout << "Section count:" << sectionCount << std::endl;
    
    std::vector<std::string> channels;
    channels.push_back(channel);

    for (int i=0; i<sectionCount; i++) {
        std::vector<SignalModel> signalModel = reader.getSignalSection(montageIndex, channels, i);
        std::cout << "[" << i << "] section size:"<< signalModel[0].samples.size() << "  :" << signalModel[0].samples[0] << std::endl;
        for (int j = 0; j < 10; j++) {
            std::cout << signalModel[i].samples[j] << std::endl;
        }
    }
       
    std::cout << "DONE" << std::endl;
    return 0;
}

void convert_seconds(double seconds) {
    unsigned int hours = seconds / 3600;
    unsigned int minutes = ((int)seconds % 3600) / 60;
    unsigned int secs = (int)seconds % 60;
    unsigned int milliseconds = (seconds - (hours*3600) - (minutes*60) - secs) * 1000;

    std::cout << hours << " hours " << minutes << " minutes " << secs << " seconds " << milliseconds << " milliseconds" << std::endl;
}