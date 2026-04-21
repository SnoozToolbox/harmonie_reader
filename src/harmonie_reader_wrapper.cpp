#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "harmonie_reader.h"

namespace py = pybind11;
using namespace Harmonie;

PYBIND11_MODULE(HarmonieReader, m) {
    py::class_<SignalModel>(m, "SignalModel")
        .def(py::init<>())
        .def_readwrite("samples", &SignalModel::samples)
        .def_readwrite("start_time", &SignalModel::startTime)
        .def_readwrite("end_time", &SignalModel::endTime)
        .def_readwrite("duration", &SignalModel::duration)
        .def_readwrite("sample_rate", &SignalModel::sampleRate)
        .def_readwrite("channel", &SignalModel::channel);
        
    py::class_<CPSGFile::EVITEM>(m, "CPSGFile::EVITEM")
        .def(py::init<>())
        .def_property("name",
            [](const CPSGFile::EVITEM &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.Name.data(), self.Name.length(),NULL));
                },
            [](CPSGFile::EVITEM &self, const char *new_a) { self.Name = strdup(new_a); }
        )
        .def_readwrite("channels", &CPSGFile::EVITEM::Channels)
        .def_readwrite("start_time", &CPSGFile::EVITEM::StartTime)
        .def_readwrite("end_time", &CPSGFile::EVITEM::EndTime)
        .def_readwrite("duration", &CPSGFile::EVITEM::TimeLength)
        .def_readwrite("start_sample", &CPSGFile::EVITEM::StartSample)
        .def_readwrite("end_sample", &CPSGFile::EVITEM::EndSample)
        .def_readwrite("sample_length", &CPSGFile::EVITEM::SampleLength)
        .def_property("group",
            [](const CPSGFile::EVITEM &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.GroupName.data(), self.GroupName.length(),NULL));
                },
            [](CPSGFile::EVITEM &self, const char *new_a) { self.GroupName = strdup(new_a); }
        );

    py::class_<CPSGFile::EVGROUP>(m, "CPSGFile::EVGROUP")
        .def(py::init<>())
        .def_property("name",
            [](const CPSGFile::EVGROUP &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.Name.data(), self.Name.length(),NULL));
                },
            [](CPSGFile::EVGROUP &self, const char *new_a) { self.Name = strdup(new_a); }
        )
        .def_property("description",
            [](const CPSGFile::EVGROUP &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.Description.data(), self.Description.length(),NULL));
                },
            [](CPSGFile::EVGROUP &self, const char *new_a) { self.Description = strdup(new_a); }
        )
        .def_readwrite("is_deletable", &CPSGFile::EVGROUP::IsDeletable)
        .def_readwrite("sleep_stage_group", &CPSGFile::EVGROUP::SleepStageGroup)
        .def_readwrite("sample_section_group", &CPSGFile::EVGROUP::SampleSectionGroup);

    py::class_<CPSGFile::CHANNEL>(m, "CPSGFile::CHANNEL")
        .def(py::init<>())
        .def_readwrite("name", &CPSGFile::CHANNEL::ChannelName)
        .def_readwrite("sample_rate", &CPSGFile::CHANNEL::TrueSampleFrequency)
        .def_readwrite("unit", &CPSGFile::CHANNEL::Unit);

    py::class_<CPSGFile::MONTAGE>(m, "CPSGFile::MONTAGE")
        .def(py::init<>())
        .def_readwrite("index", &CPSGFile::MONTAGE::Index)
        .def_property("name",
            [](const CPSGFile::MONTAGE &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.MontageName.data(), self.MontageName.length(),NULL));
                },
            [](CPSGFile::MONTAGE &self, const char *new_a) { self.MontageName = strdup(new_a); }
        );

    py::class_<HarmonieReader::SLEEP_STAGE>(m, "HarmonieReader::SLEEP_STAGE")
        .def(py::init<>())
        .def_readwrite("sleep_stage", &HarmonieReader::SLEEP_STAGE::sleepStage)
        .def_readwrite("start_time", &HarmonieReader::SLEEP_STAGE::startTime)
        .def_readwrite("duration", &HarmonieReader::SLEEP_STAGE::duration);

    py::class_<HarmonieReader::SUBJECT_INFO>(m, "HarmonieReader::SUBJECT_INFO")
        .def(py::init<>())
        .def_property("id",
            [](const HarmonieReader::SUBJECT_INFO &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.id.data(), self.id.length(),NULL));
                },
            [](HarmonieReader::SUBJECT_INFO &self, const char *new_a) { self.id = strdup(new_a); }
        )
        .def_property("firstname",
            [](const HarmonieReader::SUBJECT_INFO &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.firstname.data(), self.firstname.length(),NULL));
                },
            [](HarmonieReader::SUBJECT_INFO &self, const char *new_a) { self.firstname = strdup(new_a); }
        )
        .def_property("lastname",
            [](const HarmonieReader::SUBJECT_INFO &self) { 
                return py::str(PyUnicode_DecodeLatin1(self.lastname.data(), self.lastname.length(),NULL));
                },
            [](HarmonieReader::SUBJECT_INFO &self, const char *new_a) { self.lastname = strdup(new_a); }
        )
        .def_readwrite("sex", &HarmonieReader::SUBJECT_INFO::sex)
        .def_readwrite("birth_date", &HarmonieReader::SUBJECT_INFO::birthDate)
        .def_readwrite("creation_date", &HarmonieReader::SUBJECT_INFO::creationDate)
        .def_readwrite("age", &HarmonieReader::SUBJECT_INFO::age)
        .def_readwrite("height", &HarmonieReader::SUBJECT_INFO::height)
        .def_readwrite("weight", &HarmonieReader::SUBJECT_INFO::weight)
        .def_readwrite("bmi", &HarmonieReader::SUBJECT_INFO::bmi)
        .def_readwrite("waist_size", &HarmonieReader::SUBJECT_INFO::waistSize);

    py::class_<HarmonieReader>(m, "HarmonieReader")
        .def(py::init<>())
        .def("open_file", &HarmonieReader::openFile)
        .def("close_file", &HarmonieReader::closeFile)
        .def("save_file", &HarmonieReader::saveFile)
        .def("get_montages", &HarmonieReader::getMontages)
        .def("get_channels", &HarmonieReader::getChannels)
        .def("get_signal_section_count", &HarmonieReader::getSignalSectionCount)
        .def("get_signal_section", &HarmonieReader::getSignalSection)
        .def("get_event_groups", &HarmonieReader::getEventGroups)
        .def("get_events", &HarmonieReader::getEvents)
        .def("add_event", &HarmonieReader::addEvent)
        .def("get_extensions", &HarmonieReader::getExtensions)
        .def("get_extensions_filters", &HarmonieReader::getExtensionsFilters)
        .def("get_recording_start_time", &HarmonieReader::getRecordingStartTime)
        .def("remove_events_by_group", &HarmonieReader::removeEventsByGroup)
        .def("remove_events_by_name", &HarmonieReader::removeEventsByName)
        .def("get_sleep_stages", &HarmonieReader::getSleepStages)
        .def("set_sleep_stage", &HarmonieReader::setSleepStage)
        .def("get_subject_info", &HarmonieReader::getSubjectInfo)
        .def("get_last_error", &HarmonieReader::getLastError);

}
