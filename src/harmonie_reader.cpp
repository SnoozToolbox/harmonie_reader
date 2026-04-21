#include <algorithm>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <vector>

#include "harmonie_reader.h"
#include "PSGFileDefs.h"

using namespace Harmonie;

HarmonieReader::HarmonieReader() {
    setlocale(LC_ALL, ".UTF8");
}

HarmonieReader::~HarmonieReader() {
}

bool HarmonieReader::openFile(std::string filename) {
    this->currentChannel = "";
    this->currentMontageIndex = -1;

    FILE *f = fopen( filename.c_str(), "rb" );

    if(f == nullptr) {
        std::cout << "ERROR: " << strerror(errno) << std::endl;
        m_lastError = "ERROR openFile:Could not open file.";
        return false;
    }

 	m_file = new CHarmonieFile(f, filename.c_str(), true, false);
    
    if(!m_file->m_OK) {
        m_lastError = m_file->m_lastError;
        std::cout << "ERROR: Error while reading file content, file:" << filename << std::endl;
    }
    
    return m_file->m_OK;
}

void HarmonieReader::closeFile() {
    this->currentChannel = "";
    this->currentMontageIndex = -1;
    if (m_file != nullptr) {
        delete m_file;
        m_file = nullptr;
    }
}

bool HarmonieReader::saveFile() {
    if (m_file != nullptr) {
        return m_file->SaveFile(nullptr);
    }
    return false;
}

std::vector<CPSGFile::MONTAGE> HarmonieReader::getMontages() {
    // Set all index for future reference
	for( int i = 0; i < m_file->m_Montages.size(); i++ ) {
		m_file->m_Montages[i].Index = i;
	}
    return m_file->m_Montages;
}

std::vector<CPSGFile::CHANNEL> HarmonieReader::getChannels(int montageIndex) {
    std::vector<CPSGFile::CHANNEL> channels;

    if (montageIndex < m_file->m_Montages.size()) {
        for(const auto& ch: m_file->m_Montages[montageIndex].Channel) {
            CPSGFile::CHANNEL tmpCh = ch;
            double maxSampleRate = round(m_file->GetTrueSampleFrequency());
            double ratio = maxSampleRate / ch.SampleFrequency;
            tmpCh.TrueSampleFrequency = m_file->GetTrueSampleFrequency() / ratio;

            channels.push_back(tmpCh);
        }
    }

    return channels;
}

std::vector<CPSGFile::EVITEM> HarmonieReader::getEvents() {
    std::vector<CPSGFile::EVITEM> events;
    events = m_file->m_EventItems;
    for (int i=0; i<events.size(); i++) {
        // Put the 0 of the time just after the calibration when the recording actually starts
        events[i].StartTime = events[i].StartTime - this->getRecordingStartTime();
        events[i].GroupName = m_file->m_EventGroups[events[i].Group].Name;

        // Put the 0 of the samples just after the calibration when the recording actually starts
        uint32_t sampleSectionIndex = m_file->m_SampleSectionEvents[0];
        uint32_t recordingOffset = m_file->m_EventItems[sampleSectionIndex].StartSample;
        events[i].StartSample = events[i].StartSample - recordingOffset;
        events[i].EndSample = events[i].EndSample - recordingOffset;
    }

    return events;
}

std::vector<CPSGFile::EVGROUP> HarmonieReader::getEventGroups() {
    return m_file->m_EventGroups;
}

int HarmonieReader::getSleepStageGroup() {
    for (int i=0; i < m_file->m_EventGroups.size(); i++){
        if (m_file->m_EventGroups[i].SleepStageGroup) {
            return i;
        }
    }

    return -1;
}

std::vector<HarmonieReader::SLEEP_STAGE> HarmonieReader::getSleepStages() {
    std::vector<HarmonieReader::SLEEP_STAGE> stages;
    int stageGroupId = this->getSleepStageGroup();

    if (stageGroupId != -1) {
        for (int i=0; i<m_file->m_EventItems.size(); i++) {
            if (m_file->m_EventItems[i].Group == stageGroupId) {
                HarmonieReader::SLEEP_STAGE sleepStage;
                sleepStage.sleepStage = (int)m_file->m_EventItems[i].SleepStage;
                sleepStage.startTime = m_file->m_EventItems[i].StartTime - this->getRecordingStartTime();
                sleepStage.duration = m_file->m_EventItems[i].TimeLength;
                stages.push_back(sleepStage);
            }
        }
    }

    return stages;
}

std::map<std::string, std::vector<double>> HarmonieReader::getSignal(int montageIndex, std::vector<std::string> channels) {
    std::map<std::string, int> channelIndexes;
    std::map<std::string, std::vector<double>> signals;
    bool result = m_file->InitDataReading(montageIndex);

    if (!result) {
        return signals;
    }

    // Get channel index for all channels
    for (const auto& channelName : channels) {
        int channelIndex = this->getChannelIndexByName(channelName, montageIndex);
        channelIndexes[channelName] = channelIndex;
    }
    
    //unsigned int nSamplesPerRecord = m_file->GetRecordSampleCountForChannel(channelIndex);
    unsigned int nRecords = m_file->GetRecordCount();

    //unsigned long nSamples = nSamplesPerRecord * nRecords;

    for(unsigned int i=0; i < nRecords; i++) {
        m_file->ReadRecord();

        for (const auto& pair : channelIndexes) {
            std::string channelName = pair.first;
            int channelIndex = pair.second;

            if (channelIndex == -1) {
                signals[channelName] = std::vector<double>();
                continue;
            }
            
            unsigned int nSamplesPerRecord = m_file->GetRecordSampleCountForChannel(channelIndex);
            for(unsigned int j=0; j < nSamplesPerRecord; j++) {
                signals[channelName].push_back(m_file->GetDataUnits(channelIndex, j));
            }
        }
    }
    m_file->EndDataReading();
    return signals;
}

int HarmonieReader::getChannelIndexByName(std::string channelName, int montageIndex) {
    std::vector<CPSGFile::CHANNEL> channels = this->getChannels(montageIndex);

    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].ChannelName == channelName)
            return i;
    }
    return -1;
}

int HarmonieReader::getChannelSampleRateByName(std::string channelName, int montageIndex) {
    std::vector<CPSGFile::CHANNEL> channels = this->getChannels(montageIndex);

    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].ChannelName == channelName)
            return channels[i].SampleFrequency;
    }
    return -1;
}

float HarmonieReader::getChannelTrueSampleRateByName(std::string channelName, int montageIndex) {
    std::vector<CPSGFile::CHANNEL> channels = this->getChannels(montageIndex);

    for (int i = 0; i < channels.size(); i++) {
        if (channels[i].ChannelName == channelName)
            return channels[i].TrueSampleFrequency;
    }
    return -1;
}

bool HarmonieReader::addEvent(std::string name, std::string group, double startSec, 
    double durationSec, std::vector<std::string> channels, int montageIndex) {
    
    std::string eventNameLatin1;
    std::string groupLatin1;

    try {
        eventNameLatin1 = utf8ToLatin1(name);
        groupLatin1 = utf8ToLatin1(group);
    } catch (std::invalid_argument& e) {
        m_lastError = "ERROR addEvent:Invalid string encoding for event name or group value.";
        return false;
    }

    if (channels.empty()) {
        m_lastError = "ERROR addEvent:Channels can't be empty.";
        return false;
    }

    if (m_file->m_Montages.empty()) {
        m_lastError = "ERROR addEvent:no montage found.";
        return false;
    }

    if (groupLatin1 == "Stade") {
        m_lastError = "ERROR addEvent:Cannot add event of group Stade";
        std::cout << "ERROR addEvent:Cannot add event of group Stade." << std::endl;
        return false;
    }

    int sectionIndex = findSampleSectionIndex(startSec);
    if (sectionIndex == -1) {
        std::cout << "ERROR addEvent: Could not find section at startTime:" << startSec << std::endl;
        m_lastError = "ERROR addEvent:Could not find section at startTime";
        return false;
    }
    // Retrieve the event that represent the sample section
    int sampleSectionIndex = m_file->m_SampleSectionEvents[sectionIndex];
    Harmonie::CPSGFile::EVITEM* currentSection = &m_file->m_EventItems[sampleSectionIndex];

    double sectionStartTime = currentSection->StartTime - this->getRecordingStartTime();
    double sectionEndTime = currentSection->EndTime - this->getRecordingStartTime();
    
    // Trunc the signal to fit within the section
    if (startSec + durationSec > sectionEndTime) {
        durationSec = sectionEndTime - startSec;
    }
    
    // Get the relative time values and samples based on the section the event is in.
    double relativeStartTime = startSec - sectionStartTime;
    int relativeStartSample = round(relativeStartTime * m_file->GetTrueSampleFrequency());
    int startSample = currentSection->StartSample + relativeStartSample;
    int nSamples = durationSec * m_file->GetTrueSampleFrequency();
    
    // channel is received in parameter. It will only use the first one of the list.
    std::string channel = channels[0];
    
    // Start edition
    m_file->BeginGroupsEventEditing(true);

    // Find the group index from the group name
    int group_index = this->getGroupIndexByName(groupLatin1);
    if (group_index == -1) {
        // if the group isn't found, create a new one.
        group_index = m_file->AddEventGroup(groupLatin1.c_str(), "");
    }

    // Convert to recording time
    startSec = this->getRecordingStartTime() + startSec;

    // Add the event
    m_file->AddEventItem( group_index, eventNameLatin1.c_str(), "", 
        startSample, nSamples, startSec, durationSec, channel.c_str() );
    m_file->BeginGroupsEventEditing(false);

    return true;
}

/**
 * @brief Find the start sample index of a given section from the provided start time
 * @param[in] sectionIndex Index of the section for which the start sample needs to be found
 * @param[in] startTime Start time from which the start sample needs to be calculated
 * @return start sample index
 */
double HarmonieReader::findSampleIndexFromSectionByStartTime(int sectionIndex, double startTime) {
    // Calculate the time difference between the provided start time and the start time of the specified section
    double deltaTime = startTime - (m_file->m_EventItems[sectionIndex].StartTime - getRecordingStartTime());
    // Get the recording offset of the specified section
    uint32_t recordingOffset = m_file->m_EventItems[sectionIndex].StartSample;
    // Calculate the start sample by adding the recording offset to the result of rounding the product of delta time and the true sample frequency
    double startSample = recordingOffset + round(deltaTime * m_file->GetTrueSampleFrequency());
    // return the calculated start sample
    return startSample;
}

/**
 * @brief Find the index of the sample section that contains the provided start time
 * @param[in] startSec Start time for which the sample section index needs to be found
 * @return Index of the sample section containing the provided start time, -1 if not found
 */
int HarmonieReader::findSampleSectionIndex(double startSec) {
    // Get the count of signal sections
    int sectionCount = getSignalSectionCount();

    // Iterate through all the sample sections
    for (int i = 0; i < sectionCount; i++) {
        // Get the index of the sample section event
        int sampleSectionIndex = m_file->m_SampleSectionEvents[i];
        // Get the start and end time of the sample section
        double sectionStartTime = m_file->m_EventItems[sampleSectionIndex].StartTime - getRecordingStartTime();
        double sectionEndTime = m_file->m_EventItems[sampleSectionIndex].EndTime - getRecordingStartTime();

        // Check if the provided start time falls within the start and end time of the sample section
        if (startSec >= sectionStartTime && startSec <= sectionEndTime)
            return i;
    }
    // If the sample section is not found, return -1
    return -1;
}


double HarmonieReader::getRecordingStartTime() {
    if (m_file->m_SampleSectionEvents.size() == 0) {
        return 0;
    }
    return m_file->m_EventItems[m_file->m_SampleSectionEvents[0]].StartTime;
}

std::vector<std::string> HarmonieReader::getExtensionsFilters() {
    std::vector<std::string> extensions;
    extensions.push_back("Harmonie (*.sts)");
    return extensions;
}

std::vector<std::string> HarmonieReader::getExtensions() {
    std::vector<std::string> extensions;
    extensions.push_back("sts");
    return extensions;
}

int HarmonieReader::getGroupIndexByName(std::string groupName) {
    int i;

    for( i = 0; i < m_file->m_EventGroups.size(); i++ ) {	
        if (m_file->m_EventGroups[i].Name == groupName) {
            break;
        }
	}
    if (i != m_file->m_EventGroups.size()) 
        return i;
    else 
        return -1;
}

bool  HarmonieReader::removeEventsByGroup(std::string groupName) {
    std::string groupLatin1;
    try {
        groupLatin1 = utf8ToLatin1(groupName);
    } catch (std::invalid_argument& e) {
        m_lastError = "ERROR removeEventsByGroup:Invalid string encoding for group name value.";
        return false;
    }

    int groupIndex = this->getGroupIndexByName(groupLatin1);
    
    if(groupIndex != -1) {
        m_file->BeginGroupsEventEditing(true);
        m_file->DeleteEventGroup(groupIndex);
        m_file->BeginGroupsEventEditing(false);
        return true;
    } else {
        m_lastError = "ERROR removeEventsByGroup:Could not find event group:" + groupName;
        return false;
    }
    
    
}

bool HarmonieReader::removeEventsByName(std::string eventName, std::string groupName) {
    std::string nameLatin1;
    std::string groupLatin1;
    try {
        nameLatin1 = utf8ToLatin1(eventName);
        groupLatin1 = utf8ToLatin1(groupName);
    } catch (std::invalid_argument& e) {
        m_lastError = "ERROR removeEventsByName:Invalid string encoding for event name of group name value.";
        return false;
    }

    int groupIndex = this->getGroupIndexByName(groupLatin1);
    
    if(groupIndex != -1) {
        m_file->BeginGroupsEventEditing(true);
        m_file->DeleteEventsByName(nameLatin1, groupIndex);
        m_file->BeginGroupsEventEditing(false);
        return true;
    } else {
        m_lastError = "ERROR removeEventsByName:Could not find event group:" + groupName;
        return false;
    }
}

void HarmonieReader::setSleepStage(double startSec, double durationSec, int stage) {
    std::vector<HarmonieReader::SLEEP_STAGE> stages;
    int stageGroupId = this->getSleepStageGroup();
    if (stageGroupId != -1) {
        for (int i=0; i<m_file->m_EventItems.size(); i++) {
            if (m_file->m_EventItems[i].Group == stageGroupId) {
                
                double stageStartTime = this->getRecordingStartTime() + m_file->m_EventItems[i].StartTime;
                
                if (abs(startSec - stageStartTime) < 1) {
                    m_file->m_EventItems[i].SleepStage = static_cast<Harmonie::CPSGFile::EPSGSleepStage>(stage);
                }
            }
        }
    }
}

HarmonieReader::SUBJECT_INFO HarmonieReader::getSubjectInfo() {
    m_subjectInfo.id =              ((CPSGFile*)m_file)->m_PatientInfo.Id;
    m_subjectInfo.firstname =       ((CPSGFile*)m_file)->m_PatientInfo.FirstName;
    m_subjectInfo.lastname =       ((CPSGFile*)m_file)->m_PatientInfo.LastName;
    int gender = ((CPSGFile*)m_file)->m_PatientInfo.Gender;
    switch(gender) {
        case 0:
            m_subjectInfo.sex = 'X';
        case 1:
            m_subjectInfo.sex = 'F';
        case 2:
            m_subjectInfo.sex = 'M';
        default:
            m_subjectInfo.sex = 'X';
    }
    m_subjectInfo.birthDate =       ((CPSGFile*)m_file)->m_PatientInfo.BirthDate;
    m_subjectInfo.creationDate =    this->getRecordingStartTime();
    int age = this->computeAge(((CPSGFile*)m_file)->m_FileInfo.CreationTime, ((CPSGFile*)m_file)->m_PatientInfo.BirthDate);
    m_subjectInfo.age =         age;
    m_subjectInfo.height =      atof(((CPSGFile*)m_file)->m_PatientInfo.Height.c_str());
    m_subjectInfo.weight =      atof(((CPSGFile*)m_file)->m_PatientInfo.Weight.c_str());
    m_subjectInfo.bmi =         m_subjectInfo.weight / (m_subjectInfo.height*m_subjectInfo.height);
    m_subjectInfo.waistSize =   0; // TODO
    return m_subjectInfo;   
}

/**
 * @brief Retrieves the signal section for the specified montage index, channel name, and section index.
 *
 * @param montageIndex The index of the montage to retrieve the signal section from.
 * @param channelName The name of the channel to retrieve the signal section from.
 * @param sectionIndex The index of the section to retrieve from the signal.
 *
 * @return A unique pointer to a SignalModel object containing the retrieved signal section.
 */
std::vector<SignalModel> HarmonieReader::getSignalSection(
    int montageIndex, 
    std::vector<std::string> channels, 
    int sectionIndex) {

    // Read the full signal if it was not loaded before
    /*if  (this->currentMontageIndex == -1 || 
        this->currentMontageIndex != montageIndex || 
        this->currentChannel != channelName) {

        this->currentChannel = channelName;
        this->currentMontageIndex = montageIndex;
        this->m_currentSignal = this->getSignal(montageIndex, channelName);
    }*/
    
    this->m_currentSignals = this->getSignal(montageIndex, channels);

    std::vector<SignalModel> signalModels;

    for(const auto& pair: this->m_currentSignals) {
        SignalModel signalModel;

        std::string channelName = pair.first;
        signalModel.sampleRate = getChannelTrueSampleRateByName(channelName, montageIndex);
        signalModel.channel = channelName;

        uint32_t sampleSectionIndex = m_file->m_SampleSectionEvents[sectionIndex];
        signalModel.startTime = m_file->m_EventItems[sampleSectionIndex].StartTime - getRecordingStartTime();
        signalModel.endTime = m_file->m_EventItems[sampleSectionIndex].EndTime - getRecordingStartTime();
        signalModel.duration = signalModel.endTime - signalModel.startTime;

        double recordingSampleRate = m_file->GetTrueSampleFrequency();
        double channelSampleRate = signalModel.sampleRate;
        double sampleRateRatio = recordingSampleRate / channelSampleRate;

        uint32_t startSample = (int)round((m_file->m_EventItems[sampleSectionIndex].StartSample / sampleRateRatio));
        uint32_t endSample = (int)round((m_file->m_EventItems[sampleSectionIndex].EndSample / sampleRateRatio));
    
        if (pair.second.size() == 0) {
            // If the channel isn't found, the signal size is 0. If that's the case we just set
            // the samples to the empty vector.
            signalModel.startTime = 0;
            signalModel.endTime = 0;
            signalModel.duration = 0;
            signalModel.sampleRate = 0;
            signalModel.samples = pair.second;
            std::cout << "Channel not found:" << channelName << std::endl;
        } else {
            signalModel.samples = {pair.second.begin() + startSample, pair.second.begin() + endSample + 1};
        }
        
        signalModels.push_back(signalModel);
    }

    return signalModels;
}

/**
 * @brief Returns the number of signal sections in the HarmonieReader file.
 * @return The number of signal sections in the HarmonieReader file.
 */
uint32_t HarmonieReader::getSignalSectionCount() {
    return m_file->m_SampleSectionEvents.size();
}

int HarmonieReader::computeAge(double creationTime, double birthdate) {
    struct tm ct_tm;
    struct tm bt_tm;

    SecondsSinceEpochToDateTime(&ct_tm, (uint64)creationTime);
    SecondsSinceEpochToDateTime(&bt_tm, (uint64)birthdate);

    int age = ct_tm.tm_year - bt_tm.tm_year;

    if (ct_tm.tm_mon < bt_tm.tm_mon)
        age--;
    else if(ct_tm.tm_mon == bt_tm.tm_mon &&
        ct_tm.tm_mday < bt_tm.tm_mday)
        age--;

    return age;
}


struct tm* HarmonieReader::SecondsSinceEpochToDateTime(struct tm* pTm, uint64 SecondsSinceEpoch)
{
  uint64 sec;
  uint quadricentennials, centennials, quadrennials, annuals/*1-ennial?*/;
  uint year, leap;
  uint yday, hour, min;
  uint month, mday, wday;
  static const uint daysSinceJan1st[2][13]=
  {
    {0,31,59,90,120,151,181,212,243,273,304,334,365}, // 365 days, non-leap
    {0,31,60,91,121,152,182,213,244,274,305,335,366}  // 366 days, leap
  };
/*
  400 years:

  1st hundred, starting immediately after a leap year that's a multiple of 400:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n n

  2nd hundred:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n n

  3rd hundred:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n n

  4th hundred:
  n n n l  \
  n n n l   } 24 times
  ...      /
  n n n l /
  n n n L <- 97'th leap year every 400 years
*/

  // Re-bias from 1970 to 1601:
  // 1970 - 1601 = 369 = 3*100 + 17*4 + 1 years (incl. 89 leap days) =
  // (3*100*(365+24/100) + 17*4*(365+1/4) + 1*365)*24*3600 seconds
  sec = SecondsSinceEpoch + 11644473600LL;

  wday = (uint)((sec / 86400 + 1) % 7); // day of week

  // Remove multiples of 400 years (incl. 97 leap days)
  quadricentennials = (uint)(sec / 12622780800ULL); // 400*365.2425*24*3600
  sec %= 12622780800ULL;

  // Remove multiples of 100 years (incl. 24 leap days), can't be more than 3
  // (because multiples of 4*100=400 years (incl. leap days) have been removed)
  centennials = (uint)(sec / 3155673600ULL); // 100*(365+24/100)*24*3600
  if (centennials > 3)
  {
    centennials = 3;
  }
  sec -= centennials * 3155673600ULL;

  // Remove multiples of 4 years (incl. 1 leap day), can't be more than 24
  // (because multiples of 25*4=100 years (incl. leap days) have been removed)
  quadrennials = (uint)(sec / 126230400); // 4*(365+1/4)*24*3600
  if (quadrennials > 24)
  {
    quadrennials = 24;
  }
  sec -= quadrennials * 126230400ULL;

  // Remove multiples of years (incl. 0 leap days), can't be more than 3
  // (because multiples of 4 years (incl. leap days) have been removed)
  annuals = (uint)(sec / 31536000); // 365*24*3600
  if (annuals > 3)
  {
    annuals = 3;
  }
  sec -= annuals * 31536000ULL;

  // Calculate the year and find out if it's leap
  year = 1601 + quadricentennials * 400 + centennials * 100 + quadrennials * 4 + annuals;
  leap = !(year % 4) && (year % 100 || !(year % 400));

  // Calculate the day of the year and the time
  yday = sec / 86400;
  sec %= 86400;
  hour = sec / 3600;
  sec %= 3600;
  min = sec / 60;
  sec %= 60;

  // Calculate the month
  for (mday = month = 1; month < 13; month++)
  {
    if (yday < daysSinceJan1st[leap][month])
    {
      mday += yday - daysSinceJan1st[leap][month - 1];
      break;
    }
  }

  // Fill in C's "struct tm"
  memset(pTm, 0, sizeof(*pTm));
  pTm->tm_sec = sec;          // [0,59]
  pTm->tm_min = min;          // [0,59]
  pTm->tm_hour = hour;        // [0,23]
  pTm->tm_mday = mday;        // [1,31]  (day of month)
  pTm->tm_mon = month - 1;    // [0,11]  (month)
  pTm->tm_year = year - 1900; // 70+     (year since 1900)
  pTm->tm_wday = wday;        // [0,6]   (day since Sunday AKA day of week)
  pTm->tm_yday = yday;        // [0,365] (day since January 1st AKA day of year)
  pTm->tm_isdst = -1;         // daylight saving time flag

  return pTm;
}


void HarmonieReader::display_bytes(const std::string& str) {
    for (unsigned char byte : str) {
        // Print each byte as a two-digit hexadecimal value
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ' ';
    }
    std::cout << std::dec << std::endl; // Restore decimal output
}

std::string HarmonieReader::utf8ToLatin1(const std::string& utf8String) {
    std::string latin1String;

    for (size_t i = 0; i < utf8String.size(); ++i) {
        char utf8Char = utf8String[i];

        // Check if the character is part of a multi-byte sequence
        if ((utf8Char & 0xC0) == 0x80) {
            // Skip continuation bytes in multi-byte characters
            continue;
        }

        // Convert UTF-8 character to Unicode code point
        unsigned int codePoint = static_cast<unsigned char>(utf8Char);
        if ((utf8Char & 0xE0) == 0xC0) {
            codePoint = ((codePoint & 0x1F) << 6) | (static_cast<unsigned char>(utf8String[++i]) & 0x3F);
        }
        else if ((utf8Char & 0xF0) == 0xE0) {
            i = i + 2;
        }
        else if ((utf8Char & 0xF1) == 0xF0) {
            i = i + 3;
        }
        // Convert Unicode code point to Latin-1
        if (codePoint < 127 || (codePoint >=161 && codePoint <= 255)) {
            latin1String.push_back(static_cast<char>(codePoint));
        }
        else {
            throw std::invalid_argument("String is not a valid latin1 string");
        }
    }

    return latin1String;
}

std::string HarmonieReader::getLastError() {
    return m_lastError;
}