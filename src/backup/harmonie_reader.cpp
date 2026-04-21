#include <algorithm>
#include <iostream>
#include <vector>

#include "harmonie_reader.h"

HarmonieReader::HarmonieReader() {
    m_file = nullptr;
}

HarmonieReader::~HarmonieReader() {
    
}

bool HarmonieReader::openFile(const std::string &filename) {
    m_file = fopen( filename.c_str(), "rb" );
	if( m_file == nullptr ) {
        return false;
	}

    return true;
}

bool HarmonieReader::readFileHeader() {
    // Read file version
    int16_t version;
	fread( &version, sizeof( version ), 1, m_file );
    m_fileInfo.version = version;

    // Read separator
    fread( &m_fileHeader.separator, sizeof( m_fileHeader.separator ), 1, m_file );

    std::cout << "File version:" << version << "\n";
    std::cout << "File separator:" << m_fileHeader.separator << "\n";

    // Validate class name
    m_fileHeader.className = readClassName();
    std::cout << "File className:" << m_fileHeader.className << "\n";

    std::transform( m_fileHeader.className.begin(), 
        m_fileHeader.className.end(), 
        m_fileHeader.className.begin(), ::tolower );
    
    if (m_fileHeader.className != "cfileheader") {
        std::cout << "ERROR wrong class name:" << m_fileHeader.className << "\n";
        return false;
    }

    // Read common informations shared with all versions of the file
    fread(&m_fileHeader.version, sizeof( m_fileHeader.version ), 1, m_file);

    m_fileInfo.description = readText();
	m_fileInfo.institution = readText();
    std::cout << "File description:" << m_fileInfo.description << "\n";
    std::cout << "File institution:" << m_fileInfo.institution << "\n";
	fread( &m_fileHeader.fileType, sizeof( m_fileHeader.fileType ), 1, m_file );
    std::cout << "File type:" << m_fileInfo.fileType << "\n";
    double creationTime;
	fread( &creationTime, sizeof( double ), 1, m_file );
	
    // TODO 
    //pp->CreationTime = StellateTimeToCTime( CreationTime );		//	...au format 'time_t' C avec les mSec

    if( m_fileHeader.version >= 2 )
		m_fileInfo.createdBy = readText();
	else
		m_fileInfo.createdBy = "Harmonie 5.3 ou ant\xE9rieur";

	if( m_fileHeader.version >= 3 )
		m_fileInfo.lastModifiedBy = readText();
	else
		m_fileInfo.lastModifiedBy = m_fileInfo.createdBy;

    return true;
}

void HarmonieReader::closeFile() {
    if (m_file != nullptr) {
        fclose(m_file);
        m_file = nullptr;
    }
}

std::string HarmonieReader::readClassName()
{
	uint16_t size;

    // Read class name size to read
	fread( &size, sizeof( size ), 1, m_file );
    char *buffer = new char[size+1]; //  +1 for the null termination character

    // Read the class name
    fread( buffer, sizeof( char ), size, m_file );
    // Add the null termination character
    buffer[size] = 0; 
    std::string className;
    className = buffer;

    delete buffer;

    return className;
}

std::string HarmonieReader::readText()
{
	unsigned char cLon;
	uint16_t iLon;
	std::vector<char> Txt;
	char *pTxt;

	//	Lecture de la longueur du texte à lire
	fread( &cLon, sizeof( cLon ), 1, m_file );		//	Lecture de la longueur 'char'
	if( cLon == 0xff )								//	Indique une longueur 'uint16_t'
		fread( &iLon, sizeof( iLon ), 1, m_file );	//	Lecture de la longueur ,uint16_t'
	else
		iLon = cLon;								//	Longueur à lire

	//	Retour immédiat si la longueur est nulle
	if( !iLon )
	{	
		return "";
	}

	//	Allocation du tampon et lecture du texte
    char *buffer = new char[iLon+1]; //  +1 for the null termination character

    // Read the class name
    fread( buffer, sizeof( char ), iLon, m_file );
    // Add the null termination character
    buffer[iLon] = 0; 
    
    std::string text;
    text = buffer;

    delete buffer;

    return text;
}