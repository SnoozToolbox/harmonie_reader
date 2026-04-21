cmake_minimum_required(VERSION 3.19)

project(harmonie_readers)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(SOURCE_FILES 
    src/harmonie_reader.cpp 
    src/harmonie_reader.h
    src/original/HarmonieFile.cpp
    src/original/HarmonieFile.h
    src/original/MathExpression.cpp
    src/original/MathExpression.h
    src/original/StdString.cpp
    src/original/StdString.h
    src/original/PSGFile.cpp
    src/original/PSGFile.h
    src/original/PSGFileDefs.h)

# Test pybind11
add_subdirectory(extern/pybind11)
pybind11_add_module(HarmonieReader 
    src/harmonie_reader_wrapper.cpp 
    ${SOURCE_FILES})

if (MSVC)
    add_definitions(-DCEAMS_WINDOWS)
endif (MSVC)

if (APPLE)
    add_definitions(-DCEAMS_APPLE)
endif (APPLE)
    
# EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
# define (VERSION_INFO) here.
target_compile_definitions(HarmonieReader PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})

include_directories(
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/original
)

add_executable(test_read_channels 
    test/cpp/test_read_channels.cpp
    ${SOURCE_FILES}
)

add_executable(test_read_events 
    test/cpp/test_read_events.cpp
    ${SOURCE_FILES}
)

add_executable(test_read_sleep_stages 
    test/cpp/test_read_sleep_stages.cpp
    ${SOURCE_FILES}
)

add_executable(test_read_signal 
    test/cpp/test_read_signal.cpp
    ${SOURCE_FILES}
)

add_executable(test_write_sleep_stages 
    test/cpp/test_write_sleep_stages.cpp
    ${SOURCE_FILES}
)

add_executable(test_write_events 
    test/cpp/test_write_events.cpp
    ${SOURCE_FILES}
)

add_executable(test_write_events_discontinuity
    test/cpp/test_write_events_discontinuity.cpp
    ${SOURCE_FILES}
)

add_executable(test_delete_event_group 
    test/cpp/test_delete_event_group.cpp
    ${SOURCE_FILES}
)

add_executable(test_delete_events 
    test/cpp/test_delete_events.cpp
    ${SOURCE_FILES}
)

add_executable(test_read_subject_info 
    test/cpp/test_read_subject_info.cpp
    ${SOURCE_FILES}
)

add_executable(fix_events_offset 
    test/cpp/fix_events_offset.cpp
    ${SOURCE_FILES}
)

add_executable(misc_test 
    test/cpp/misc_test.cpp
    ${SOURCE_FILES}
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
