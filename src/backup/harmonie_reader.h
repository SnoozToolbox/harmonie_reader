#ifndef HARMONIE_READER_H
#define HARMONIE_READER_H

#include <stdio.h>
#include <string>

class HarmonieReader
{

private:
    FILE *m_file;
    
    struct FILEINFO
	{
        std::string filename;
		//EPSGFileType FileType; TODO
        int fileType;
		int version;
		std::string description, institution;
		double creationTime;
		std::string createdBy, lastModifiedBy;
	} m_fileInfo;

    struct FILEHEADER {
        uint32_t separator;
        std::string className;
        int16_t version;
        int16_t fileType;
    } m_fileHeader;

public:
    HarmonieReader();
    ~HarmonieReader();

    bool                openFile(const std::string &filename);
    void                closeFile();
    bool                readFileHeader();

private:
    std::string         readText();
    std::string         readClassName();
};

#endif // HEADER_H