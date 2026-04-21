#ifndef HARMONIE_READER_H
#define HARMONIE_READER_H

#include <map>
#include <stdio.h>
#include <string>
#include <vector>

#include "original/HarmonieFile.h"

typedef unsigned uint;
typedef unsigned long long uint64;

namespace Harmonie {

class SignalModel {
    public:
        std::vector<double> samples;
        double startTime;
        double endTime;
        double duration;
        float sampleRate;
        std::string channel;
};

class HarmonieReader
{
public:
    struct SLEEP_STAGE {
        int sleepStage;
		double startTime, duration;
    };

    struct SUBJECT_INFO {
        std::string id;
        std::string firstname;
        std::string lastname;
        std::string sex;
        double birthDate;
        double creationDate;
        int age;
        double height;
        double weight;
        double bmi;
        double waistSize;
    };

    CHarmonieFile *m_file;
    SUBJECT_INFO m_subjectInfo;

    HarmonieReader();
    ~HarmonieReader();

    void                            closeFile();
    bool                            openFile(std::string filename);
    bool                            saveFile();

    std::vector<CPSGFile::CHANNEL>  getChannels(int montageIndex);
    std::vector<CPSGFile::EVITEM>   getEvents();
    std::vector<CPSGFile::MONTAGE>  getMontages();

    std::vector<CPSGFile::EVGROUP>  getEventGroups();
    
    
    int                             getChannelIndexByName(std::string channelName, int montageIndex);
    std::vector<SLEEP_STAGE>        getSleepStages();
    void                            setSleepStage(double startSec, double durationSec, int stage);
    bool                            addEvent(std::string name, std::string group, double startSec, 
                                        double durationSec, std::vector<std::string> channels, int montageIndex);
    
    bool                            removeEventsByGroup(std::string groupName);
    bool                            removeEventsByName(std::string eventName, std::string groupName);
    double                          getRecordingStartTime();
    std::vector<std::string>        getExtensions();
    std::vector<std::string>        getExtensionsFilters();
    int                             getGroupIndexByName(std::string groupName);
    int                             getChannelSampleRateByName(std::string channelName, int montageIndex);
    float                           getChannelTrueSampleRateByName(std::string channelName, int montageIndex);
    SUBJECT_INFO                    getSubjectInfo();
    
    int                             getSleepStageGroup();
    uint32_t                        getSignalSectionCount();
    std::vector<SignalModel>        getSignalSection(int montageIndex,
                                        std::vector<std::string> channels,
                                        int sectionIndex);
    int                             findSampleSectionIndex(double startSec);
    double                          findSampleIndexFromSectionByStartTime(int sectionIndex, double startTime);
    int                             computeAge(double creationTime, double birthdate);
    std::string                     getLastError();
private:
    std::map<std::string, std::vector<double>> getSignal(int montageIndex, std::vector<std::string> channels);
    struct tm*                      SecondsSinceEpochToDateTime(struct tm* pTm, uint64 SecondsSinceEpoch);
    void                            display_bytes(const std::string& str);
    std::string                     utf8ToLatin1(const std::string& utf8String);
    std::string                     m_lastError;
    
    std::string currentChannel;
    int currentMontageIndex;
    std::map<std::string, std::vector<double>> m_currentSignals;
};

}
#endif // HARMONIE_READER_H