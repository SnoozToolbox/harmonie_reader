# Start Visual Studio Code using this:
- From the Windows menu, search and open the command line program called Developer Command Prompt for VS 2022
- Using the command line, navigate to the subfolder of this projects.
- Open Visual studio code from the command line using the command:
```
code .
```

# Debugging the CPP code.
There is a bug with Pybind11 that prevents it to compile the library in debug mode. Therefor the project will not be 
able to compile when the build target is set to ALL_BUILD. Thankfully compiling the library in debug is not necessary,
when you want to debug a specific feature of the lib, just target a specific CPP test as the build target. This will
bypass anything related to PyBind11 and you'll be able to step through the code.

See more info here:https://github.com/pybind/pybind11/issues/3459

# New computer installation follow these steps to install everything
## Install Microsoft Visual Studio Code
Follow this tutorial to install Visual Studio Code and the C++ compiler: 
https://code.visualstudio.com/docs/cpp/config-msvc

## Install Git for Windows
https://git-scm.com/download/win

## Install CMake
- Download and install CMAKE from this here: https://cmake.org/download/
- During the installation, select the option to add cmake the to user path.

## Installation de Python 3.10.11
- [Python Release Python 3.10.11 | Python.org](https://www.python.org/downloads/release/python-31011/)

on ubuntu you also need to run:
```
apt-get install python3-dev
```

### Update the git submodule (for pybind11)
Type this command in the VSCODE terminal
```
git submodule update --init
```

### Setup Visual Studio Code.
In Visual Studio Code, set the active kit:
- Click "No active kit" in the blue bar at the bottom
- Select the kit: "Visual Studio Build Tools 2022 Release x86_amd64"

Select the appropriate CMake build (Debug or Release) (See the note about debugging the CPP code above)
- Select the build target "ALL_BUILD"
- Press "Build"

### Copy the libs into Snooz folder
These scripts are useful to copy the latest release version of the lib into the Snooz project.
You must change the script to point to the installation of Snooz on your dev machine.
Linux:   ./update_release_lib.sh
Windows: ./update_release_lib_windows.bat
MAC : ./update_release_lib_mac.bat

### OPTIONAL
#### Create the build folder manually
This isn't always necessary but can come in handy when Visual Studio Code is acting up.
mkdir build
cd build
cmake ..
make

## Testing
### CPP tests
- CPP tests are used to test the feature of the library before wrapping it in python code. 
- Make sure you fill the filename, channel and montage index when necessary with a file you want to use the tests with. By default there isn't any file test in this repository. You need to find a Harmonie file in order to test the library. Note that reading a Harmonie file over a network works but is very slow. It is adviced to copy the file locally before testing.
- Once built, the test executable are inside the build subfolder of this project.
- Run them manually one at the time to execute the tests.

### Python tests
- Python tests are used to test the interaction of the cpp library with the python code. They are very similar than the cpp tests. Make sure you fill the filename, channel and montage index when necessary with a file you want to use the tests with. 
- Run them manually from the tests/python folders by using a python command, ex: python ./test/python/test_read_events.py
- Important!!! Python tests are using the release version of the library and not the debug version. When you make changes, make sure you are also compiling in release before using the python tests.
- 
## Data format
## Events
The STS format:
- StartSample starts at 0 at the very beginning of the file before the calibration signals.
- StartTime is based on the number of seconds from January 1 of the year 0 at 00:00:00AM in local time.

Our format coming out of this library:
- Basically, the calibration is removed from the sample position when the library sends the list of events.
- start_sample's zero start at the begining of the recording after the calibration period.
- start_time's zero start at the begining of the recording after the calibration period.

## Compiling library
In the lower bar of the cmake tool extension
Select the target build : ALL BUILD
Configure the cmake : from debug to release
Press on Build

