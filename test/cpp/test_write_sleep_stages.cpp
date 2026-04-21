// Your First C++ Program

#include <iostream>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_write_sleep_stages" << std::endl;

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

    reader.setSleepStage(0, 30, 3);
    reader.setSleepStage(30, 30, 3);
    reader.setSleepStage(60, 30, 2);
    reader.setSleepStage(90, 30, 1);
    reader.setSleepStage(120, 30, 0);
    reader.setSleepStage(150, 30, 1);
    reader.setSleepStage(180, 30, 1);
    reader.setSleepStage(210, 30, 1);
    reader.setSleepStage(240, 30, 2);
    reader.setSleepStage(270, 30, 2);
    reader.setSleepStage(300, 30, 1);
    reader.setSleepStage(330, 30, 2);
    reader.setSleepStage(360, 30, 2);
    reader.setSleepStage(390, 30, 2);
    reader.setSleepStage(420, 30, 2);
    reader.setSleepStage(450, 30, 2);
    reader.setSleepStage(480, 30, 2);
    reader.setSleepStage(510, 30, 2);
    reader.setSleepStage(540, 30, 2);
    reader.setSleepStage(570, 30, 2);
    reader.setSleepStage(600, 30, 2);
    reader.setSleepStage(630, 30, 2);
    reader.setSleepStage(660, 30, 2);
    reader.setSleepStage(690, 30, 5);
    reader.setSleepStage(720, 30, 5);
    reader.setSleepStage(750, 30, 5);
    reader.setSleepStage(780, 30, 0);
    reader.setSleepStage(810, 30, 0);
    reader.setSleepStage(840, 30, 0);
    reader.setSleepStage(870, 30, 0);
    reader.setSleepStage(900, 30, 0);
    reader.setSleepStage(930, 30, 0);
    reader.setSleepStage(960, 30, 0);
    reader.setSleepStage(990, 30, 1);
    reader.setSleepStage(1020, 30, 1);
    reader.setSleepStage(1050, 30, 1);
    reader.setSleepStage(1080, 30, 2);
    reader.setSleepStage(1110, 30, 1);
    reader.setSleepStage(1140, 30, 2);
    reader.setSleepStage(1170, 30, 2);
    reader.setSleepStage(1200, 30, 2);
    reader.setSleepStage(1230, 30, 2);
    reader.setSleepStage(1260, 30, 2);
    reader.setSleepStage(1290, 30, 2);
    reader.setSleepStage(1320, 30, 2);
    reader.setSleepStage(1350, 30, 2);
    reader.setSleepStage(1380, 30, 2);
    reader.setSleepStage(1410, 30, 2);
    reader.setSleepStage(1440, 30, 2);
    reader.setSleepStage(1470, 30, 5);
    reader.setSleepStage(1500, 30, 5);
    reader.setSleepStage(1530, 30, 5);
    reader.setSleepStage(1560, 30, 5);
    reader.setSleepStage(1590, 30, 5);
    reader.setSleepStage(1620, 30, 5);
    reader.setSleepStage(1650, 30, 5);
    reader.setSleepStage(1680, 30, 5);
    reader.setSleepStage(1710, 30, 5);
    reader.setSleepStage(1740, 30, 5);
    reader.setSleepStage(1770, 30, 5);

    reader.saveFile();

    res = reader.openFile(filename);

    if (!res) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        return 0;
    } else {
        std::cout << "SUCCESS" << std::endl;
    }

    sleepStages = reader.getSleepStages();

    std::cout << "SleepStages:" << sleepStages.size() << std::endl << std::endl;
    for (int i=0; i<sleepStages.size(); i++) {
        std::cout << sleepStages[i].sleepStage << " " << sleepStages[i].startTime << " " << sleepStages[i].duration << std::endl;
    }

    std::cout << "DONE" << std::endl;
    return 0;
}
