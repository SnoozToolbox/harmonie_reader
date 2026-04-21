// Your First C++ Program

#include <iostream>
#include "../src/harmonie_reader.h"

using namespace Harmonie;

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_read_channels" << std::endl;

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

    std::cout << "Reading montages..." << std::endl;
    std::vector<CPSGFile::MONTAGE> montages = reader.getMontages();

    if( montages.size()==0 ) {
        std::cout << "ERROR No montage found" << std::endl;
        return 0;
    } else {
        std::cout << "SUCCESS Montage count:" << montages.size() << std::endl;
    }

    for(const auto& m: montages) {
        std::cout     << " Montages: " << m.MontageName << std::endl;
        
        std::vector<CPSGFile::CHANNEL> channels = reader.getChannels(m.Index);
        std::cout     << "  Channels (" << channels.size() << ")" << std::endl;
        
        for(const auto& ch: channels) {
            std::cout << "      " << ch.ChannelName << ":" << ch.TrueSampleFrequency <<"\n";
        }
    }

    std::cout << "DONE" << std::endl;
    return 0;
}
