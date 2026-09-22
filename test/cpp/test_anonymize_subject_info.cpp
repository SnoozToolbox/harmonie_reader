#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>
#include <vector>

#include "../src/harmonie_reader.h"

using namespace Harmonie;

static void printSubject(const char *label, const HarmonieReader::SUBJECT_INFO &info) {
    std::cout << "--- " << label << " ---" << std::endl;
    std::cout << "  id: " << info.id << std::endl;
    std::cout << "  firstname: " << info.firstname << std::endl;
    std::cout << "  lastname: " << info.lastname << std::endl;
    std::cout << "  sex: " << info.sex << std::endl;
    std::cout << "  birthDate: " << info.birthDate << std::endl;
    std::cout << "  age: " << info.age << std::endl;
    std::cout << "  height: " << info.height << std::endl;
    std::cout << "  weight: " << info.weight << std::endl;
}

static bool copyFile(const std::string &src, const std::string &dst) {
    std::ifstream in(src, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ofstream out(dst, std::ios::binary);
    if (!out) {
        return false;
    }
    out << in.rdbuf();
    return static_cast<bool>(out);
}

int main(int argc, char **argv) {
    HarmonieReader reader;
    std::cout << "Test: test_anonymize_subject_info" << std::endl;

    std::string filename = "";
    std::string replacementId = "ANON";
    // Or set a path here when debugging from the IDE:
    // filename = "C:\\path\\to\\recording.sts";

    if (argc >= 2) {
        filename = argv[1];
    }
    if (argc >= 3) {
        replacementId = argv[2];
    }

    if (filename.empty()) {
        std::cout << "ERROR: No filename specified" << std::endl;
        std::cout << "Usage: test_anonymize_subject_info <file.sts> [replacement_id]" << std::endl;
        return 1;
    }

    std::string anonFile = filename;
    size_t dot = anonFile.find_last_of('.');
    if (dot == std::string::npos) {
        anonFile += "_anon";
    } else {
        anonFile = anonFile.substr(0, dot) + "_anon" + anonFile.substr(dot);
    }

    std::cout << "Copying " << filename << " -> " << anonFile << std::endl;
    if (!copyFile(filename, anonFile)) {
        std::cout << "ERROR: Could not copy file" << std::endl;
        return 1;
    }

    std::cout << "Opening copy: " << anonFile << std::endl;
    if (!reader.openFile(anonFile)) {
        std::cout << "ERROR Failed to open the file:" << anonFile << std::endl;
        std::cout << reader.getLastError() << std::endl;
        return 1;
    }

    HarmonieReader::SUBJECT_INFO before = reader.getSubjectInfo();
    printSubject("BEFORE", before);

    std::cout << "Anonymizing with replacement_id=" << replacementId << " ..." << std::endl;
    if (!reader.anonymizeSubjectInfo(replacementId, true)) {
        std::cout << "ERROR anonymize failed: " << reader.getLastError() << std::endl;
        return 1;
    }

    std::cout << "Saving file..." << std::endl;
    if (!reader.saveFile()) {
        std::cout << "ERROR saveFile failed" << std::endl;
        return 1;
    }
    reader.closeFile();

    std::string bak = anonFile + ".bak";
    if (std::remove(bak.c_str()) == 0) {
        std::cout << "Removed backup with original PHI: " << bak << std::endl;
    }

    std::cout << "Reopening anonymized file for verification..." << std::endl;
    HarmonieReader validation;
    if (!validation.openFile(anonFile)) {
        std::cout << "ERROR Failed to reopen:" << anonFile << std::endl;
        return 1;
    }

    HarmonieReader::SUBJECT_INFO after = validation.getSubjectInfo();
    printSubject("AFTER", after);
    validation.closeFile();

    bool ok = (after.id == replacementId) &&
              (after.firstname == "ANON") &&
              (after.lastname == "ANON") &&
              (after.birthDate == 0) &&
              (after.height == 0) &&
              (after.weight == 0);

    if (!ok) {
        std::cout << "ERROR anonymization incomplete" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS Subject info anonymized. Output file: " << anonFile << std::endl;
    std::cout << "DONE" << std::endl;
    return 0;
}
