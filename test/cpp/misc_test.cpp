// Your First C++ Program

#include <sstream>
#include <iostream>
#include <iomanip>
#include "../src/harmonie_reader.h"
#include <cmath>
#include <limits>
#include <chrono>
#include <thread>

using namespace Harmonie;
using namespace std;

bool compareDouble(double x, double y, int precision) {
    double diff = abs(x - y);
    double epsilon = 1.f/precision;
    return diff <= epsilon;
}

bool compareVectors(const vector<double>& a, const vector<double>& b) {
    return equal(a.begin(), a.end(), b.begin());
}

void myAssert(bool condition, string functionName, string message) {
    if (!condition) {
        cout << "Assertion \033[1;31mFailed\033[0m: " << functionName << ":" << message << endl;
    } else {
        cout << "Assertion \033[1;35mSuccess\033[0m: " << functionName << ":" << message << endl;
    }
}

void test_getSignalSectionCount() {
    HarmonieReader reader;

    // File with a discontinuity
    string filename = "C:\\projects\\data\\harmonie\\soraya\\discontinuity\\01-03-0004.sts";
    reader.openFile(filename);
    uint32_t count = reader.getSignalSectionCount();
    uint32_t expected_value = 2;
    stringstream ss;
    ss << " expected value:" << expected_value << " value received:" << count;
    myAssert(count == expected_value, "test_getSignalSectionCount", ss.str());
    reader.closeFile();

    // File without a discontinuity
    filename = "C:\\projects\\data\\harmonie\\A14121-N1.sts";
    reader.openFile(filename);
    count = reader.getSignalSectionCount();
    expected_value = 1;
    ss.clear();
    ss << " expected value:" << expected_value << " value received:" << count;
    myAssert(count == expected_value, "test_getSignalSectionCount", ss.str());
    reader.closeFile();
}

void test_getSignalBySection() {
    HarmonieReader reader;
    string filename = "C:\\projects\\data\\harmonie\\soraya\\discontinuity\\01-03-0004.sts";
    reader.openFile(filename);
    int montageIndex = 0;
    string channelName = "T6";
    int sectionIndex = 0;

    auto startTime = std::chrono::high_resolution_clock::now();
    /*unique_ptr<SignalModel> signalModel = reader.getSignalSection(montageIndex, channelName, sectionIndex);
    myAssert(signalModel->channel == "T6", "test_getSignalBySection.channel", " invalid channel");
    myAssert(compareDouble(signalModel->startTime, 0, 7), "test_getSignalBySection.startTime", " invalid startTime");
    myAssert(compareDouble(signalModel->endTime, 608.70713806152344, 7), "test_getSignalBySection.endTime", " invalid endTime");
    myAssert(compareDouble(signalModel->duration, 608.70713806152344, 7), "test_getSignalBySection.duration", " invalid duration");
    myAssert(compareDouble(signalModel->sampleRate, 256.016388, 7), "test_getSignalBySection.sampleRate", " invalid sampleRate");
    myAssert(signalModel->samples.size() == 155840, "test_getSignalBySection.samples.size", " invalid samples size");
    sectionIndex = 1;
    unique_ptr<SignalModel> signalModel2 = reader.getSignalSection(montageIndex, channelName, sectionIndex);
    myAssert(signalModel2->channel == "T6", "test_getSignalBySection.channel", " invalid channel");
    myAssert(compareDouble(signalModel2->startTime, 2269.6047363281250, 7), "test_getSignalBySection.startTime", " invalid startTime");
    myAssert(compareDouble(signalModel2->endTime, 29974.327613830566, 7), "test_getSignalBySection.endTime", " invalid endTime");
    myAssert(compareDouble(signalModel2->duration, 27704.722877502441, 7), "test_getSignalBySection.duration", " invalid duration");
    myAssert(compareDouble(signalModel2->sampleRate, 256.016388, 7), "test_getSignalBySection.sampleRate", " invalid sampleRate");
    myAssert(signalModel2->samples.size() == 7092864, "test_getSignalBySection.samples.size", " invalid samples size");*/

    auto endTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Delta time: " << deltaTime << "ms" << std::endl;
    reader.closeFile();

}

void test_findClosestSampleSectionIndex() {
    HarmonieReader reader;
    string filename = "C:\\projects\\data\\harmonie\\soraya\\discontinuity\\01-03-0004.sts";
    reader.openFile(filename);
    auto startTime = std::chrono::high_resolution_clock::now();
    int sectionIndex;
    sectionIndex = reader.findSampleSectionIndex(0);
    myAssert(sectionIndex == 0, "test_findClosestSampleSectionIndex.startTime", " invalid section index");
    sectionIndex = reader.findSampleSectionIndex(5*60);
    myAssert(sectionIndex == 0, "test_findClosestSampleSectionIndex.startTime", " invalid section index");
    sectionIndex = reader.findSampleSectionIndex(15*60);
    myAssert(sectionIndex == -1, "test_findClosestSampleSectionIndex.startTime", " invalid section index");
    sectionIndex = reader.findSampleSectionIndex(100*60);
    myAssert(sectionIndex == 1, "test_findClosestSampleSectionIndex.startTime", " invalid section index");
    auto endTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Delta time: " << deltaTime << "ms" << std::endl;
    reader.closeFile();
}

void test_findSampleIndexFromSectionByStartTime() {
    HarmonieReader reader;
    string filename = "C:\\projects\\data\\harmonie\\soraya\\discontinuity\\01-03-0004.sts";
    reader.openFile(filename);
    auto startTime = std::chrono::high_resolution_clock::now();
    int sampleIndex;
    sampleIndex = reader.findSampleIndexFromSectionByStartTime(0,0);
    myAssert(sampleIndex == 0, "test_findSampleIndexFromSectionByStartTime.sampleIndex", " invalid sample index");
    sampleIndex = reader.findSampleIndexFromSectionByStartTime(0,5*60);
    myAssert(sampleIndex == 0, "test_findSampleIndexFromSectionByStartTime.sampleIndex", " invalid sample index");
    auto endTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "Delta time: " << deltaTime << "ms" << std::endl;
    reader.closeFile();
}

void test_computeAge() {
    HarmonieReader reader;
    double creationTime = 0;
    double birthdate = 0;
    int age = reader.computeAge(creationTime, birthdate);
    int expected_value = 0;
    stringstream ss;
    ss << " expected value:" << expected_value << " value received:" << age;
    myAssert(age == expected_value, "test_computeAge", ss.str());

    creationTime = 315532800; // Date and time (GMT): Tuesday 1 January 1980 00:00:00
    birthdate = 220924800; // Date and time (GMT): Saturday 1 January 1977 00:00:00
    age = reader.computeAge(creationTime, birthdate);
    expected_value = 3;
    ss.str(""); ss.clear();
    ss << " expected value:" << expected_value << " value received:" << age;
    myAssert(age == expected_value, "test_computeAge", ss.str());

    creationTime = 315532800; // Date and time (GMT): Tuesday 1 January 1980 00:00:00
    birthdate = -315619200; // Date and time (GMT): Friday 1 January 1960 00:00:00
    age = reader.computeAge(creationTime, birthdate);
    expected_value = 20;
    ss.str(""); ss.clear();
    ss << " expected value:" << expected_value << " value received:" << age;
    myAssert(age == expected_value, "test_computeAge", ss.str());

    creationTime = 1735689600; // Date and time (GMT): Wednesday 1 January 2025 00:00:00
    birthdate = -315619200; // Date and time (GMT): Friday 1 January 1960 00:00:00
    age = reader.computeAge(creationTime, birthdate);
    expected_value = 65;
    ss.str(""); ss.clear();
    ss << " expected value:" << expected_value << " value received:" << age;
    myAssert(age == expected_value, "test_computeAge", ss.str());

    creationTime = 1735689599; // GMT: Tuesday 31 December 2024 23:59:59
    birthdate = -315619200; // Date and time (GMT): Friday 1 January 1960 00:00:00
    age = reader.computeAge(creationTime, birthdate);
    expected_value = 64;
    ss.str(""); ss.clear();
    ss << " expected value:" << expected_value << " value received:" << age;
    myAssert(age == expected_value, "test_computeAge", ss.str());

    creationTime = -157766400; // Date and time (GMT): Friday 1 January 1965 00:00:00
    birthdate = -315619200; // Date and time (GMT): Friday 1 January 1960 00:00:00
    age = reader.computeAge(creationTime, birthdate);
    expected_value = 5;
    ss.str(""); ss.clear();
    ss << " expected value:" << expected_value << " value received:" << age;
    myAssert(age == expected_value, "test_computeAge", ss.str());
}

void test_getBulkSignals(string filename, int montageIndex, std::vector<string>  channels) {
    
    stringstream ss;
    HarmonieReader reader;
    std::cout << "Opening:" << filename << std::endl;
    reader.openFile(filename);

    auto startProcess = std::chrono::high_resolution_clock::now();
    
    int sectionIndex = 0;
    std::cout << "getSignalSection" << std::endl;
    std::vector<SignalModel> signalSectionModels = reader.getSignalSection(montageIndex, channels, sectionIndex);
    std::cout << "getSignalSection Over" << std::endl;

    for (auto const &signalModel: signalSectionModels) {
        std::cout <<  signalModel.channel << " " << signalModel.sampleRate << " " << signalModel.samples.size() << std::endl;
    }
    
    auto endProcess = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(endProcess - startProcess).count();
    std::cout << " ( " << deltaTime << "ms)" << std::endl;
    std::cout << "about to close file" << std::endl;
    reader.closeFile();
    std::cout << "closed" << std::endl;
}

int main() {
    setlocale(LC_ALL, ".UTF8");
    std::cout << "UnitTest started" << std::endl;
    std::string filename = "";
    int montageIndex = 0;
    
    std::vector<std::string> channels = {"F7-A1"};
    int delay = 10000;
    test_getBulkSignals(filename, montageIndex, channels);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    test_getBulkSignals(filename, montageIndex, channels);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    test_getBulkSignals(filename, montageIndex, channels);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    test_getBulkSignals(filename, montageIndex, channels);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    test_getBulkSignals(filename, montageIndex, channels);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    /*std::vector<std::string> channels2 = 
        {"F7-A1", "F8-A1", "T3-A1", "T4-A1",
        "T5-A1", "T6-A1", "P3-A1", "P4-A1", "Fz-A1", "Cz-A1", "ECG", "SANABDO"};
        
//        "ECG", "SANABDO"

    test_getBulkSignals(filename, montageIndex, channels2);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
      
    test_getBulkSignals(filename, montageIndex, channels2);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));*/
}