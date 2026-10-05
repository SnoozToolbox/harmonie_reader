#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>

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

int main() {
    HarmonieReader reader;
    std::cout << "Test: test_anonymize_subject_info" << std::endl;

    // -----------------------------------------------------------------------
    // CONFIG — edit these before running
    // -----------------------------------------------------------------------
    std::string filename = "";              // Path to the source .sts file
    std::string replacementId = "ANON";     // Patient ID written into the anonymized file
    bool copyBeforeAnonymize = true;        // true: keep source, write new files; false: overwrite/move
    bool renameToId = false;                // true: name output files after replacementId
    std::string outputPath = "";            // Destination folder (or full .sts path). Empty = next to original.
    bool keepSex = true;                    // true: keep gender; false: clear it
    // -----------------------------------------------------------------------

    if (filename.empty()) {
        std::cout << "ERROR: No filename specified" << std::endl;
        std::cout << "Set 'filename' at the top of main() before running." << std::endl;
        return 1;
    }

    if (!copyBeforeAnonymize) {
        std::cout << "WARNING: copyBeforeAnonymize=false will overwrite/move the source files" << std::endl;
    }

    std::cout << "Opening: " << filename << std::endl;
    if (!reader.openFile(filename)) {
        std::cout << "ERROR Failed to open the file:" << filename << std::endl;
        std::cout << reader.getLastError() << std::endl;
        return 1;
    }

    HarmonieReader::SUBJECT_INFO before = reader.getSubjectInfo();
    printSubject("BEFORE", before);

    std::cout << "Anonymizing with replacement_id=" << replacementId
              << " copy=" << copyBeforeAnonymize
              << " rename=" << renameToId
              << " out=" << (outputPath.empty() ? "<next to original>" : outputPath)
              << " ..." << std::endl;
    if (!reader.anonymizeSubjectInfo(replacementId, keepSex, copyBeforeAnonymize, renameToId,
                                     outputPath)) {
        std::cout << "ERROR anonymize failed: " << reader.getLastError() << std::endl;
        return 1;
    }

    std::cout << "Saving file..." << std::endl;
    if (!reader.saveFile()) {
        std::cout << "ERROR saveFile failed: " << reader.getLastError() << std::endl;
        return 1;
    }

    std::string anonFile = reader.getFilename();
    reader.closeFile();
    std::cout << "Anonymized file: " << anonFile << std::endl;

    if (copyBeforeAnonymize && !std::ifstream(filename.c_str())) {
        std::cout << "ERROR source file disappeared while copying was requested" << std::endl;
        return 1;
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

    std::string bak = anonFile + ".bak";
    if (std::ifstream(bak.c_str())) {
        std::cout << "ERROR backup with original values was left behind: " << bak << std::endl;
        return 1;
    }

    std::cout << "SUCCESS Subject info anonymized. Output file: " << anonFile << std::endl;
    std::cout << "DONE" << std::endl;
    return 0;
}
