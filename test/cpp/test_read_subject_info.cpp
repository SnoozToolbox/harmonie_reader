// Your First C++ Program

#include <iomanip>
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

    HarmonieReader::SUBJECT_INFO subjectInfo = reader.getSubjectInfo();

    std::cout << "*** Subject info ***" << std::endl;
    std::cout << "id: " << subjectInfo.id << std::endl;
    std::cout << "firstname: " << subjectInfo.firstname << std::endl;
    std::cout << "lastname: " << subjectInfo.lastname << std::endl;
    std::cout << "sex: " << subjectInfo.sex << std::endl;
    std::cout << "birthDate: " << std::setprecision (17) << subjectInfo.birthDate << std::endl;
    std::cout << "creationDate: " << std::setprecision (17) << subjectInfo.creationDate << std::endl;
    std::cout << "age: " << subjectInfo.age << std::endl;
    std::cout << "height: " << subjectInfo.height << std::endl;
    std::cout << "weight: " << subjectInfo.weight << std::endl;
    std::cout << "bmi: " << subjectInfo.bmi << std::endl;
    std::cout << "waistSize: " << subjectInfo.waistSize << std::endl;

    std::cout << "DONE" << std::endl;
    return 0;
}
