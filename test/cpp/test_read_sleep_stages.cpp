// Your First C++ Program

#include <iostream>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_read_sleep_stages" << std::endl;

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

    std::vector<HarmonieReader::SLEEP_STAGE> sleepStages = reader.getSleepStages();

    std::cout << "SleepStages:" << sleepStages.size() << std::endl << std::endl;
    for (int i=0; i<sleepStages.size(); i++) {
        std::cout << sleepStages[i].sleepStage << " " << sleepStages[i].startTime << " " << sleepStages[i].duration << std::endl;
    }

    std::cout << "DONE" << std::endl;
    return 0;
}
