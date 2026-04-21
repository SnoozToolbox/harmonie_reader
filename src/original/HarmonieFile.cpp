#include <iostream>
#include <stdlib.h>
#include <fstream>

#include "HarmonieFile.h"
#include "StdString.h"

using namespace Harmonie;

CHarmonieFile::CHarmonieFile( FILE *pFile, const char *pFileName, bool ReadEvents, bool DisplayMessages )
{
	//	Copie des arguments dans les variables membres
	m_File = pFile;
	m_SigFile = nullptr;
	m_FileInfo.FileName = pFileName;
	//m_DisplayMessages = DisplayMessages;

	//	Type de fichier et succès
	m_FileInfo.FileType = FileTypeHarmonie;
	m_OK = true;

	//	Initialisation des variables communes
	InitPSG();
    m_FileInfo.Version = 0;

	//	Initialisation des variables de classe
	InitHarmonie();

	//	Lecture de la version du fichier
	int16_t v;
	fread( &v, sizeof( v ), 1, m_File );
	m_FileInfo.Version = v;

	//	Lecture de chaque élément du fichier
	if( !ReadFileHeader() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read file header."; return; }
	if( !ReadPatientInfo() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read patient info."; return; }
	if( !ReadCoDec() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read codec."; return; }
	if( !ReadCalibration() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read calibration."; return; }
	if( !ReadPhysicalMontage() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read physical montage."; return; }
	if( !ReadRecordingMontage() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read recording montage."; return; }
	if( !ReadReformatedMontages() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read reformated montages."; return; }
	if( !ReadElecGroup() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read elec. group."; return; }

	//	Nombre de records de 64 points (LONRECORD)
	fread( &m_RecordCount, sizeof( m_RecordCount ), 1, m_File );

	//	Retour immédiat si les événements ne doivent pas être lus
	if( !ReadEvents ) return;

	//	Lecture des groupes et des événements
	if( !ReadStatusGroup() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read status group."; return; }
	if( !ReadStatusItem() ) { m_OK = false; m_lastError="ERROR CHarmonieFile:Could not read status item."; return; }

	//	Détermination de variables de groupes supplémentaires
	FindGroupVariables();

	//	Tri des événements car ils ne sont pas tout à fait chronologiques dans 0.05% des cas
	SortEventItems();

	//	Recherche des événements de section d'échantillon
	BuildSampleSections();

	//	Construit le tableau de stades de sommeil 'm_EventItem.EpochEvent' contenant les événements d'époques de sommeil
	BuildEpochEvents();

	//	Ajout des époques de stade manquant correspondant aux discontinuités
	AddMissingEpochs();

	//	Assigne le stade de sommeil pour chaque événement dans la variable 'EVTEM::SleepStage'
	AssignSleepStagesToEvents();

	//	Calcule la durée des événements
	ComputeEventsDuration();

	//	Fermeture du fichier et libération du tampon de lecture
	fclose( m_File ); m_File = nullptr;
	m_FileBuffer.clear();
}

CHarmonieFile::~CHarmonieFile()
{
	//	Libère les différents objets alloués
	CleanCommonObjects();
}

//	Initialisation des variables de classe
void CHarmonieFile::InitHarmonie()
{
	//	Pointeur vers la structure 'CPSGFile::PATIENTINFO' parente
	CPSGFile::PATIENTINFO *p = &((CPSGFile *)this)->m_PatientInfo;

	//	1 janvier 2000 GMT en date Harmonie. Correspond au nombre de secondes depuis
	// l'an 0. https://www.epochconverter.com/seconds-days-since-y0
	double d2000 = 63113904000;

	//	Valeurs fixes fréquemment utilisées
	TIMECONSTANT = 62167219200; // Convertis une date en nombre de seconde depuis l'an 0 à une date qui référence le 1er janvier 1970
	RECORDLENGTH = 64;
	RECMTG_ID = 0x000C;		//	ID du montage d'enregistrement dans 'm_EventGroup.Group[n].Montage'
	REFMTG0_ID = 0x000E;	//	ID du premier montage reformatté dans 'm_EventGroup.Group[n].Montage'

	//	m_FileInfo
	m_FileInfo.CreationTime = d2000;

	//	CFileHeader
	m_FileHeader.FileType = 1;
	m_FileHeader.Separator = 0x1ffff;
	m_FileHeader.ClassName = "CFileHeader";
	m_FileHeader.Version = 3;
	m_FileHeader.FileType = 1;

	//	CPatientInfo
	m_PatientInfo.Separator = 0x1ffff;
	m_PatientInfo.ClassName = "CPatientInfo";
	m_PatientInfo.Version = 3;
	p->BirthDate = d2000;
	p->Gender = GenderUnknown;

	//	Initialisation de 'm_CoDec'
	m_CoDec.Separator = 0x1ffff;
	m_CoDec.ClassName = "CStdCoDec";
	m_CoDec.Version = 1;
	m_CoDec.CompRecSize = 8;				//	Longueur de base d'un record = 8 octets pour l'heure.
	m_CoDec.SamplesRecord = RECORDLENGTH;

	//	Initialisation de 'm_RecordingCalibration'
	m_RecordingCalibration.Separator = 0x1ffff;
	m_RecordingCalibration.ClassName = "CCalibration";
	m_RecordingCalibration.Version = 3;
	m_RecordingCalibration.TrueSampleFrequency = 0;
	m_RecordingCalibration.BaseCalibrationMinADC = 0;
	m_RecordingCalibration.BaseCalibrationMinVolts = 0;
	m_RecordingCalibration.BaseCalibrationMaxADC = 1;
	m_RecordingCalibration.BaseCalibrationMaxVolts = 1;

	//	Initialisation de 'm_PhysicalMontage'
	m_PhysicalMontage.Separator = 0x1ffff;
	m_PhysicalMontage.ClassName = "CPhysicalMontage";
	m_PhysicalMontage.Version = 7;
	m_PhysicalMontage.MapType = MapType_None;
	m_PhysicalMontage.ElectrodeCountX3 = 0;

	//	Initialisation de 'm_RecordingMontage'
	m_RecordingMontage.Separator = 0x1ffff;
	m_RecordingMontage.ClassName = "CRecordingMontage";
	m_RecordingMontage.RecordingMontageCount = 1;
	m_RecordingMontage.Version = 7;
	m_RecordingMontage.SamplesRecord = 4194314;	//	Valeur constante dans tous les fichiers Harmonie
	m_BaseSampleFrequency = 0;

	//	Initialisation de 'CReformatedMontage'
	m_ReformatedMontages.Separator = 0x1ffff;
	m_ReformatedMontages.ClassName = "CReformatedMontage";

	//	Initialisation de 'CElecGroup'
	m_ElectrodeGroup.Separator = 0x1ffff;
	m_ElectrodeGroup.ClassName = "CElecGroup";
	m_ElectrodeGroup.First = true;

	//	Initialisation de 'CStatusGroup'
	m_EventGroup.Separator = 0x1ffff;
	m_EventGroup.ClassName = "CStatusGroup";
	m_LengthOfSleepEpochs = -1;

	//	Structure équivalente à 'CStatusItem'
	m_EventItem.Separator = 0x1ffff;
	m_EventItem.ClassName = "CStatusItem";

	//	Variables diverses
	m_RecordCount = 0;		//	Nombre de records de 'LONRECORD' points
	m_SampSectGroup = UINT_MAX;
	m_StageGroup = UINT_MAX;
	m_StageProperty = UINT_MAX;
}

//	Lit le nom de la classe dans 'Dest'
void CHarmonieFile::ReadClassName( std::string *Dest )
{
	uint16_t i16;
	std::vector<char> Txt;
	char *pTxt;

	//	Lecture de la taille du texte à lire
	fread( &i16, sizeof( i16 ), 1, m_File );
	Txt.resize( i16 + 1 );
	pTxt = Txt.data();
	fread( pTxt, sizeof( char ), i16, m_File );
	pTxt[i16] = 0;
	*Dest = pTxt;
}

//	Écrit le nom de la classe 'ClassName'
void CHarmonieFile::WriteClassName( const char *ClassName )
{
	//	Longueur du nom de la classe
	int16_t Lon = strlen( ClassName );

	//	Enregistrement du séparateur et du nom
	fwrite( &Lon, sizeof( Lon ), 1, m_File );
	fwrite( ClassName, sizeof( char ), Lon, m_File );
}

//	Lecture d'un texte dans le fichier et entreposage dans 'Dest'
void CHarmonieFile::ReadText( std::string *Dest )
{
	unsigned char cLon;
	uint16_t iLon;
	std::vector<char> Txt;
	char *pTxt;

	//	Lecture de la longueur du texte à lire
	fread( &cLon, sizeof( cLon ), 1, m_File );		//	Lecture de la longueur 'char'
	if( cLon == 0xff )								//	Indique une longueur 'uint16_t'
		fread( &iLon, sizeof( iLon ), 1, m_File );	//	Lecture de la longueur ,uint16_t'
	else
		iLon = cLon;								//	Longueur à lire

	//	Retour immédiat si la longueur est nulle
	if( !iLon )
	{	Dest->clear();
		return;
	}

	//	Allocation du tampon et lecture du texte
	Txt.resize( iLon + 1 );
	pTxt = Txt.data();
	fread( pTxt, sizeof( char ), iLon, m_File );
	pTxt[iLon] = 0;
	*Dest = pTxt;
}

//	Enregistrement de 'Text' dans le fichier
void CHarmonieFile::WriteText( const char *Text )
{
	unsigned char cLon;

	//	Longueur du texte
	int16_t iLon = strlen( Text );

	//	Enregistrement de la longueur du texte
	if( iLon >= 0xff )
	{	cLon = 0xff;
		fwrite( &cLon, sizeof( cLon ), 1, m_File );
		fwrite( &iLon, sizeof( iLon ), 1, m_File );
	}else
	{	cLon = (unsigned char )iLon;
		fwrite( &cLon, sizeof( cLon ), 1, m_File );
	}

	//	Retour immédiat si la longueur est nulle
	if( iLon == 0 ) return;

	//	Enregistrement du texte
	fwrite( Text, sizeof( char ), iLon, m_File );
}

//	Retourne l'heure 'Stellate' au format standard C (time_t), avec les mSec
double CHarmonieFile::StellateTimeToCTime( double StellateTime )
{
	if( StellateTime > 0 )
		return StellateTime - TIMECONSTANT;
	else
		return 0;
}

//	Retourne l'heure 'time_t' C avec les mSec au format Stellate
double CHarmonieFile::CTimeToStellateTime( double CTime_mSec )
{
	return CTime_mSec + TIMECONSTANT;
}

//	Lecture de la classe 'CFileHeader'
bool CHarmonieFile::ReadFileHeader()
{
	std::string ClassName( "CFileHeader" );
	std::string FunctionName( "CHarmonieFile::ReadFileHeader" );

	std::string s1, s2;
	double CreationTime;

	//	Pointeur vers les destinations locale et parente
	FILEHEADER *pl = &m_FileHeader;
	FILEINFO *pp = &m_FileInfo;

	//	Lecture du séparateur et du nom de la classe
	fread( &pl->Separator, sizeof( pl->Separator ), 1, m_File );
	ReadClassName( &pl->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = pl->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	
        ////DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture des informations communes à toutes les versions
	fread( &pl->Version, sizeof( pl->Version ), 1, m_File );	//	Version de la classe
	ReadText( &pp->Description );								//	Description
	ReadText( &pp->Institution );								//	Institution
	fread( &pl->FileType, sizeof( pl->FileType ), 1, m_File );	//	Type de fichier: EEG CSA etc...
	fread( &CreationTime, sizeof( double ), 1, m_File );		//	CreationTime...
	pp->CreationTime = StellateTimeToCTime( CreationTime );		//	...au format 'time_t' C avec les mSec

	//	Lecture des informations spécifiques à chaque version
	if( pl->Version >= 2 )
		ReadText( &pp->CreatedBy );								//	CreatedBy
	else
		pp->CreatedBy = "Harmonie 5.3 ou ant\xE9rieur";
	if( pl->Version >= 3 )
		ReadText( &pp->LastModifiedBy );						//	LastModifiedBy
	else
		pp->LastModifiedBy = pp->CreatedBy;

	//	Retour
	return true;
}

//	Lecture de la classe 'CPatientInfo'
bool CHarmonieFile::ReadPatientInfo()
{
	std::string ClassName( "CPatientInfo" );
	std::string FunctionName( "CHarmonieFile::ReadPatientInfo" );

	std::string s1, s2;
	double BirthDate;
	int16_t iSex;

	//	Pointeur vers la classe de destination
	PATIENTINFO *p = &m_PatientInfo;

	//	Pointeur vers la structure 'CPSGFile::PATIENTINFO' parente
	CPSGFile::PATIENTINFO *pp = &((CPSGFile *)this)->m_PatientInfo;

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	////DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture des informations communes à toutes les versions
	fread( &p->Version, sizeof( p->Version ), 1, m_File );
	ReadText( &pp->Id1 );
	ReadText( &pp->Id2 );
	ReadText( &pp->LastName );
	ReadText( &pp->FirstName );
	fread( &BirthDate, sizeof( double ), 1, m_File );	//	Date de naissance...
	pp->BirthDate = StellateTimeToCTime( BirthDate );	//	...au format 'time_t' C avec les mSec
	fread( &iSex, sizeof( iSex ), 1, m_File );
	if( iSex == PatientSex_Male )
		pp->Gender = GenderMale;
	else if( iSex == PatientSex_Female )
		pp->Gender = GenderFemale;
	ReadText( &pp->Address );
	ReadText( &pp->City );
	ReadText( &pp->State );
	ReadText( &pp->Country );
	ReadText( &pp->ZipCode );
	ReadText( &pp->HomePhone );
	ReadText( &pp->WorkPhone );
	ReadText( &pp->Comments );

	//	Lecture des informations spécifiques à chaque version
	if( p->Version >= 2 )
	{	ReadText( &pp->Height );
		ReadText( &pp->Weight );
	}
	if( p->Version >= 3 )
	{	ReadText( &p->UserFieldP1 );
		ReadText( &p->UserFieldP2 );
		ReadText( &p->UserFieldP3 );
		ReadText( &p->UserFieldP4 );
		ReadText( &p->UserFieldP5 );
	}

	//	Composition du nom complet
	if( pp->FirstName.length() )
	{	pp->Name = pp->LastName;
		pp->Name += ", ";
		pp->Name += pp->FirstName;
	}
	else
		pp->Name = pp->LastName;

	//	Composition de l'identificateur complet
	if( pp->Id2.length() )
	{	pp->Id = pp->Id1;
		pp->Id += ", ";
		pp->Id += pp->Id2;
	}
	else
		pp->Id = pp->Id1;

	//	Retour
	return true;
}

//	Lecture de la classe 'CCODEC'
bool CHarmonieFile::ReadCoDec()
{
	//	Il peut y avoir 2 noms de classes :
	//	- 'CStdCoDec' pour les fichiers 'STS' assoscés à un fichier 'SIG' (la très grande majorité)
	//	- ''
	std::string ClassName1( "CDosCoDec" );
	std::string ClassName2( "CStdCoDec" );
	std::string FunctionName( "CHarmonieFile::ReadCoDec" );

	std::string s1, s2, s3;
	int i;
	std::vector<char> Buf;
	char *pBuf;
	char c;
	const char *ps1;
	long pStart, pEnd;
	int16_t NumChannels;
	int16_t *pSamplesChannel;
	uint16_t DosCoDecSize;

	//	Pointeur vers la classe de destination
	CCODEC *p = &m_CoDec;

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName1;
	s2 = ClassName2;
	s3 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	std::transform( s3.begin(), s3.end(), s3.begin(), ::tolower );
	if( s1 != s3 && s2 != s3 )
	{	s1 = ClassName1; s1 += '/'; s1 += ClassName2;
		//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture des informations de base
	fread( &p->Version, sizeof( p->Version ), 1, m_File );
	fread( &NumChannels, sizeof( NumChannels ), 1, m_File );
	fread( &p->CompRecSize, sizeof( p->CompRecSize ), 1, m_File );
	fread( &p->SamplesRecord, sizeof( p->SamplesRecord ), 1, m_File );

	//	Ajustement de la taille de 'CCODEC::SamplesChannel' et lecture des données
	p->SamplesChannel.resize( NumChannels );
	pSamplesChannel = p->SamplesChannel.data();
	for( i = 0; i < NumChannels; i++ )
		fread( &pSamplesChannel[i], sizeof( int16_t ), 1, m_File );

	//	Données supplémentaires inconnues la classe 'CDosCoDec' (fichiers 'STS' associés à un fichier 'EEG').
	//	Elles semblent toujours avoir une longueur de 28 octets.

	//	Recherche du prochain séparateur (normalement 'CCalibration')
	s1 = "CCalibration"; ps1 = s1.c_str();								//	Nom du séparateur et son pointeur
	i = s1.length();													//	Sa longueur dans 'i'
	pEnd = i - 1;														//	...-1 dans 'pEnd'
	Buf.resize( i );													//	Allocation de 'Buf'...
	pBuf = Buf.data();													//	...et son pointeur
	pStart = ftell( m_File );											//	Position courante dans le fichier
	while( fread( &c, sizeof( char ), 1, m_File ) == sizeof( char ) )	//	Lecture du fichier caractère par caractère
	{	memcpy( pBuf, pBuf + 1, pEnd );									//	Déplacement de 'Buf' de 1 octet vers la gauche...
		pBuf[pEnd] = c;													//	...et copie du caractère lu à la fin de 'Buf'
		if( !memcmp( pBuf, ps1, i ) ) break;							//	Fin de la recherche si trouvé
	}

	pEnd = ftell( m_File );												//	Position courante dans le fichier
	pEnd -= i;															//	Moins 'CCalibration'
	pEnd -= sizeof( uint32_t ); pEnd -= sizeof( uint16_t );				//	Moins '' et '' (séparateur et longueur du nom de la classe)
	DosCoDecSize = (uint16_t)( pEnd - pStart );							//	Taille du bloc
	fseek( m_File, pStart, SEEK_SET );									//	Repositionnement à la position originale
	if( DosCoDecSize )													//	S'il y a des données à lire
	{	p->DosCoDec.resize( DosCoDecSize );
		fread( p->DosCoDec.data(), sizeof( char ), DosCoDecSize, m_File );
	}

	//	Retour
	return true;
}

//	Lecture de la classe 'CCalibration'
bool CHarmonieFile::ReadCalibration()
{
	std::string ClassName( "CCalibration" );
	std::string FunctionName( "CHarmonieFile::ReadCalibration" );

	std::string s1, s2;
	uint16_t Count, i;

	//	Pointeur vers la classe de destination
	RECORDINGCALIBRATION *p = &m_RecordingCalibration;

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture des informations de base
	fread( &p->Version, sizeof( p->Version ), 1, m_File );
	fread( &p->TrueSampleFrequency, sizeof( p->TrueSampleFrequency ), 1, m_File );
	fread( &p->BaseCalibrationMinADC, sizeof( p->BaseCalibrationMinADC ), 1, m_File );
	fread( &p->BaseCalibrationMinVolts, sizeof( p->BaseCalibrationMinVolts ), 1, m_File );
	fread( &p->BaseCalibrationMaxADC, sizeof( p->BaseCalibrationMaxADC ), 1, m_File );
	fread( &p->BaseCalibrationMaxVolts, sizeof( p->BaseCalibrationMaxVolts ), 1, m_File );

	//	Lecture des informations des canaux et ajustement de la taille de 'RECORDINGCALIBRATION::Channel'
	fread( &Count, sizeof( Count ), 1, m_File );
	p->Channel.resize( Count );
	CHANNELCALIBRATION *pChannel = p->Channel.data();
	memset( pChannel, 0, Count * sizeof( CHANNELCALIBRATION ) );

	//	Lecture des informations de chaque canal, dépendant des versions
	if( p->Version == 1 )
	{	for( i = 0; i < Count; i++ )
			fread( &pChannel[i].awUnit, sizeof( pChannel[i].awUnit ), 1, m_File );
	}
	for( i = 0; i < Count; i++ )
		fread( &pChannel[i].als1, sizeof( pChannel[i].als1 ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &pChannel[i].OutputMin, sizeof( pChannel[i].OutputMin ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &pChannel[i].InputMin, sizeof( pChannel[i].InputMin ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &pChannel[i].als2, sizeof( pChannel[i].als2 ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &pChannel[i].OutputMax, sizeof( pChannel[i].OutputMax ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &pChannel[i].InputMax, sizeof( pChannel[i].InputMax ), 1, m_File );

	//	Lecture des informations de position selon la version
	if( p->Version > 2 )
	{	fread( &Count, sizeof( Count ), 1, m_File );
		p->BodyPosition.resize( Count );
		BODYPOSITION *pBodyPosition = p->BodyPosition.data();
		fread( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			fread( &pBodyPosition[i].BposThres, sizeof( pBodyPosition[i].BposThres ), 1, m_File );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			ReadText( &pBodyPosition[i].BposKeys );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			ReadText( &pBodyPosition[i].BposNames );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			fread( &pBodyPosition[i].BposUids, sizeof( pBodyPosition[i].BposUids ), 1, m_File );
	}

	//	Retour
	return true;
}

//	Lecture de la classe 'CPhysicalMontage'
bool CHarmonieFile::ReadPhysicalMontage()
{
	std::string ClassName = "CPhysicalMontage";
	std::string FunctionName( "CHarmonieFile::ReadPhysicalMontage" );

	std::string s1, s2;
	uint16_t Count, i;
	size_t s;

	//	Pointeur vers la classe de destination
	PHYSICALMONTAGE *p = &m_PhysicalMontage;

	//	Pointeur vers la structure 'CPSGFile::PATIENTINFO' parente
	CPSGFile::PHYSICALMONTAGE *pp = &((CPSGFile *)this)->m_PhysicalMontage;

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture des informations de base
	fread( &p->Version, sizeof( p->Version ), 1, m_File );
	ReadText( &pp->MontageName );

	//	Lecture du nombre de canaux et ajustement de la taille de 'PHYSICALMONTAGE::Electrode'
	fread( &Count, sizeof( Count ), 1, m_File );
	p->Electrode.resize( Count );
	pp->ElectrodeName.resize( Count );

	//	Lecture des noms d'électrodes
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		ReadText( &pp->ElectrodeName[i] );

	//	Map Type
	fread( &i, sizeof( i ), 1, m_File );
	p->MapType = (EMapType)i;

	//	Nombre de positions d'électrodes à lire ci-dessous (3 positions par électrode)
	fread( &p->ElectrodeCountX3, sizeof( p->ElectrodeCountX3 ), 1, m_File );

	//	Lecture des positions d'électrodes
	for( s = 0; s < p->Electrode.size(); s++ )
	{	fread( &p->Electrode[s].Coord1, sizeof( p->Electrode[s].Coord1 ), 1, m_File );
		fread( &p->Electrode[s].Coord2, sizeof( p->Electrode[s].Coord2 ), 1, m_File );
		fread( &p->Electrode[s].Coord3, sizeof( p->Electrode[s].Coord3 ), 1, m_File );
	}

	//	Retour
	return true;
}

//	Lecture de la classe 'CRecordingMontage'
bool CHarmonieFile::ReadRecordingMontage()
{
	std::string ClassName = "CRecordingMontage";
	std::string FunctionName( "CHarmonieFile::ReadRecordingMontage" );

	std::string s1, s2;
	uint16_t Count, i, i16;

	//	Pointeur vers la classe de destination
	RECORDINGMONTAGE *p = &m_RecordingMontage;

	//	Nombre de montages d'enregistrement
	fread( &p->RecordingMontageCount, sizeof( p->RecordingMontageCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu (ne devrait jamais arriver car il y a toujours 1 montage d'enregistrement)
	if( !p->RecordingMontageCount ) return true;

	//	Ajustement du nombre de montages
	m_Montages.resize( 1 );

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture des informations de base
	fread( &p->Version, sizeof( p->Version ), 1, m_File );
	ReadText( &m_Montages[0].MontageName );

	//	Lecture du nombre de canaux et ajustement de la taille de 'PHYSICALMONTAGE::Electrode'
	fread( &Count, sizeof( Count ), 1, m_File );

	//	Allocation du tableau de canaux
	if( Count )
	{	p->Channel.resize( Count );
		m_Montages[0].Channel.resize( Count );
	}

	//	Lecture des informations 'CHANNEL' des canaux
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &m_Montages[0].Channel[i].SampleFrequency, sizeof( m_Montages[0].Channel[i].SampleFrequency ), 1, m_File );
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
	{	fread( &i16, sizeof( i16 ), 1, m_File );
		m_Montages[0].Channel[i].ChannelType = EChannelType2EPSGChannelType( (EChannelType)i16 );
	}
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
	{	fread( &i16, sizeof( i16 ), 1, m_File );
		m_Montages[0].Channel[i].ChannelUnits = EChannelUnits2EPSGChannelUnits( (EChannelUnits)i16 );
		m_Montages[0].Channel[i].Unit = (int)m_Montages[0].Channel[i].ChannelUnits;
	}
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &p->Channel[i].SpikeOptions, sizeof( p->Channel[i].SpikeOptions ), 1, m_File );
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &p->Channel[i].SeizureOptions, sizeof( p->Channel[i].SeizureOptions ), 1, m_File );
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &m_Montages[0].Channel[i].ChannelColor, sizeof( m_Montages[0].Channel[i].ChannelColor ), 1, m_File );

	//	Suite de la lecture
	fread( &p->SamplesRecord, sizeof( p->SamplesRecord ), 1, m_File );
	fread( &m_BaseSampleFrequency, sizeof( m_BaseSampleFrequency ), 1, m_File );

	//	Lecture des informations supplémentaires 'RECCHANNEL' des canaux

	//	Lecture des noms d'électrodes pour chaque canal
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		ReadText( &p->Channel[i].Electrode1Name );
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		ReadText( &p->Channel[i].Electrode2Name );

	//	Composition du nom complet de l'électrode
	for( i = 0; i < Count; i++ )
	{	if( p->Channel[i].Electrode2Name.length() )
		{	m_Montages[0].Channel[i].ChannelName = p->Channel[i].Electrode1Name;
			m_Montages[0].Channel[i].ChannelName += "-";
			m_Montages[0].Channel[i].ChannelName += p->Channel[i].Electrode2Name;
		}else
			m_Montages[0].Channel[i].ChannelName = p->Channel[i].Electrode1Name;
	}

	//	Variables de canaux inconnues
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fread( &p->Channel[i].UnderSamplingMethod, sizeof( p->Channel[i].UnderSamplingMethod ), 1, m_File );
	fread( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		ReadText( &p->Channel[i].Preprocessing );

	//	Retour
	return true;
}

//	Convertit 'EChannelType' en 'CPSGFile::EPSGChannelType'
CPSGFile::EPSGChannelType CHarmonieFile::EChannelType2EPSGChannelType( EChannelType Type )
{
	if( Type == ChannelType_None ) return CPSGFile::ChannelType_None;
	else if( Type == ChannelType_EEG ) return CPSGFile::ChannelType_EEG;
	else if( Type == ChannelType_SEEG ) return CPSGFile::ChannelType_SEEG;
	else if( Type == ChannelType_EMG ) return CPSGFile::ChannelType_EMG;
	else if( Type == ChannelType_ESP ) return CPSGFile::ChannelType_ESP;
	else if( Type == ChannelType_SAO2 ) return CPSGFile::ChannelType_SAO2;
	else if( Type == ChannelType_Movement ) return CPSGFile::ChannelType_Movement;
	else if( Type == ChannelType_EOG ) return CPSGFile::ChannelType_EOG;
	else if( Type == ChannelType_EKG ) return CPSGFile::ChannelType_ECG;
	else if( Type == ChannelType_X1 ) return CPSGFile::ChannelType_X1;
	else if( Type == ChannelType_X2 ) return CPSGFile::ChannelType_X2;
	else if( Type == ChannelType_X3 ) return CPSGFile::ChannelType_X3;
	else if( Type == ChannelType_X4 ) return CPSGFile::ChannelType_X4;
	else if( Type == ChannelType_X5 ) return CPSGFile::ChannelType_X5;
	else if( Type == ChannelType_X6 ) return CPSGFile::ChannelType_X6;
	else if( Type == ChannelType_X7 ) return CPSGFile::ChannelType_X7;
	else if( Type == ChannelType_X8 ) return CPSGFile::ChannelType_X8;
	else if( Type == ChannelType_PhoticStim ) return CPSGFile::ChannelType_PhoticStim;
	else if( Type == ChannelType_Pressure ) return CPSGFile::ChannelType_Pressure;
	else if( Type == ChannelType_Volume ) return CPSGFile::ChannelType_Volume;
	else if( Type == ChannelType_Acidity ) return CPSGFile::ChannelType_Acidity;
	else if( Type == ChannelType_Temperature ) return CPSGFile::ChannelType_Temperature;
	else if( Type == ChannelType_Position ) return CPSGFile::ChannelType_Position;
	else if( Type == ChannelType_Flow ) return CPSGFile::ChannelType_Flow;
	else if( Type == ChannelType_Effort ) return CPSGFile::ChannelType_Effort;
	else if( Type == ChannelType_Rate ) return CPSGFile::ChannelType_Rate;
	else if( Type == ChannelType_Length ) return CPSGFile::ChannelType_Length;
	else if( Type == ChannelType_Light ) return CPSGFile::ChannelType_Light;
	else if( Type == ChannelType_DigitalInput ) return CPSGFile::ChannelType_DigitalInput;
	else if( Type == ChannelType_Microphone ) return CPSGFile::ChannelType_Microphone;
	else if( Type == ChannelType_ECoG ) return CPSGFile::ChannelType_ECoG;
	else if( Type == ChannelType_CO2 ) return CPSGFile::ChannelType_CO2;
	else if( Type == ChannelType_Pleth ) return CPSGFile::ChannelType_Pleth;
	else return CPSGFile::ChannelType_Unknown;
}

//	Convertit 'CPSGFile::EPSGChannelType' en 'CHarmonieFile::EChannelType'
CHarmonieFile::EChannelType CHarmonieFile::EPSGChannelType2EChannelType( CPSGFile::EPSGChannelType Type )
{
	if( Type == CPSGFile::ChannelType_None ) return ChannelType_None;
	else if( Type == CPSGFile::ChannelType_EEG ) return ChannelType_EEG;
	else if( Type == CPSGFile::ChannelType_SEEG ) return ChannelType_SEEG;
	else if( Type == CPSGFile::ChannelType_EMG ) return ChannelType_EMG;
	else if( Type == CPSGFile::ChannelType_ESP ) return ChannelType_ESP;
	else if( Type == CPSGFile::ChannelType_SAO2 ) return ChannelType_SAO2;
	else if( Type == CPSGFile::ChannelType_Movement ) return ChannelType_Movement;
	else if( Type == CPSGFile::ChannelType_EOG ) return ChannelType_EOG;
	else if( Type == CPSGFile::ChannelType_ECG ) return ChannelType_EKG;
	else if( Type == CPSGFile::ChannelType_X1 ) return ChannelType_X1;
	else if( Type == CPSGFile::ChannelType_X2 ) return ChannelType_X2;
	else if( Type == CPSGFile::ChannelType_X3 ) return ChannelType_X3;
	else if( Type == CPSGFile::ChannelType_X4 ) return ChannelType_X4;
	else if( Type == CPSGFile::ChannelType_X5 ) return ChannelType_X5;
	else if( Type == CPSGFile::ChannelType_X6 ) return ChannelType_X6;
	else if( Type == CPSGFile::ChannelType_X7 ) return ChannelType_X7;
	else if( Type == CPSGFile::ChannelType_X8 ) return ChannelType_X8;
	else if( Type == CPSGFile::ChannelType_PhoticStim ) return ChannelType_PhoticStim;
	else if( Type == CPSGFile::ChannelType_Pressure ) return ChannelType_Pressure;
	else if( Type == CPSGFile::ChannelType_Volume ) return ChannelType_Volume;
	else if( Type == CPSGFile::ChannelType_Acidity ) return ChannelType_Acidity;
	else if( Type == CPSGFile::ChannelType_Temperature ) return ChannelType_Temperature;
	else if( Type == CPSGFile::ChannelType_Position ) return ChannelType_Position;
	else if( Type == CPSGFile::ChannelType_Flow ) return ChannelType_Flow;
	else if( Type == CPSGFile::ChannelType_Effort ) return ChannelType_Effort;
	else if( Type == CPSGFile::ChannelType_Rate ) return ChannelType_Rate;
	else if( Type == CPSGFile::ChannelType_Length ) return ChannelType_Length;
	else if( Type == CPSGFile::ChannelType_Light ) return ChannelType_Light;
	else if( Type == CPSGFile::ChannelType_DigitalInput ) return ChannelType_DigitalInput;
	else if( Type == CPSGFile::ChannelType_Microphone ) return ChannelType_Microphone;
	else if( Type == CPSGFile::ChannelType_ECoG ) return ChannelType_ECoG;
	else if( Type == CPSGFile::ChannelType_CO2 ) return ChannelType_CO2;
	else if( Type == CPSGFile::ChannelType_Pleth ) return ChannelType_Pleth;
	else return ChannelType_None;
}

//	Convertit 'EChannelUnits' en 'CPSGFile::EPSGChannelUnits'
CPSGFile::EPSGChannelUnits CHarmonieFile::EChannelUnits2EPSGChannelUnits( EChannelUnits Unit )
{
	if( Unit == ChannelUnits_None ) return CPSGFile::ChannelUnits_None;
	else if( Unit == ChannelUnits_MicroVolt ) return CPSGFile::ChannelUnits_MicroVolt;
	else if( Unit == ChannelUnits_MilliVolt ) return CPSGFile::ChannelUnits_MilliVolt;
	else if( Unit == ChannelUnits_Volt ) return CPSGFile::ChannelUnits_Volt;
	else if( Unit == ChannelUnits_Percent ) return CPSGFile::ChannelUnits_Percent;
	else if( Unit == ChannelUnits_CMH2O ) return CPSGFile::ChannelUnits_CMH2O;
	else if( Unit == ChannelUnits_MMHG ) return CPSGFile::ChannelUnits_MMHG;
	else if( Unit == ChannelUnits_CC ) return CPSGFile::ChannelUnits_CC;
	else if( Unit == ChannelUnits_ML ) return CPSGFile::ChannelUnits_ML;
	else if( Unit == ChannelUnits_PH ) return CPSGFile::ChannelUnits_PH;
	else if( Unit == ChannelUnits_Celsius ) return CPSGFile::ChannelUnits_Celsius;
	else if( Unit == ChannelUnits_Fahrenheit ) return CPSGFile::ChannelUnits_Fahrenheit;
	else if( Unit == ChannelUnits_BPM ) return CPSGFile::ChannelUnits_BPM;
	else if( Unit == ChannelUnits_MM ) return CPSGFile::ChannelUnits_MM;
	else if( Unit == ChannelUnits_CM ) return CPSGFile::ChannelUnits_CM;
	else if( Unit == ChannelUnits_Lux ) return CPSGFile::ChannelUnits_Lux;
	else if( Unit == ChannelUnits_AD ) return CPSGFile::ChannelUnits_AD;
	else return CPSGFile::ChannelUnits_Unknown;
}

//	Convertit 'CPSGFile::EPSGChannelUnits' en 'CHarmonieFile::EChannelUnits'
CHarmonieFile::EChannelUnits CHarmonieFile::EPSGChannelUnits2EChannelUnits( CPSGFile::EPSGChannelUnits Unit )
{
	if( Unit == CPSGFile::ChannelUnits_None ) return ChannelUnits_None;
	else if( Unit == CPSGFile::ChannelUnits_MicroVolt ) return ChannelUnits_MicroVolt;
	else if( Unit == CPSGFile::ChannelUnits_MilliVolt ) return ChannelUnits_MilliVolt;
	else if( Unit == CPSGFile::ChannelUnits_Volt ) return ChannelUnits_Volt;
	else if( Unit == CPSGFile::ChannelUnits_Percent ) return ChannelUnits_Percent;
	else if( Unit == CPSGFile::ChannelUnits_CMH2O ) return ChannelUnits_CMH2O;
	else if( Unit == CPSGFile::ChannelUnits_MMHG ) return ChannelUnits_MMHG;
	else if( Unit == CPSGFile::ChannelUnits_CC ) return ChannelUnits_CC;
	else if( Unit == CPSGFile::ChannelUnits_ML ) return ChannelUnits_ML;
	else if( Unit == CPSGFile::ChannelUnits_PH ) return ChannelUnits_PH;
	else if( Unit == CPSGFile::ChannelUnits_Celsius ) return ChannelUnits_Celsius;
	else if( Unit == CPSGFile::ChannelUnits_Fahrenheit ) return ChannelUnits_Fahrenheit;
	else if( Unit == CPSGFile::ChannelUnits_BPM ) return ChannelUnits_BPM;
	else if( Unit == CPSGFile::ChannelUnits_MM ) return ChannelUnits_MM;
	else if( Unit == CPSGFile::ChannelUnits_CM ) return ChannelUnits_CM;
	else if( Unit == CPSGFile::ChannelUnits_Lux ) return ChannelUnits_Lux;
	else if( Unit == CPSGFile::ChannelUnits_AD ) return ChannelUnits_AD;
	else return ChannelUnits_None;
}

//	Lecture de la classe 'CReformatedMontage'
bool CHarmonieFile::ReadReformatedMontages()
{
	std::string ClassName = "CReformatedMontage";
	std::string FunctionName( "CHarmonieFile::ReadReformatedMontages" );

	std::string s1, s2;
	uint16_t MtgCount, Count, i, j, m, i16, Last;

	//	Pointeur vers la classe de destination
	REFORMATEDMONTAGE *p = &m_ReformatedMontages;

	//	Nombre de montages reformattés
	fread( &MtgCount, sizeof( MtgCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu
	if( !MtgCount ) return true;

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Ajustement du nombre de montages
	if( MtgCount )
	{	p->Montage.resize( MtgCount );		//	Nombre de montages reformatés Harmonie
		m_Montages.resize( MtgCount + 1 );	//	Nombre totaux de montages (incluant le montage d'enregistrement)
	}

	//	Lecture de chaque montage
	Last = MtgCount - 1;
	for( i = 0; i < MtgCount; i++ )
	{	//	Index du montage dans le vecteur 'm_Montages' (0 est le montage d'enregistrement)
		m = i + 1;

		//	Version et nom du montage
		fread( &p->Montage[i].Version, sizeof( p->Montage[i].Version ), 1, m_File );
		ReadText( &m_Montages[m].MontageName );

		//	Nombre de canaux
		fread( &Count, sizeof( Count ), 1, m_File );

		//	Ajustement de la taille de 'REFMONTAGE::Channel'
		if( Count )
		{	p->Montage[i].Channel.resize( Count );
			m_Montages[m].Channel.resize( Count );
		}

		//	Lecture des informations 'CHANNEL' des canaux
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fread( &m_Montages[m].Channel[j].SampleFrequency, sizeof( m_Montages[m].Channel[j].SampleFrequency ), 1, m_File );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
		{	fread( &i16, sizeof( i16 ), 1, m_File );
			m_Montages[m].Channel[j].ChannelType = EChannelType2EPSGChannelType( (EChannelType)i16 );
		}
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
		{	fread( &i16, sizeof( i16 ), 1, m_File );
			m_Montages[m].Channel[j].ChannelUnits = EChannelUnits2EPSGChannelUnits( (EChannelUnits)i16 );
		}
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fread( &p->Montage[i].Channel[j].SpikeOptions, sizeof( p->Montage[i].Channel[j].SpikeOptions ), 1, m_File );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fread( &p->Montage[i].Channel[j].SeizureOptions, sizeof( p->Montage[i].Channel[j].SeizureOptions ), 1, m_File );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fread( &m_Montages[m].Channel[j].ChannelColor, sizeof( m_Montages[m].Channel[j].ChannelColor ), 1, m_File );

		//	Suite de la lecture du montage (variable inconnue toujours égale à 12)
		fread( &p->Montage[i].Twelve, sizeof( p->Montage[i].Twelve ), 1, m_File );

		//	Lecture des informations supplémentaires 'REFCHANNEL' des canaux

		//	Lecture des noms et formules des canaux
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			ReadText( &m_Montages[m].Channel[j].ChannelName );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			ReadText( &p->Montage[i].Channel[j].ChannelFormula );

		//	Lecture des noms de macros et copie du nombre de macros dans 'MacroCount'
		fread( &Count, sizeof( Count ), 1, m_File );

		//	Ajustement de la taille de 'REFMONTAGE::Macro'
		if( Count )
			p->Montage[i].Macro.resize( Count );

		//	Lecture des noms de macros
		for( j = 0; j < Count; j++ )
			ReadText( &p->Montage[i].Macro[j].MacroLabel );

		//	Lecture des formules des macros
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			ReadText( &p->Montage[i].Macro[j].MacroFormula );

		//	Séparateur entre les montages SAUF POUR LE DERNIER
		if( i < Last )
			fread( &p->Montage[i].Separator, sizeof( p->Montage[i].Separator ), 1, m_File );
	}

	//	Retour
	return true;
}

//	Lecture de la classe non documentée 'CElecGroup'
bool CHarmonieFile::ReadElecGroup()
{
	std::string ClassName = "CElecGroup";
	std::string FunctionName( "CHarmonieFile::ReadElecGroup" );

	std::string s1, s2;
	uint16_t ElecGroupCount, ElectrodeCount;
	uint16_t i, j, LastGroup, LastElectrode;
	int32_t i32;
	ELGROUP *pElGr;

	//	Pointeur vers la classe de destination
	ELECTRODEGROUP *p = &m_ElectrodeGroup;
	p->First = true;

	//	Retour immédiat si la version de 'm_RecordingMontage' est inférieure à 5
	//	car la classe 'CElecGroup' n'existait pas avant
	if( m_RecordingMontage.Version < 5 ) return true;

	//	Nombre de 'CElecGroup'
	fread( &ElecGroupCount, sizeof( ElecGroupCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu
	if( !ElecGroupCount ) return true;

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Ajustement de la taille de 'ELECTRODEGROUP::ElecGroup'
	p->ElecGroup.resize( ElecGroupCount );

	//	Lecture de chaque groupe d'électrode
	LastGroup = ElecGroupCount - 1;
	for( i = 0; i < ElecGroupCount; i++ )
	{	//	Pointeur vers le groupe d'électrodes 'ELGROUP'
		pElGr = &p->ElecGroup[i];

		//	Version et nom du groupe d'électrodes
		fread( &pElGr->Version, sizeof( pElGr->Version ), 1, m_File );
		ReadText( &pElGr->GroupName );

		//	Variables du groupe d'électrodes
		fread( &pElGr->ContactSize, sizeof( pElGr->ContactSize ), 1, m_File );
		fread( &pElGr->InterDistance, sizeof( pElGr->InterDistance ), 1, m_File );
		fread( &i32, sizeof( i32 ), 1, m_File ); pElGr->Type = (EElectrodeGroupType)i32;
		fread( &pElGr->NumCol, sizeof( pElGr->NumCol ), 1, m_File );
		fread( &pElGr->NumRow, sizeof( pElGr->NumRow ), 1, m_File );
		fread( &i32, sizeof( i32 ), 1, m_File ); pElGr->RowOrColWise = (EElectrodeGroupSequence)i32;
		fread( &pElGr->Color, sizeof( pElGr->Color ), 1, m_File );
		fread( &i32, sizeof( i32 ), 1, m_File ); pElGr->OriginPoint = (EElectrodeGroupOriginPoint)i32;

		//	Nombre d'électrodes, allocation et ajustement de 'ELGROUP::Electrode'
		fread( &ElectrodeCount, sizeof( ElectrodeCount ), 1, m_File );
		if( ElectrodeCount )
			pElGr->Electrode.resize( ElectrodeCount );

		//	Séparateur SAUF POUR LE PREMIER GROUPE D'ÉLECTRODES
		if( i > 0 )
			fread( &pElGr->Separateur1, sizeof( pElGr->Separateur1 ), 1, m_File );

		//	Lecture de chaque électrode
		LastElectrode = ElectrodeCount - 1;
		for( j = 0; j < ElectrodeCount; j++ )
		{	if( !ReadElectrode( &pElGr->Electrode[j] ) ) return false;

			//	Séparateur SAUF POUR LA DERNIÈRE ÉLECTRODE
			if( j < LastElectrode )
				fread( &pElGr->Separateur2, sizeof( pElGr->Separateur2 ), 1, m_File );
		}

		//	Séparateur SAUF POUR LE DERNIER GROUPE D'ÉLECTRODES
		if( i < LastGroup )
			fread( &pElGr->Separateur3, sizeof( pElGr->Separateur3 ), 1, m_File );
	}

	//	Retour
	return true;
}

//	Lecture de la classe non documentée 'CElectrode' dans 'pElectrode'
bool CHarmonieFile::ReadElectrode( ELECTRODE *pElectrode )
{
	std::string ClassName = "CElectrode";
	std::string FunctionName( "CHarmonieFile::ReadElectrode" );

	std::string s1, s2;

	//	NOTE : Le nom de la classe n'est lu qu'une fois lors de la première lecture
	if( m_ElectrodeGroup.First )
	{	//	Lecture du séparateur et du nom de la classe
		fread( &pElectrode->Separator, sizeof( pElectrode->Separator ), 1, m_File );
		ReadClassName( &pElectrode->ClassName );

		//	Vérification du nom de la classe
		s1 = ClassName;
		s2 = pElectrode->ClassName;
		std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
		std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
		if( s1 != s2 )
		{	//DisplayMessage( ReadError, FunctionName.c_str() );
			return false;
		}
		m_ElectrodeGroup.First = false;
	}

	//	Lecture des variables de l'électrode
	fread( &pElectrode->Version, sizeof( pElectrode->Version ), 1, m_File );
	ReadText( &pElectrode->ElecName );
	fread( &pElectrode->PositionX, sizeof( pElectrode->PositionX ), 1, m_File );
	fread( &pElectrode->PositionY, sizeof( pElectrode->PositionY ), 1, m_File );

	//	Retour
	return true;
}

//	Lecture de la classe 'CStatusGroup'
bool CHarmonieFile::ReadStatusGroup()
{
	std::string ClassName = "CStatusGroup";
	std::string FunctionName( "CHarmonieFile::ReadStatusGroup" );

	std::string s1, s2;
	uint16_t EventGroupCount, Last, DefaultItemCount, Count;
	uint16_t ItemPropertyCount, GroupPropertyCount;
	int16_t i16;
	uint32_t i, j;
	size_t Pos;
	EGROUP *pGroup;
	std::vector<std::string> DefName;

	//	Pointeur vers la classe de destination
	EVENTGROUP *p = &m_EventGroup;

	//	Nombre de groupes d'événements
	fread( &EventGroupCount, sizeof( EventGroupCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu
	if( !EventGroupCount ) return true;

	//	Ajustement de 'EVENTGROUP::Group' et 'm_EventGroups'
	p->Group.resize( EventGroupCount );
	m_EventGroups.resize( EventGroupCount );

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Lecture de chaque groupe
	Last = EventGroupCount - 1;
	for( i = 0; i < EventGroupCount; i++ )
	{	//	Pointeur EGROUP;
		pGroup = &p->Group[i];

		//	Version de la classe
		fread( &pGroup->Version, sizeof( pGroup->Version ), 1, m_File );

		//	Id si la version > 2
		if( pGroup->Version > 2 ) ReadText( &pGroup->Id );

		//	Nom, description, Type, Qualifier, Extent
		ReadText( &m_EventGroups[i].Name );
		ReadText( &m_EventGroups[i].Description );
		fread( &j, sizeof( j ), 1, m_File ); pGroup->Type = (EGroupType)j;
		fread( &j, sizeof( j ), 1, m_File ); pGroup->Qualifier = (EGroupQualifier)j;
		fread( &j, sizeof( j ), 1, m_File ); pGroup->Extent = (EGroupExtent)j;

		//	Lecture du montage et canal selon la version
		if( pGroup->Version < 4 )
		{	//	Version inférieure à 4

			//	Lecture du montage et canal
			fread( &pGroup->Montage, sizeof( pGroup->Montage ), 1, m_File );
			fread( &i16, sizeof( i16 ), 1, m_File ); pGroup->MontageChannel = (EMontageChannel)i16;

			//	Si le canal n'est pas spécifique ( < 0 ), copie dans 'GroupChannel'
			if( i16 < 0 )
				pGroup->GroupChannel = (EGroupChannel)i16;
			else
			{	//	Le canal n'est pas spécifique. Il faut trouver son nom
				pGroup->GroupChannel = GroupChannel_Some;

				//	Si le montage d'enregistrement a été spécifié
				if( pGroup->Montage == RECMTG_ID )
				{	//	Composition du nom du canal à partir des électrodes du montage
					pGroup->Channel = m_Montages[0].Channel[i16].ChannelName;
				}else
				{	//	Si un montage reformatté a été spécifié
					i16 = pGroup->Montage - REFMTG0_ID;	//	Index du montage
					i16++;								//	...incrémenté car '0' est le montage d'enregistrement

					//	Copie du nom
					pGroup->Channel = m_Montages[i16].Channel[pGroup->MontageChannel].ChannelName;
			}	}

		}else
		{	//	Versions >= 4, le nom du canal est directement lu
			fread( &j, sizeof( j ), 1, m_File ); pGroup->GroupChannel = (EGroupChannel)j;
			ReadText( &pGroup->Channel );
		}

		//	Couleur et accès
		fread( &pGroup->Color, sizeof( pGroup->Color ), 1, m_File );
		fread( &j, sizeof( j ), 1, m_File ); pGroup->GroupAccess = (EGroupAccess)j;
		j = j & (uint32_t)GroupAccess_Deletable;	//	Pour éviter un avertissement de compilation GCC [-Wint-in-bool-context]
		m_EventGroups[i].IsDeletable = j != 0;

		//	Lecture des DefaultItemNames selon les versions
		if( pGroup->Version < 6 )
		{	//	Version inférieure à 6 : Il sont délimités par des virgules
			ReadText( &s1 );													//	Lecture de la liste délimitée
			DefName.reserve( 16 );												//	Pour aller plus vite
			while( ( Pos = s1.find( ',' ) ) != std::string::npos )				//	Tant qu'il y a des délimiteurs
			{	s2 = std::string( s1.begin(), s1.begin() + Pos );				//	Copie du nom dans 's2'...
				DefName.push_back( s2 );										//	Ajouté à 'DefName'
				s1 = std::string( s1.begin() + Pos + 1, s1.end() );				//	Suppression du nom
			}
			if( s1.length() ) DefName.push_back( s1 );							//	Ajout du dernier nom

			DefaultItemCount = (uint16_t)DefName.size();						//	Nombre de noms par défaut
			if( DefaultItemCount )												//	S'il y en a
			{	pGroup->DefaultItem.resize( DefaultItemCount );					//	Ajustement de la taille de 'EGROUP::DefaultItem'
				for( j = 0; j < DefaultItemCount; j++ )							//	Pour chaque nom
				{	pGroup->DefaultItem[j].Name = DefName[j];					//	Copie du nom
					pGroup->DefaultItem[j].Color = pGroup->Color;				//	Même couleur que le groupe
			}	}
			DefName.clear();
		}else
		{	//	Version >= 6, les noms sont directement lus + 2 autres variables
			fread( &DefaultItemCount, sizeof( DefaultItemCount ), 1, m_File );
			if( DefaultItemCount )
				pGroup->DefaultItem.resize( DefaultItemCount );
			for( j = 0; j < DefaultItemCount; j++ )
				ReadText( &pGroup->DefaultItem[j].Name );
			fread( &Count, sizeof( Count ), 1, m_File );
			for( j = 0; j < Count; j++ )
				fread( &pGroup->DefaultItem[j].Color, sizeof( pGroup->DefaultItem[j].Color ), 1, m_File );
		}

		//	Lecture de Visibility qui n'est que dans les versions > 4
		if( pGroup->Version > 4 )
		{	fread( &j, sizeof( j ), 1, m_File );
			pGroup->Visibility = (EVisibilityStatus)j;
		}
		else
			pGroup->Visibility = StatusVisible;

		//	Variable UseNameFromList pour les versions > 6
		if( pGroup->Version > 6 )
			fread( &pGroup->UseNameFromList, sizeof( pGroup->UseNameFromList ), 1, m_File );
		else
			pGroup->UseNameFromList = 0;

		//	Traitement des ItemProperties
		fread( &ItemPropertyCount, sizeof( ItemPropertyCount ), 1, m_File );
		if( ItemPropertyCount )
			pGroup->ItemProperty.resize( ItemPropertyCount );
		for( j = 0; j < ItemPropertyCount; j++ )
			ReadText( &pGroup->ItemProperty[j].Key );
		fread( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			ReadText( &pGroup->ItemProperty[j].Description );

		//	Traitement des GroupProperties pour les versions > 1
		if( pGroup->Version > 1 )
		{	fread( &GroupPropertyCount, sizeof( GroupPropertyCount ), 1, m_File );
			if( GroupPropertyCount )
				pGroup->GroupProperty.resize( GroupPropertyCount );
			for( j = 0; j < GroupPropertyCount; j++ )
				ReadText( &pGroup->GroupProperty[j].Key );
			fread( &Count, sizeof( Count ), 1, m_File );
			for( j = 0; j < Count; j++ )
				ReadText( &pGroup->GroupProperty[j].Description );
			fread( &Count, sizeof( Count ), 1, m_File );
			for( j = 0; j < Count; j++ )
				ReadText( &pGroup->GroupProperty[j].Value );
		}

		//	Séparateur entre les groupes SAUF POUR LE DERNIER
		if( i < Last )
			fread( &pGroup->Separator, sizeof( pGroup->Separator ), 1, m_File );
	}

	//	Retour
	return true;
}

//	Lecture de la classe 'CStatusItem'
bool CHarmonieFile::ReadStatusItem()
{
	std::string ClassName = "CStatusItem";
	std::string FunctionName( "CHarmonieFile::ReadStatusItem" );

	//	Pointeur vers la classe de destination
	EVENTITEM *p = &m_EventItem;

	uint32_t EventItemCount;
	uint16_t ItemPropertyCount;
	uint32_t i, i32, Last;
	uint16_t Grp0, VersionIt0, j;
	int16_t i16;
	double EventTime;
	std::string s1, s2;
	EITEM *pItem;
	std::vector<char>ItemProperties;
	char *ItemProperty;

	//	Nombre d'événements
	fread( &j, sizeof( j ), 1, m_File );
	if( j == 0xffff )
		fread( &EventItemCount, sizeof( EventItemCount ), 1, m_File );
	else
		EventItemCount = j;

	//	Sortie immédiate s'il y a lieu
	if( !EventItemCount ) return true;

	//	Ajustement de 'EVENTITEM::Item' et 'm_EventItems'
	p->Item.resize( EventItemCount );
	m_EventItems.resize( EventItemCount );

	//	Lecture du séparateur et du nom de la classe
	fread( &p->Separator, sizeof( p->Separator ), 1, m_File );
	ReadClassName( &p->ClassName );

	//	Vérification du nom de la classe
	s1 = ClassName;
	s2 = p->ClassName;
	std::transform( s1.begin(), s1.end(), s1.begin(), ::tolower );
	std::transform( s2.begin(), s2.end(), s2.begin(), ::tolower );
	if( s1 != s2 )
	{	//DisplayMessage( ReadError, FunctionName.c_str() );
		return false;
	}

	//	Détermination de l'identificateur du groupe 0, basé sur le séparateur de groupes
	Grp0 = ( m_EventGroup.Group[0].Separator & 0x00ff ) + 1;

	//	Lecture de chaque événement
	Last = EventItemCount - 1;
	VersionIt0 = 0;
	for( i = 0; i < EventItemCount; i++ )
	{
		//	Lecture du pointeur vers l'événement
		pItem = &p->Item[i];

		//	C'est un événement réel
		m_EventItems[i].RealEvent = true;

		//	Version de la classe
		fread( &pItem->Version, sizeof( pItem->Version ), 1, m_File );

		/*	Tous les événements d'un même fichier devraient avoir la même version.
			Sinon c'est que le fichier est corrompu.	*/
		if( i == 0 ) VersionIt0 = pItem->Version;		//	Copie de la version du premier événement
		if( pItem->Version != VersionIt0 )				//	Si la version diffère
		{	p->Item.clear();							//	Effacement de 'Item'
			return false;								//	Retour
		}

		//	Lecture du groupe
		fread( &i16, sizeof( i16 ), 1, m_File );
		i16 -= Grp0;
		m_EventItems[i].Group = i16;

		//	Lecture des positions / heures
		fread( &m_EventItems[i].StartSample, sizeof( m_EventItems[i].StartSample ), 1, m_File );	//	Point de début
		fread( &EventTime, sizeof( EventTime ), 1, m_File );										//	Date de début...
		m_EventItems[i].StartTime = StellateTimeToCTime( EventTime );								//	...au format 'time_t' C avec les mSec
		fread( &m_EventItems[i].EndSample, sizeof( m_EventItems[i].EndSample ), 1, m_File );		//	Point de fin
		fread( &EventTime, sizeof( EventTime ), 1, m_File );										//	Date de fin...
		m_EventItems[i].EndTime = StellateTimeToCTime( EventTime );									//	...au format 'time_t' C avec les mSec

		//	Lecture du montage / canal pour la version 1
		if( pItem->Version == 1 )
		{	fread( &pItem->Montage, sizeof( pItem->Montage ), 1, m_File );
			fread( &i16, sizeof( i16 ), 1, m_File ); pItem->MontageChannel = (EMontageChannel)i16;

			//	Si l'événement n'est pas sur tous les canaux : Lire son nom
			if( pItem->MontageChannel >= 0 )
			{	//	Si le montage d'enregistrement a été spécifié
				if( pItem->Montage == RECMTG_ID )
					m_EventItems[i].Channels.push_back( m_Montages[0].Channel[i16].ChannelName );
				else
				{	//	Si un montage reformaté a été spécifié
					i16 = pItem->Montage - REFMTG0_ID;	//	Index du montage
					i16++;								//	...incrémenté car '0' est le montage d'enregistrement

					//	Copie du nom
					m_EventItems[i].Channels.push_back( m_Montages[i16].Channel[pItem->MontageChannel].ChannelName );
				}
			}//	Sinon, le nom du canal reste vide

		}else	//	Pour les versions plus récentes : Le nom du canal est lu directement
		{	ReadText( &s1 );
			m_EventItems[i].Channels.push_back( s1 );
		}

		//	Nom et description
		ReadText( &m_EventItems[i].Name );
		ReadText( &m_EventItems[i].Description );

		//	Couleur et visibilité pour les versions >= 2
		if( pItem->Version >= 2 )
		{	fread( &pItem->Color, sizeof( pItem->Color ), 1, m_File );
			fread( &i32, sizeof( i32 ), 1, m_File );
			pItem->Visibility = (EVisibilityStatus)i32;
		}else
		{	//	Sinon, couleur comme le groupe
			pItem->Color = m_EventGroup.Group[m_EventItems[i].Group].Color;
			pItem->Visibility = StatusVisible;
		}

		//	ItemProperties selon les versions
		if( pItem->Version < 4 )
		{	//	Nombre d'ItemProperties
			fread( &ItemPropertyCount, sizeof( ItemPropertyCount ), 1, m_File );

			//	Ajustement de la taille du vecteur 'EITEM::ItemPropertyValue'
			if( ItemPropertyCount )
				pItem->ItemPropertyValue.resize( ItemPropertyCount );

			//	Lecture de chaque ItemProperty
			for( j = 0; j < ItemPropertyCount; j++ )
			{	ReadText( &s1 );
				pItem->ItemPropertyValue[j] = s1;
			}
		}else
		{	//	Rôle inconnu
			fread( &pItem->nSize, sizeof( pItem->nSize ), 1, m_File );

			//	Nombre d'ItemProperty
			fread( &i32, sizeof( i32 ), 1, m_File );
			ItemPropertyCount = (uint16_t)i32;

			//	Traitement s'il y a lieu
			if( i32 )
			{	//	Ajustement de la taille du vecteur 'EITEM::ItemPropertyValue'
				pItem->ItemPropertyValue.resize( i32 );

				//	Longueur totale des 'ItemProperties', avec les 'nullptr', et ajustement de 'ItemProperties'
				fread( &i32, sizeof( i32 ), 1, m_File );
				ItemProperties.resize( i32 );

				//	Lecture des ItemProperty : Chacune est terminée par un 'nullptr'
				ItemProperty = ItemProperties.data();
				fread( ItemProperty, sizeof( char ), i32, m_File );

				//	Recherche et copie de chaque ItemProperty
				for( j = 0; j < ItemPropertyCount; j++ )
				{	pItem->ItemPropertyValue[j] = ItemProperty;
					i16 = (short)strlen( ItemProperty ) + 1;
					ItemProperty += i16;
				}

				//	Libération de 'ItemProperties'
				ItemProperties.clear();
		}	}

		//	Séparateur entre les groupes SAUF POUR LE LE DERNIER
		if( i < Last )
			fread( &pItem->Separator, sizeof( pItem->Separator ), 1, m_File );
	}

	/*	Comme la lecture du séparateur 'pItem->Separateur' n'a pas été effectuée
		pour le dernier item, le séparateur précédent est copié dans le dernier.
		Ceci est essentiel car il sera possiblement déplacé par la fonction de tri.  */
	m_EventItem.Item[Last].Separator = m_EventItem.Item[Last-1].Separator;

	//	Retour
	return true;
}

//	Retourne l'index de la propriété de groupe 'Name' du groupe 'Group' ou -1 si la propriété n'a pas été trouvée
int CHarmonieFile::GetEventGroupPropertyIndex( int Group, const char *Name )
{
	size_t i;
	std::string Nom1, Nom2;

	//	Validation de 'Group'
	if( Group < 0 || Group > (int)m_EventGroup.Group.size() )
		return -1;

	//	Lecture du groupe
	EGROUP *Grp = &m_EventGroup.Group[Group];

	//	Propriété en minuscules dans 'Nom1'
	Nom1 = Name;
	std::transform( Nom1.begin(), Nom1.end(), Nom1.begin(), ::tolower );

	//	Recherche de la propriété 'Nom1'
	for( i = 0; i < Grp->GroupProperty.size(); i++ )	//	Pour chaque propriété
	{	Nom2 = Grp->GroupProperty[i].Key;				//	Lecture du nom...
		std::transform( Nom2.begin(), Nom2.end(), Nom2.begin(), ::tolower );
		if( Nom1 == Nom2 )								//	Si trouvé : ...
			return i;									//	Retourne l'index
	}

	//	Retourne -1 si la propriété n'a pas été trouvée
	return -1;
}

//	Retourne la valeur de la propriété 'Name' du groupe 'Group' ou nullptr si la propriété n'a pas été trouvée
const char *CHarmonieFile::GetEventGroupPropertyValue( int Group, const char *Name )
{
	EGROUP *Grp;

	//	Lecture de l'index
	int Index = GetEventGroupPropertyIndex( Group, Name );

	if( Index == -1 )									//	Index non valide : ...
		return nullptr;									//	...retourne nullptr
	else												//	Index valide : ...
	{	Grp = &m_EventGroup.Group[Group];				//	Lecture du groupe...
		return Grp->GroupProperty[Index].Value.c_str();	//	...et retour de la valeur
	}
}

//	Retourne l'index de la propriété d'événement 'Name' du groupe 'Group' ou -1 si la propriété n'a pas été trouvée
int CHarmonieFile::GetEventItemPropertyIndex( int Group, const char *Name )
{
	size_t i;
	std::string Nom1, Nom2;

	//	Validation de 'Group'
	if( Group < 0 || Group > (int)m_EventGroup.Group.size() )
		return -1;

	//	Lecture du groupe
	EGROUP *Grp = &m_EventGroup.Group[Group];

	//	Propriété en minuscules dans 'Nom1'
	Nom1 = Name;
	std::transform( Nom1.begin(), Nom1.end(), Nom1.begin(), ::tolower );

	//	Recherche de la propriété 'Nom1'
	for( i = 0; i < Grp->ItemProperty.size(); i++ )	//	Pour chaque propriété
	{	Nom2 = Grp->ItemProperty[i].Key;			//	Lecture du nom...
		std::transform( Nom2.begin(), Nom2.end(), Nom2.begin(), ::tolower );
		if( Nom1 == Nom2 )							//	Si trouvé : ...
			return i;								//	Retourne l'index
	}

	//	Retourne -1 si la propriété n'a pas été trouvée
	return -1;
}

//	Retourne la valeur de la propriété 'Index' de l'événement 'Item', ou nullptr si la propriété n'a pas été trouvée
const char *CHarmonieFile::GetEventItemPropertyValue( int Item, int Index )
{
	//	Validation de 'Item'
	if( Item < 0 || Item > (int)m_EventItem.Item.size() )
		return nullptr;

	//	Lecture de l'événement
	EITEM *Itm = &m_EventItem.Item[Item];

	//	Retourne la valeur de la propriété, ou nullptr si elle dépasse la plage valide
	if( Index >= 0 && Index < (int)Itm->ItemPropertyValue.size() )
		return Itm->ItemPropertyValue[Index].c_str();
	else
		return nullptr;
}

/*	Détermination des variables de groupes supplémentaires suivantes :
	m_SampSectGroup, m_StageGroup, m_StageProperty.  */
void CHarmonieFile::FindGroupVariables()
{
	size_t i;
	int s;
	EGROUP *pGroup;
	std::string txt;
	bool StdInit = false;

	//	Réinitialisation des variables
	m_SampSectGroup = UINT_MAX;
	m_StageGroup = UINT_MAX;
	m_StageProperty = UINT_MAX;
	for( i = 0; i < m_EventGroup.Group.size(); i++ )								//	Lecture...
	{	pGroup = &m_EventGroup.Group[i];											//	...de chaque groupe
		txt = m_EventGroups[i].Name;												//	Nom du groupe...
		std::transform( txt.begin(), txt.end(), txt.begin(), ::tolower );			//	...en minuscules
		//	Mémorisation de l'index du groupe contenant la section d'échantillon
        if( txt.find( "sample section" ) != std::string::npos ||
			(txt.find( "section d'" ) != std::string::npos && 
            txt.find( "chantillon." ) != std::string::npos))
			m_SampSectGroup = i;

		//	Mémorisation des variables pour le traitement des stades de sommeil
		if( pGroup->Type == GroupType_Stage && !StdInit )							//	Si c'est le groupe de stades
		{	m_StageGroup = i;														//	Mémorisation de l'index du groupe de stades
			txt = GetEventGroupPropertyValue( i, "EpochLength" );					//	Détermination...
			if( txt.size() ) m_LengthOfSleepEpochs = atoi( txt.c_str() );			//	...et copie de la longueur des époques de sommeil
			s = GetEventItemPropertyIndex( i, "Stage" );							//	Détermination de la propriété d'item déterminant le stade de sommeil
			if( s != -1 ) m_StageProperty = s;
			StdInit = true;															//	Initialisation complétée
	}	}

	/*	Assignation des variables 'CPSGFile::EVGROUP::SampleSectionGroup'
		et 'CPSGFile::EVGROUP::SleepStageGroup'  */
	for( i = 0; i < m_EventGroups.size(); i++ )
	{	m_EventGroups[i].SampleSectionGroup = i == m_SampSectGroup;
		m_EventGroups[i].SleepStageGroup = i == m_StageGroup;
	}
}

/*	Tri des événements par ordre chronologique.
	Note : Les variables d'événements sont divisés en 2 vecteurs d'objets :
		- CHarmonieFile::EVITEM dans CHarmonieFile::m_EventItem.Item
		- CPSGFile::EVITEM		dans CPSGFile::m_EventItems

	La procédure qui suit assure que les objets de ces 2 vecteurs soient synchronisés.
	Le vecteur 'CPSGFile::m_EventItems' est trié en premier,
	puis 'CHarmonieFile::m_EventItem.Item' pour le synchroniser au premier.  */
void CHarmonieFile::SortEventItems()
{
	size_t i;
	EITEM *pIt;

	/*	Sortie immédiate s'il n'y a pas d'événements, par exemple si la classe 'CHarmonieFile' avait
		été créée avec un argument 'ReadEvents' à 'false'.  */
	if( m_EventItems.size() == 0 ) return;

	//	Mémorisation de la version car est elle modifiée ci-dessous
	int16_t Version = m_EventItem.Item[0].Version;

	/*	Modification de la variable 'EITEM::Version' :
		- 0 pour le premier événement 'GroupType_Break' qui doit toujours demeurer le premier,
		- 1 si c'est un événement de stade,
		- 2 pour tous les autres.  */
	m_EventItem.Item[0].Version = 0;				//	Toujours 0 pour le premier événement
	for( i = 1; i < m_EventItems.size(); i++ )		//	Pour chaque événement subséquent
	{	if( m_EventItems[i].Group == m_StageGroup )	//	Si c'est un événement de stade : ...
			m_EventItem.Item[i].Version = 1;		//	...mise de 'Version' à 1
		else										//	Sinon : ...
			m_EventItem.Item[i].Version = 2;		//	...mise de 'Version' à 2
	}

	//	Assignation du pointeur 'CPSGFile::EVITEM::pPrivate' vers la variable 'CHarmonieFile::EITEM'
	for( i = 0; i < m_EventItems.size(); i++ )
		m_EventItems[i].pPrivate = &m_EventItem.Item[i];

	//	Tri du vecteur public d'éléments 'CPSGFile::EVITEM'
	std::sort( m_EventItems.begin(), m_EventItems.end(), SortPublicEventItemsFunction );

	//	Rétablissement de la version dans chaque événement
	for( i = 0; i < m_EventItem.Item.size(); i++ )
		m_EventItem.Item[i].Version = Version;

	//	Assignation de l'index des éléments triés 'CPSGFile::EVITEM' à la variable privée 'CHarmonieFile::EITEM::SortIndex'
	for( i = 0; i < m_EventItems.size(); i++ )
	{	pIt = (EITEM *)m_EventItems[i].pPrivate;
		pIt->SortIndex = i;
		m_EventItems[i].pPrivate = nullptr;
	}

	/*	Tri du vecteur d'éléments 'CHarmonieFile::m_EventItem.Item' selon 'EITEM::SortIndex'
		assigné ci-haut.  Les 2 vecteurs sont maintenant synchronisés.  */
	std::sort( m_EventItem.Item.begin(), m_EventItem.Item.end(), SortPrivateEventItemsFunction );
}

/*	Fonction de tri du vecteur public d'éléments 'CPSGFile::EVITEM', par ordre de:
		- StartSample
		- EITEM::Version (0 si c'est un 'GroupType_Break', 1 si c'est un événement de stade, 2 sinon)
		- Group
		- EndSample
		- Channel
		- Name
		- StartTime	*/
bool CHarmonieFile::SortPublicEventItemsFunction( const CPSGFile::EVITEM &p1, const CPSGFile::EVITEM &p2 )
{
	//	https://www.cplusplus.com/reference/algorithm/sort/
	//	The value returned indicates whether the element passed as first argument
	//	is considered to go before the second in the specific strict weak ordering it defines.

	int Compare;

	//	Premier tri sur StartSample
	if( p1.StartSample < p2.StartSample ) return true;
	else if( p1.StartSample > p2.StartSample ) return false;

	//	StartSample égaux.  Deuxième tri sur 'Version' (voir 'CHarmonieFile::SortEventItems')
	EITEM *pIt1 = (EITEM *)p1.pPrivate;
	EITEM *pIt2 = (EITEM *)p2.pPrivate;
	if( pIt1->Version < pIt2->Version ) return true;
	else if( pIt1->Version > pIt2->Version ) return false;

	//	Version égaux.  Troisième tri sur Group
	if( p1.Group < p2.Group ) return true;
	else if( p1.Group > p2.Group ) return false;

	//	Group égaux.  Quatrième tri sur EndSample
	if( p1.EndSample < p2.EndSample ) return true;
	else if( p1.EndSample > p2.EndSample ) return false;

	//	EndSample égaux.  Cinquième tri sur Channel
	if( p1.Channels.size() && p2.Channels.size() )
	{	Compare = strcmp( p1.Channels[0].c_str(), p2.Channels[0].c_str() );
		if( Compare < 0 ) return true;
		else if( Compare > 0 ) return false;
	}

	//	Channel égaux : Sixième tri sur 'Name'
	Compare = strcmp( p1.Name.c_str(), p2.Name.c_str() );
	if( Compare < 0 ) return true;
	else if( Compare > 0 ) return false;

	//	'Name' égaux.  Septième tri sur StartTime (pour les événements ajoutés dans 'CHarmonieFile::AddMissingEpochs'
	if( p1.StartTime < p2.StartTime ) return true;
	else if( p1.StartTime > p2.StartTime ) return false;

	//	In C++, comparator should return false if its arguments are equal
	return false;
}

//	Tri du vecteur d'éléments 'CHarmonieFile::m_EventItem.Item' selon 'EITEM::SortIndex'
bool CHarmonieFile::SortPrivateEventItemsFunction( const EITEM &p1, const EITEM &p2 )
{
	//	Premier tri sur StartSample
	if( p1.SortIndex < p2.SortIndex ) return true;
	else return false;
}

//	Construction du vecteur 'm_SampleSectionEvents' contenant les événements de section d'échantillon
void CHarmonieFile::BuildSampleSections()
{
	uint32_t i;

	m_SampleSectionEvents.clear();						//	Réinitialisation
	for( i = 0; i < m_EventItem.Item.size(); i++ )		//	Pour chaque événement : ...
	{	
        if( m_EventItems[i].Group == m_SampSectGroup ) {
			m_SampleSectionEvents.push_back( i );		//	... ajout au tableau 'SampleSections'
        }
	}
}

//	Construit le tableau de stades de sommeil 'm_EventItem.EpochEvent' contenant les événements d'époques de sommeil
void CHarmonieFile::BuildEpochEvents()
{
	uint32_t i;

	m_SleepEpochEvents.clear();								//	Réinitialisation
	for( i = 0; i < m_EventItems.size(); i++ )				//	Pour chaque événement : ...
	{	if( m_EventItems[i].Group == m_StageGroup )			//	... Si son groupe est 'm_StageGroup' : ...
			m_SleepEpochEvents.push_back( i );				//	... ajout au tableau 'EpochEvent'
	}
}

/*	Ajout d'époques de stade manquant correspondant aux discontinuités.
	Un nombre d'époque correspondant à la longueur de chaque discontinuité sont ajoutés.  */
void CHarmonieFile::AddMissingEpochs()
{
	uint32_t e, i, p, Count;
	EITEM EItem, *pEItem0;
	EVITEM EvItem, *pEvItem0, *pEvItem1;
	std::vector<EVITEM> vEvItem;
	std::string PrVal;
	double Diff, dLengthOfSleepEpoch, StartTime;

	//	La procédure suivante requiert au moins 2 époques
	if( m_SleepEpochEvents.size() < 2 ) return;

	//	Lecture des objets 'CHarmonieFile::EITEM' et 'CPSGFile::EVITEM' pour la première époque
	e = m_SleepEpochEvents[0];
	pEItem0 = &m_EventItem.Item[e];
	pEvItem0 = &m_EventItems[e];

	//	Initialisation des valeurs fixes de 'EvItem' et 'EItem'
	EItem.Version = pEItem0->Version;
	EItem.Montage = pEItem0->Montage;
	EItem.MontageChannel = pEItem0->MontageChannel;
	EItem.Color = pEItem0->Color;
	EItem.Visibility = pEItem0->Visibility;
	EItem.ItemPropertyValue.resize( pEItem0->ItemPropertyValue.size() );
	for( p = 0; p < pEItem0->ItemPropertyValue.size(); p++ )//	Copie des propriétés
	{	PrVal = pEItem0->ItemPropertyValue[p];				//	Lecture de la propriété
		if( p == m_StageProperty ) PrVal = "9";				//	Remplacement du stade par du stade 'Manquant'
		EItem.ItemPropertyValue[p] = PrVal;					//	Ajout de la propriété
	}
	EItem.Separator = pEItem0->Separator;
	EvItem.RealEvent = false;								//	Pas un événement réel
	EvItem.Group = pEvItem0->Group;
	EvItem.Channels = pEvItem0->Channels;
	EvItem.Name = "StdND";
	EvItem.Description = pEvItem0->Description;

	//	Longueur réelle d'une époque, en secondes
	dLengthOfSleepEpoch = pEvItem0->EndTime - pEvItem0->StartTime;

	//	Heure de survenue courante : Celle de la première époque
	StartTime = pEvItem0->StartTime;

	//	Examen des époques de sommeil à partir de la deuxième (la première a été lue ci-haut)
	for( i = 1; i < m_SleepEpochEvents.size(); i++ )
	{	e = m_SleepEpochEvents[i];								//	Lecture...
		pEvItem1 = &m_EventItems[e];							//	...de l'époque
		Diff = pEvItem1->StartTime - pEvItem0->StartTime;		//	Écart entre le début des 2 époques
		if( Diff < 0 ||											//	Certains anciens fichiers STS sont défectueux :  Des époques surviennent AVANT la précédente...
			Diff > PSG_SECJOUR )								//	...ou certaines heures d'époques sont corrompues, provoquant des sauts impossibles
				Diff = m_LengthOfSleepEpochs;
		Diff += 0.5; Diff = (int)Diff;							//	Arrondissement de l'écart
		Count = (uint32_t)Diff;									//	Nombre d'époques...
		Count /= m_LengthOfSleepEpochs;							//	...correspondant à l'écart
		for( e = 1; e < Count; e++ )							//	Pour chaque époque manquante
		{	StartTime += m_LengthOfSleepEpochs;					//	Incrémentation de son heure de survenue
			EvItem.StartSample = pEvItem1->StartSample - 1;		//	Le début est après la discontinuité
			EvItem.EndSample = EvItem.StartSample;				//	La fin est égale au début (pas de durée en points)
			EvItem.StartTime = StartTime;						//	Seconde de début
			EvItem.EndTime = StartTime + dLengthOfSleepEpoch;	//	Seconde de fin
			vEvItem.push_back( EvItem );						//	Ajout à 'vEvItem'
		}
		pEvItem0 = pEvItem1;									//	Mémorisation de l'événement, ...
		StartTime = pEvItem1->StartTime;						//	...et de sa position en secondes
	}

	//	Ajout des époques manquantes à 'm_EventItem.Item' et 'm_EventItems'
	for( i = 0; i < vEvItem.size(); i++ )
	{	m_EventItem.Item.push_back( EItem );
		m_EventItems.push_back( vEvItem[i] );
	}

	//	Nouveau traitement des éléments du vecteur
	if( vEvItem.size() )
	{	SortEventItems();
		BuildSampleSections();
		BuildEpochEvents();
	}
}

//	Assigne le stade de sommeil pour chaque événement dans la variable 'EVITEM::SleepStage'
void CHarmonieFile::AssignSleepStagesToEvents()
{
	uint32_t i;
	const char *tStage;

	EPSGSleepStage Stage = StageND;										//	Stade manquant au départ
	for( i = 0; i < m_EventItem.Item.size(); i++ )						//	Lecture...
	{	if( m_EventItems[i].Group == m_StageGroup )						//	Si son groupe est 'm_StageGroup'
		{	tStage = GetEventItemPropertyValue( i, m_StageProperty );	//	Copie du stade...
			Stage = (EPSGSleepStage)atoi( tStage );						//	...en 'EPSGSleepStage'
		}
		m_EventItems[i].SleepStage = Stage;								//	Copie du stade dans l'événement
	}
}

//	Calcule la durée des événements
void CHarmonieFile::ComputeEventsDuration()
{
	size_t i;
	unsigned int g;

	for( i = 0; i < m_EventItems.size(); i++ )
	{	g = m_EventItems[i].Group;
		if( m_EventGroup.Group[g].Extent == GroupExtent_Interval )
		{	m_EventItems[i].SampleLength = m_EventItems[i].EndSample - m_EventItems[i].StartSample + 1;
			m_EventItems[i].TimeLength = m_EventItems[i].EndTime - m_EventItems[i].StartTime;
		}else
		{	m_EventItems[i].SampleLength = 0;
			m_EventItems[i].TimeLength = 0.;
	}	}
}

//	Débute (true) ou termine (false) une série de 'AddEventGroup', 'DeleteEventGroup', 'AddEventItem', 'DeleteEventItem'
void CHarmonieFile::BeginGroupsEventEditing( bool Start )
{
	//	Retour immédiat si non applicable
	if( !Start && m_Editing == false ) return;

	if( Start )
	{	m_Editing = true;
		return;
	}

	//	Fin de la modification des événements
	m_Editing = false;

	//	Traitement après l'ajout des événements
	FindGroupVariables();
	SortEventItems();
	BuildSampleSections();
	BuildEpochEvents();
	AddMissingEpochs();
	AssignSleepStagesToEvents();
	ComputeEventsDuration();
}

/*	Ajout d'un groupe d'événements.
	Retourne l'index du nouveau groupe ajouté, ou 'UINT32_MAX' en cas d'erreur.  */
uint32_t CHarmonieFile::AddEventGroup( const char *Name, const char *Description, int32_t Color )
{
	size_t GrpIns, i, j;
	uint32_t Ret;
	uint16_t Sep;
	EGROUP EGroup;
	EVGROUP EvGroup;

	// Check if the group name already exist. 
	for (i=0; i < m_EventGroups.size(); i++) {
		if(m_EventGroups[i].Name.compare(Name) == 0) {
			return i;
		}
	}

	//	Retour immédiat si la fonction 'BeginGroupsEventEditing' n'a pas été exécutée
	if( !m_Editing ) return UINT32_MAX;

	//	Liste des identificateurs de groupes qui doivent être après le groupe à ajouter
	std::string GrApres[4] = {  "84ADC1B5-45A8-4264-9AB9-F4B33127019A",		// COM_GRPID_ARTIFACT
								"F4B8F3EE-8AC2-4EFC-BC86-2A55AF130A6C",		// COM_GRPID_STAGE
								"725824B1-4822-40CA-A017-F4B7E4D06D28",		// COM_GRPID_HYPERVENT
								"20E29563-73B4-4B07-BD29-1E7B724DE98D" };	// COM_GRPID_POSTHYPERVENT

	//	Détermination des groupes à venir après celui à ajouter
	GrpIns = SHRT_MAX;									//	Valeur impossible au départ
	for( i = 0; i < m_EventGroup.Group.size(); i++ )	//	Pour chaque groupe
	{	for( j = 0; j < 4; j++ )						//	Pour chaque identificateur
		{	if( m_EventGroup.Group[i].Id == GrApres[j] )//	Si le groupe est dans la liste
			{	GrpIns = std::min( GrpIns, i );			//	Assignation dans 'GrpIns'
				break;									//	Sortie de la boucle
		}	}
		if( GrpIns != SHRT_MAX ) break;					//	Sortie de la boucle s'il a été trouvé
	}


	//	Détermination du groupe des stades au cas où 'GrpIns' serait encore indéterminé
	if( m_StageGroup != UINT_MAX )
		GrpIns = std::min( GrpIns, (size_t)m_StageGroup );

	//	Initialisation de la structure 'CHarmonieFile::EGROUP' à ajouter
	if( m_EventGroup.Group.size() != 0 )
	{	EGroup.Version = m_EventGroup.Group[0].Version;
		EGroup.Separator = m_EventGroup.Group[0].Separator;
	}else
	{	EGroup.Version = 7;
		EGroup.Separator = 0x800d;
	}
	EGroup.Id = "AAAAAAAA-1796-405F-8D4A-BAFC8E385163";	//	COM_GRPID_USERDEFINED
	EGroup.Type = GroupType_Label;
	EGroup.Qualifier = GroupQualifier_UserSection;
	EGroup.Extent = GroupExtent_Instantaneous;			//	À corriger lors des exécutions de la fonction 'AddEventItem'
	EGroup.GroupChannel = GroupChannel_All;				//	À corriger lors des exécutions de la fonction 'AddEventItem'
	EGroup.Montage = 0;									//	Version < 6. À corriger lors des exécutions de la fonction 'AddEventItem'
	EGroup.MontageChannel = MontageChannel_All;			//	Version < 6. À corriger lors des exécutions de la fonction 'AddEventItem'
	EGroup.Color = Color;
	EGroup.GroupAccess = GroupAccess_DeletableEditableCreatable;
	EGroup.Visibility = StatusVisible;
	EGroup.UseNameFromList = 0;

	//	Initialisation de la structure 'CPSGFile::EVGROUP' à ajouter
	EvGroup.Name = Name;
	if( Description != nullptr) EvGroup.Description = Description;
	EvGroup.IsDeletable = true;
	EvGroup.SleepStageGroup = false;
	EvGroup.SampleSectionGroup = false;

	//	Ajout ou insertion du groupe aux vecteurs
	if( GrpIns == SHRT_MAX )														//	Ajout à la fin
	{	m_EventGroup.Group.push_back( EGroup );
		m_EventGroups.push_back( EvGroup );
		Ret = m_EventGroups.size() - 1;												//	Index du nouveau groupe créé
	}else																			//	Insertion
	{	m_EventGroup.Group.insert( m_EventGroup.Group.begin() + GrpIns, EGroup );
		m_EventGroups.insert( m_EventGroups.begin() + GrpIns, EvGroup );
		for( i = 0; i < m_EventItems.size(); i++ )									//	Incrémentation du groupe pour les événements affectés
		{	if( m_EventItems[i].Group >= GrpIns )
				m_EventItems[i].Group++;
		}
		Ret = GrpIns;																//	Index du nouveau groupe créé
	}

	//	Incrémentation du séparateur de tous les événements
	if( m_EventItem.Item.size() )
	{	Sep = m_EventItem.Item[0].Separator + 1;
		for( i = 0; i < m_EventItem.Item.size(); i++ )
			m_EventItem.Item[i].Separator = Sep;
	}

	//	Retourne l'index du groupe
	return Ret;
}

int CHarmonieFile::DeleteEventsByName(std::string eventName, uint32_t group_index) {
	// Return if not editing or if the group index is out of bound
	if( !m_Editing || group_index >= m_EventGroups.size() ) {
		return 0;
	}

	// Remove targeted events starting with the end of the vector
	int event_count = 0;
	int delete_count = 0;
	for (int i = (int32_t) m_EventItems.size() - 1; i >= 0; i--) {	
		if (m_EventItems[i].Group == group_index) {
			event_count++;
			if (m_EventItems[i].Name == eventName) {
				m_EventItem.Item.erase( m_EventItem.Item.begin() + i );
				m_EventItems.erase( m_EventItems.begin() + i );
				delete_count++;
			}	
		}
    }

	// If all events from the group were deleted, delete the empty group
	if (event_count == delete_count) {
		this->DeleteEventGroup(group_index);
	}
	return 1;
}

/*	Supprime le groupe 'Group' et tous ses événements.
	Demande une confirmation à l'usager si 'DisplayMessage'.
	Retourne la réponse de l'usager : wxYES ou wxNO.  */
int CHarmonieFile::DeleteEventGroup( uint32_t Group, bool Message )
{
	int32_t ei;
	uint32_t e;
	uint16_t Sep;

	//	Réponse de l'usager à l'effacement : Non par défaut
	int Ret = 0;

	//	Retour immédiat s'il y a lieu
	if( !m_Editing || Group >= m_EventGroups.size() ) {
		return Ret;
	}

	//	Retour immédiat si le groupe ne peut être effacé
	if( !m_EventGroups[Group].IsDeletable ) {
		return Ret;
	}

	//	Suppression des événements en commençant par la fin
	for( ei = (int32_t)m_EventItems.size() - 1; ei >= 0; ei-- )	{	
		if( m_EventItems[ei].Group == Group ) {	
			m_EventItem.Item.erase( m_EventItem.Item.begin() + ei );
			m_EventItems.erase( m_EventItems.begin() + ei );
		}	
	}

	//	Suppression des groupes
	m_EventGroup.Group.erase( m_EventGroup.Group.begin() + Group );
	m_EventGroups.erase( m_EventGroups.begin() + Group );

	/*	Décrémentation du séparateur de tous les événements
		et ajustement du groupe des événements  */
	if( m_EventItem.Item.size() ) {	
		Sep = m_EventItem.Item[0].Separator - 1;
		for( e = 0; e < m_EventItem.Item.size(); e++ ) {	
			m_EventItem.Item[e].Separator = Sep;
			if( m_EventItems[e].Group > Group ) m_EventItems[e].Group--;
		}	
	}

	//	Retour
	return Ret;
}

/*	Ajout d'un événement au groupe 'Group'.
	Retourne l'index du nouveau groupe ajouté, ou 'UINT32_MAX' en cas d'erreur.  */
uint32_t CHarmonieFile::AddEventItem( uint32_t Group, const char *Name, const char *Description, 
    uint32_t StartSample, uint32_t SampleLenght, double StartTime, double TimeLenght, const char *Channel )
{
	uint32_t EndSample;
	double EndTime;
	EITEM EItem;
	EVITEM EvItem;
	bool Ok;
	size_t i, j;
	std::string EvChannel, MtgChannel;

	//	Retour immédiat si la fonction 'BeginGroupsEventEditing' n'a pas été exécutée
	if( !m_Editing ) return UINT32_MAX;

	//	Correction de 'CHarmonieFile::EGROUP::Extent' qui a été défini à 'GroupExtent_Instantaneous' dans la fonction 'AddEventGroup'
	if( SampleLenght != 0 )
		m_EventGroup.Group[Group].Extent = GroupExtent_Interval;

	//	Correction des variables 'CHarmonieFile::EGROUP::GroupChannel', '::MontageChannel', '::Montage',
	if( Channel != nullptr )
	{	m_EventGroup.Group[Group].GroupChannel = GroupChannel_Any;
		m_EventGroup.Group[Group].MontageChannel = MontageChannel_Any;
	}

	//	Initialisation de la structure 'CHarmonieFile::EITEM' à ajouter
	if( m_EventItem.Item.size() != 0 )
	{	EItem.Version = m_EventItem.Item[0].Version;
		EItem.nSize = m_EventItem.Item[0].nSize;
		EItem.Separator = m_EventItem.Item[0].Separator;
	}else
	{	EItem.Version = 4;
		EItem.nSize = 1;
		EItem.Separator = 0x8010;
	}
	EItem.Montage = 0;							//	Corrigé ci-dessous s'il y a lieu
	EItem.MontageChannel = MontageChannel_All;	//	Corrigé ci-dessous s'il y a lieu
	EItem.Color = m_EventGroup.Group[Group].Color;
	EItem.Visibility = StatusVisible;

	//	Initialisation des variables de canal pour les anciennes versions
	if( EItem.Version < 3 && Channel != nullptr )
	{	EvChannel = Channel; CStdString::tolower( EvChannel );		//	Copie du canal de l'événement, en minuscules
		Ok = false;
		j = 0;														//	Pour éviter l'avertissement 'C4701: variable locale 'j' potentiellement non initialisée utilisée'
		for( i = 0; i < m_Montages.size(); i++ )					//	Recherche du canal 'EvChannel' dans tous les montages
		{	for( j = 0; j < m_Montages[i].Channel.size(); j++ )
			{	MtgChannel = m_Montages[i].Channel[j].ChannelName;
				CStdString::tolower( MtgChannel );
				if( MtgChannel == EvChannel )						//	Si trouvé
				{	Ok = true;
					break;
			}	}
			if( Ok ) break;
		}
		if( Ok )													//	Si trouvé...
		{	if( i == 0 )											//	...dans le montage d'enregistrement
				EItem.Montage = RECMTG_ID;
			else													//	...dans un montage reformaté
				EItem.Montage = REFMTG0_ID + i - 1;
			m_EventGroup.Group[Group].Montage = EItem.Montage;
			EItem.MontageChannel = (EMontageChannel)j;				//	Index du canal
		}else
			return UINT32_MAX;										//	Retour en erreur si non trouvé
	}

	//	Calcul des variables 'EndSample' et 'EndTime'
	if( SampleLenght == 0 )
	{	EndSample = StartSample;
		EndTime = StartTime;
	}else
	{	EndSample = StartSample + SampleLenght - 1;
		EndTime = StartTime + TimeLenght;
	}

	//	Initialisation de la structure 'CHarmonieFile::EITEM' à ajouter
	EvItem.RealEvent = true;
	EvItem.SleepStage = StageND;
	EvItem.Group = Group;
	EvItem.StartSample = StartSample;
	EvItem.EndSample = EndSample;
	EvItem.SampleLength = SampleLenght;
	EvItem.StartTime = StartTime;
	EvItem.EndTime = EndTime;
	EvItem.TimeLength = TimeLenght;
	if( Channel != nullptr ) EvItem.Channels.push_back( Channel );
	if( Name != nullptr ) EvItem.Name = Name;
	if( Description != nullptr ) EvItem.Description = Description;

	//	Ajout de l'événement aux vecteurs
	m_EventItem.Item.push_back( EItem );
	m_EventItems.push_back( EvItem );

	//	Retourne l'index de l'événement ajouté
	return m_EventItems.size() - 1;
}

//	Effacement de l'événement 'Item'
void CHarmonieFile::DeleteEventItem( uint32_t Item )
{
	//	Retour immédiat s'il y a lieu
	if( !m_Editing ||					//	La fonction 'BeginGroupsEventEditing' n'a pas été exécutée
		Item >= m_EventItems.size() )	//	'Item' invalide
			return;

	//	Effacement de l'événement
	m_EventItem.Item.erase( m_EventItem.Item.begin() + Item );
	m_EventItems.erase( m_EventItems.begin() + Item );
}

/*	Enregistrement dans le fichier 'pFileName'.
	S'il n'est pas spécifié, écrase le fichier existant.  */
bool CHarmonieFile::SaveFile( const char *pFileName )
{
	std::string STSFile, TmpFile, BakFile, extension;
	int16_t v;

	//	Valeur de retour par défaut
	bool Ret = false;

	//	Composition du nom du fichier STS de travail selon 'pFileName'
	if( pFileName == nullptr )												//	S'il faut écraser le fichier
	{
		// Get the extension of the file
		int position=m_FileInfo.FileName.find_last_of(".");
  		extension = m_FileInfo.FileName.substr(position+1);

		STSFile = ReplaceExtension( m_FileInfo.FileName.c_str(), "tmp" );	//	Le STS est d'abord créé dans un fichier temporaire
	} else {
		std::string filename = pFileName;
		int position=filename.find_last_of(".");
  		extension = filename.substr(position+1);
		STSFile = ReplaceExtension( pFileName, "sts" );						//	Nom du fichier STS associé à 'pFileName'
	}
		


	//	Ouverture du fichier et assignation du buffer
	m_File = fopen( STSFile.c_str(), "wb" );
	if( m_File == nullptr )
	{	BakFile = m_FileInfo.FileName;		//	La fonction 'DisplayMessage' utilise 'm_FileInfo.FileName'
		m_FileInfo.FileName = STSFile;
		//DisplayMessage( FileNotOpened );
		m_FileInfo.FileName = BakFile;		//	Remise de 'm_FileInfo.FileName'
		return Ret;
	}
	AssignFILEBuffer();

	//	Enregistrement de la version du fichier
	v = m_FileInfo.Version;
	fwrite( &v, sizeof( v ), 1, m_File );

	//	Enregistrement de chaque élément du fichier
	WriteFileHeader();
	WritePatientInfo();
	WriteCoDec();
	WriteCalibration();
	WritePhysicalMontage();
	WriteRecordingMontage();
	WriteReformatedMontages();
	WriteElecGroup();
	fwrite( &m_RecordCount, sizeof( m_RecordCount ), 1, m_File );	//	Nombre de records de 64 points (LONRECORD)
	WriteStatusGroup();
	WriteStatusItem();

	//	Fermeture du fichier
	fclose( m_File ); m_File = nullptr;

	//	Sauvegarde du fichier original s'il y a lieu
	if( pFileName == nullptr )
	{	TmpFile = STSFile;													//	Nom du fichier temporaire...
		STSFile = ReplaceExtension( m_FileInfo.FileName.c_str(), extension.c_str() );	//	...et du fichier STS associé
		BakFile = STSFile + ".bak";											//	Nom du fichier de sauvegarde
		std::remove( BakFile.c_str() );										//	Effacement du fichier de sauvegarde précédent
		std::rename( STSFile.c_str(), BakFile.c_str() );					//	Renommage du fichier original...
		std::rename( TmpFile.c_str(), STSFile.c_str() );					//	...et du nouveau fichier
	}

	//	Succès
	Ret = true;

	//	Retour
	return Ret;
}

//	Enregistrement de la classe 'CFileHeader'
void CHarmonieFile::WriteFileHeader()
{
	double CreationTime;

	//	Pointeur vers les sources locale et parente
	FILEHEADER *pl = &m_FileHeader;
	FILEINFO *pp = &m_FileInfo;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &pl->Separator, sizeof( pl->Separator ), 1, m_File );
	WriteClassName( pl->ClassName.c_str() );

	//	Enregistrement des informations communes à toutes les versions
	fwrite( &pl->Version, sizeof( pl->Version ), 1, m_File );		//	Version de la classe
	WriteText( pp->Description.c_str() );							//	Description
	WriteText( pp->Institution.c_str() );							//	Institution
	fwrite( &pl->FileType, sizeof( pl->FileType ), 1, m_File );		//	Type de fichier: EEG CSA etc...
	CreationTime = CTimeToStellateTime( pp->CreationTime );			//	CreationTime...
	fwrite( &CreationTime, sizeof( double ), 1, m_File );			//	au format Stellate

	//	Lecture des informations spécifiques à chaque version
	if( pl->Version >= 2 ) WriteText( pp->CreatedBy.c_str() );		//	CreatedBy
	if( pl->Version >= 3 ) WriteText( pp->LastModifiedBy.c_str() );	//	LastModifiedBy
}

//	Enregistrement de la classe 'CPatientInfo'
void CHarmonieFile::WritePatientInfo()
{
	double BirthDate;
	int16_t iSex;

	//	Pointeur vers la classe source
	PATIENTINFO *p = &m_PatientInfo;

	//	Pointeur vers la structure 'CPSGFile::PATIENTINFO' source
	CPSGFile::PATIENTINFO *pp = &((CPSGFile *)this)->m_PatientInfo;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement des informations communes à toutes les versions
	fwrite( &p->Version, sizeof( p->Version ), 1, m_File );
	WriteText( pp->Id1.c_str() );
	WriteText( pp->Id2.c_str() );
	WriteText( pp->LastName.c_str() );
	WriteText( pp->FirstName.c_str() );
	BirthDate = CTimeToStellateTime( pp->BirthDate );	//	Date de naissance...
	fwrite( &BirthDate, sizeof( double ), 1, m_File );	//	...au format Stellate
	if( pp->Gender == GenderMale )
		iSex = PatientSex_Male;
	else if( pp->Gender == GenderFemale )
		iSex = PatientSex_Female;
	else
		iSex = PatientSex_Unknown;
	fwrite( &iSex, sizeof( iSex ), 1, m_File );
	WriteText( pp->Address.c_str() );
	WriteText( pp->City.c_str() );
	WriteText( pp->State.c_str() );
	WriteText( pp->Country.c_str() );
	WriteText( pp->ZipCode.c_str() );
	WriteText( pp->HomePhone.c_str() );
	WriteText( pp->WorkPhone.c_str() );
	WriteText( pp->Comments.c_str() );

	//	Enregistrement des informations spécifiques à chaque version
	if( p->Version >= 2 )
	{	WriteText( pp->Height.c_str() );
		WriteText( pp->Weight.c_str() );
	}
	if( p->Version >= 3 )
	{	WriteText( p->UserFieldP1.c_str() );
		WriteText( p->UserFieldP2.c_str() );
		WriteText( p->UserFieldP3.c_str() );
		WriteText( p->UserFieldP4.c_str() );
		WriteText( p->UserFieldP5.c_str() );
	}
}

//	Enregistrement de la classe 'CCODEC'
void CHarmonieFile::WriteCoDec()
{
	int16_t NumChannels;
	int16_t *pSamplesChannel;
	uint16_t DosCoDecSize;
	int i;

	//	Pointeur vers la classe source
	CCODEC *p = &m_CoDec;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement des informations de base
	fwrite( &p->Version, sizeof( p->Version ), 1, m_File );
	NumChannels = p->SamplesChannel.size();
	fwrite( &NumChannels, sizeof( NumChannels ), 1, m_File );
	fwrite( &p->CompRecSize, sizeof( p->CompRecSize ), 1, m_File );
	fwrite( &p->SamplesRecord, sizeof( p->SamplesRecord ), 1, m_File );

	//	Enregistrement du vecteur 'SamplesChannel'
	pSamplesChannel = p->SamplesChannel.data();
	for( i = 0; i < NumChannels; i++ )
		fwrite( &pSamplesChannel[i], sizeof( int16_t ), 1, m_File );

	//	Enregistrement des données supplémentaires inconnues s'il y a lieu
	DosCoDecSize = p->DosCoDec.size();
	if( DosCoDecSize )
		fwrite( p->DosCoDec.data(), sizeof( char ), DosCoDecSize, m_File );
}

//	Enregistrement de la classe 'CCalibration'
void CHarmonieFile::WriteCalibration()
{
	uint16_t Count, i;

	//	Pointeur vers la classe source
	RECORDINGCALIBRATION *p = &m_RecordingCalibration;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement des informations de base
	fwrite( &p->Version, sizeof( p->Version ), 1, m_File );
	fwrite( &p->TrueSampleFrequency, sizeof( p->TrueSampleFrequency ), 1, m_File );
	fwrite( &p->BaseCalibrationMinADC, sizeof( p->BaseCalibrationMinADC ), 1, m_File );
	fwrite( &p->BaseCalibrationMinVolts, sizeof( p->BaseCalibrationMinVolts ), 1, m_File );
	fwrite( &p->BaseCalibrationMaxADC, sizeof( p->BaseCalibrationMaxADC ), 1, m_File );
	fwrite( &p->BaseCalibrationMaxVolts, sizeof( p->BaseCalibrationMaxVolts ), 1, m_File );
	Count = p->Channel.size();
	fwrite( &Count, sizeof( Count ), 1, m_File );

	//	Enregistrement des informations de chaque canal, dépendant des versions
	CHANNELCALIBRATION *pChannel = p->Channel.data();
	if( p->Version == 1 )
	{	for( i = 0; i < Count; i++ )
			fwrite( &pChannel[i].awUnit, sizeof( pChannel[i].awUnit ), 1, m_File );
	}
	for( i = 0; i < Count; i++ )
		fwrite( &pChannel[i].als1, sizeof( pChannel[i].als1 ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &pChannel[i].OutputMin, sizeof( pChannel[i].OutputMin ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &pChannel[i].InputMin, sizeof( pChannel[i].InputMin ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &pChannel[i].als2, sizeof( pChannel[i].als2 ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &pChannel[i].OutputMax, sizeof( pChannel[i].OutputMax ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &pChannel[i].InputMax, sizeof( pChannel[i].InputMax ), 1, m_File );

	//	Enregistrement des informations de position selon la version
	if( p->Version > 2 )
	{	Count = p->BodyPosition.size();
		fwrite( &Count, sizeof( Count ), 1, m_File );
		BODYPOSITION *pBodyPosition = p->BodyPosition.data();
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			fwrite( &pBodyPosition[i].BposThres, sizeof( pBodyPosition[i].BposThres ), 1, m_File );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			WriteText( pBodyPosition[i].BposKeys.c_str() );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			WriteText( pBodyPosition[i].BposNames.c_str() );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( i = 0; i < Count; i++ )
			fwrite( &pBodyPosition[i].BposUids, sizeof( pBodyPosition[i].BposUids ), 1, m_File );
	}
}

//	Enregistrement de la classe 'CPhysicalMontage'
void CHarmonieFile::WritePhysicalMontage()
{
	uint16_t Count, i;
	size_t s;

	//	Pointeur vers la classe source
	PHYSICALMONTAGE *p = &m_PhysicalMontage;

	//	Pointeur vers la structure 'CPSGFile::PATIENTINFO' source
	CPSGFile::PHYSICALMONTAGE *pp = &((CPSGFile *)this)->m_PhysicalMontage;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement des informations de base
	fwrite( &p->Version, sizeof( p->Version ), 1, m_File );
	WriteText( pp->MontageName.c_str() );
	Count = p->Electrode.size();
	fwrite( &Count, sizeof( Count ), 1, m_File );

	//	Enregistrement des noms d'électrodes
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		WriteText( pp->ElectrodeName[i].c_str() );

	//	Map Type
	i = p->MapType;
	fwrite( &i, sizeof( i ), 1, m_File );

	//	Nombre de positions d'électrodes (3 positions par électrode)
	fwrite( &p->ElectrodeCountX3, sizeof( p->ElectrodeCountX3 ), 1, m_File );

	//	Enregistrement des positions d'électrodes
	for( s = 0; s < p->Electrode.size(); s++ )
	{	fwrite( &p->Electrode[s].Coord1, sizeof( p->Electrode[s].Coord1 ), 1, m_File );
		fwrite( &p->Electrode[s].Coord2, sizeof( p->Electrode[s].Coord2 ), 1, m_File );
		fwrite( &p->Electrode[s].Coord3, sizeof( p->Electrode[s].Coord3 ), 1, m_File );
	}
}

//	Enregistrement de la classe 'CRecordingMontage'
void CHarmonieFile::WriteRecordingMontage()
{
	uint16_t Count, i, i16;

	//	Pointeur vers la classe de destination
	RECORDINGMONTAGE *p = &m_RecordingMontage;

	//	Nombre de montages d'enregistrement
	fwrite( &p->RecordingMontageCount, sizeof( p->RecordingMontageCount ), 1, m_File );

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement des informations de base
	fwrite( &p->Version, sizeof( p->Version ), 1, m_File );
	WriteText( m_Montages[0].MontageName.c_str() );
	Count = p->Channel.size();
	fwrite( &Count, sizeof( Count ), 1, m_File );

	//	Lecture des informations 'CHANNEL' des canaux
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &m_Montages[0].Channel[i].SampleFrequency, sizeof( m_Montages[0].Channel[i].SampleFrequency ), 1, m_File );
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
	{	i16 = EPSGChannelType2EChannelType( m_Montages[0].Channel[i].ChannelType );
		fwrite( &i16, sizeof( i16 ), 1, m_File );
	}
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
	{	i16 = EPSGChannelUnits2EChannelUnits( m_Montages[0].Channel[i].ChannelUnits );
		fwrite( &i16, sizeof( i16 ), 1, m_File );
	}
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &p->Channel[i].SpikeOptions, sizeof( p->Channel[i].SpikeOptions ), 1, m_File );
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &p->Channel[i].SeizureOptions, sizeof( p->Channel[i].SeizureOptions ), 1, m_File );
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &m_Montages[0].Channel[i].ChannelColor, sizeof( m_Montages[0].Channel[i].ChannelColor ), 1, m_File );

	//	Suite de l'enregistrement
	fwrite( &p->SamplesRecord, sizeof( p->SamplesRecord ), 1, m_File );
	fwrite( &m_BaseSampleFrequency, sizeof( m_BaseSampleFrequency ), 1, m_File );

	//	Enregistrement des noms d'électrodes pour chaque canal
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		WriteText( p->Channel[i].Electrode1Name.c_str() );
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		WriteText( p->Channel[i].Electrode2Name.c_str() );

	//	Variables de canaux inconnues
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		fwrite( &p->Channel[i].UnderSamplingMethod, sizeof( p->Channel[i].UnderSamplingMethod ), 1, m_File );
	fwrite( &Count, sizeof( Count ), 1, m_File );
	for( i = 0; i < Count; i++ )
		WriteText( p->Channel[i].Preprocessing.c_str() );
}

//	Enregistrement de la classe 'CReformatedMontage'
void CHarmonieFile::WriteReformatedMontages()
{
	uint16_t MtgCount, Last, Count, i, j, m, i16;

	//	Pointeur vers la classe de destination
	REFORMATEDMONTAGE *p = &m_ReformatedMontages;

	//	Nombre de montages reformatés
	MtgCount = p->Montage.size();
	fwrite( &MtgCount, sizeof( MtgCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu
	if( !MtgCount ) return;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement de chaque montage
	Last = MtgCount - 1;
	for( i = 0; i < MtgCount; i++ )
	{	//	Index du montage dans le vecteur 'm_Montages' (0 est le montage d'enregistrement)
		m = i + 1;

		//	Version et nom du montage
		fwrite( &p->Montage[i].Version, sizeof( p->Montage[i].Version ), 1, m_File );
		WriteText( m_Montages[m].MontageName.c_str() );

		//	Nombre de canaux
		Count = p->Montage[i].Channel.size();
		fwrite( &Count, sizeof( Count ), 1, m_File );

		//	Enregistrement des informations 'CHANNEL' des canaux
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fwrite( &m_Montages[m].Channel[j].SampleFrequency, sizeof( m_Montages[m].Channel[j].SampleFrequency ), 1, m_File );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
		{	i16 = EPSGChannelType2EChannelType( m_Montages[m].Channel[j].ChannelType );
			fwrite( &i16, sizeof( i16 ), 1, m_File );
		}
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
		{	i16 = EPSGChannelUnits2EChannelUnits( m_Montages[m].Channel[j].ChannelUnits );
			fwrite( &i16, sizeof( i16 ), 1, m_File );
		}
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fwrite( &p->Montage[i].Channel[j].SpikeOptions, sizeof( p->Montage[i].Channel[j].SpikeOptions ), 1, m_File );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fwrite( &p->Montage[i].Channel[j].SeizureOptions, sizeof( p->Montage[i].Channel[j].SeizureOptions ), 1, m_File );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			fwrite( &m_Montages[m].Channel[j].ChannelColor, sizeof( m_Montages[m].Channel[j].ChannelColor ), 1, m_File );

		//	Suite de l'enregistrement du montage (variable inconnue toujours égale à 12)
		fwrite( &p->Montage[i].Twelve, sizeof( p->Montage[i].Twelve ), 1, m_File );

		//	Enregistrement des informations supplémentaires 'REFCHANNEL' des canaux

		//	Enregistrement des noms et formules des canaux
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			WriteText( m_Montages[m].Channel[j].ChannelName.c_str() );
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			WriteText( p->Montage[i].Channel[j].ChannelFormula.c_str() );

		//	Traitement des macros s'il y a lieu
		Count = p->Montage[i].Macro.size();

		//	Enregistrement des noms de macros
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			WriteText( p->Montage[i].Macro[j].MacroLabel.c_str() );

		//	Enregistrement des formules des macros
		fwrite( &Count, sizeof( Count ), 1, m_File );
		for( j = 0; j < Count; j++ )
			WriteText( p->Montage[i].Macro[j].MacroFormula.c_str() );

		//	Séparateur entre les montages SAUF POUR LE DERNIER
		if( i < Last )
			fwrite( &p->Montage[i].Separator, sizeof( p->Montage[i].Separator ), 1, m_File );
	}
}

//	Enregistrement de la classe non documentée 'CElecGroup'
void CHarmonieFile::WriteElecGroup()
{
	uint16_t ElecGroupCount, ElectrodeCount;
	uint16_t i, j, LastGroup, LastElectrode;
	int32_t i32;
	ELGROUP *pElGr;

	//	Pointeur vers la classe de destination
	ELECTRODEGROUP *p = &m_ElectrodeGroup;
	p->First = true;

	//	Retour immédiat si la version de 'm_RecordingMontage' est inférieure à 5
	//	car la classe 'CElecGroup' n'existait pas avant
	if( m_RecordingMontage.Version < 5 ) return;

	//	Nombre de 'CElecGroup'
	ElecGroupCount = p->ElecGroup.size();
	fwrite( &ElecGroupCount, sizeof( ElecGroupCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu
	if( !ElecGroupCount ) return;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Lecture de chaque groupe d'électrode
	LastGroup = ElecGroupCount - 1;
	for( i = 0; i < ElecGroupCount; i++ )
	{	//	Pointeur vers le groupe d'électrodes 'ELGROUP'
		pElGr = &p->ElecGroup[i];

		//	Version et nom du groupe d'électrodes
		fwrite( &pElGr->Version, sizeof( pElGr->Version ), 1, m_File );
		WriteText( pElGr->GroupName.c_str() );

		//	Variables du groupe d'électrodes
		fwrite( &pElGr->ContactSize, sizeof( pElGr->ContactSize ), 1, m_File );
		fwrite( &pElGr->InterDistance, sizeof( pElGr->InterDistance ), 1, m_File );
		i32 = pElGr->Type; fwrite( &i32, sizeof( i32 ), 1, m_File );
		fwrite( &pElGr->NumCol, sizeof( pElGr->NumCol ), 1, m_File );
		fwrite( &pElGr->NumRow, sizeof( pElGr->NumRow ), 1, m_File );
		i32 = pElGr->RowOrColWise; fwrite( &i32, sizeof( i32 ), 1, m_File );
		fwrite( &pElGr->Color, sizeof( pElGr->Color ), 1, m_File );
		i32 = pElGr->OriginPoint; fwrite( &i32, sizeof( i32 ), 1, m_File );

		//	Nombre d'électrodes, allocation et ajustement de 'ELGROUP::Electrode'
		ElectrodeCount = pElGr->Electrode.size();
		fwrite( &ElectrodeCount, sizeof( ElectrodeCount ), 1, m_File );

		//	Séparateur SAUF POUR LE PREMIER GROUPE D'ÉLECTRODES
		if( i > 0 )
			fwrite( &pElGr->Separateur1, sizeof( pElGr->Separateur1 ), 1, m_File );

		//	Enregistrement de chaque électrode
		LastElectrode = ElectrodeCount - 1;
		for( j = 0; j < ElectrodeCount; j++ )
		{	WriteElectrode( &pElGr->Electrode[j] );

			//	Séparateur SAUF POUR LA DERNIÈRE ÉLECTRODE
			if( j < LastElectrode )
				fwrite( &pElGr->Separateur2, sizeof( pElGr->Separateur2 ), 1, m_File );
		}

		//	Séparateur SAUF POUR LE DERNIER GROUPE D'ÉLECTRODES
		if( i < LastGroup )
			fwrite( &pElGr->Separateur3, sizeof( pElGr->Separateur3 ), 1, m_File );
	}
}

//	Enregistrement de la classe non documentée 'CElectrode' dans 'pElectrode'
void CHarmonieFile::WriteElectrode( ELECTRODE *pElectrode )
{

	//	NOTE : Le nom de la classe n'est enregistré qu'une fois lors de la première lecture
	if( m_ElectrodeGroup.First )
	{	//	Enregistrement du séparateur et du nom de la classe
		fwrite( &pElectrode->Separator, sizeof( pElectrode->Separator ), 1, m_File );
		WriteClassName( pElectrode->ClassName.c_str() );

		m_ElectrodeGroup.First = false;
	}

	//	Enregistrement des variables de l'électrode
	fwrite( &pElectrode->Version, sizeof( pElectrode->Version ), 1, m_File );
	WriteText( pElectrode->ElecName.c_str() );
	fwrite( &pElectrode->PositionX, sizeof( pElectrode->PositionX ), 1, m_File );
	fwrite( &pElectrode->PositionY, sizeof( pElectrode->PositionY ), 1, m_File );
}

//	Enregistrement de la classe 'CStatusGroup'
void CHarmonieFile::WriteStatusGroup()
{
	uint16_t EventGroupCount, Last, DefaultItemCount;
	uint16_t ItemPropertyCount, GroupPropertyCount;
	int16_t i16;
	uint32_t i, j;
	EGROUP *pGroup;
	std::string DefName;

	//	Pointeur vers la classe de destination
	EVENTGROUP *p = &m_EventGroup;

	//	Nombre de groupes d'événements
	EventGroupCount = p->Group.size();
	fwrite( &EventGroupCount, sizeof( EventGroupCount ), 1, m_File );

	//	Sortie immédiate s'il y a lieu
	if( !EventGroupCount ) return;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Enregistrement de chaque groupe
	Last = EventGroupCount - 1;
	for( i = 0; i < EventGroupCount; i++ )
	{	//	Pointeur EGROUP;
		pGroup = &p->Group[i];

		//	Version de la classe
		fwrite( &pGroup->Version, sizeof( pGroup->Version ), 1, m_File );

		//	Id si la version > 2
		if( pGroup->Version > 2 ) WriteText( pGroup->Id.c_str() );

		//	Nom, description, Type, Qualifier, Extent
		WriteText( m_EventGroups[i].Name.c_str() );
		WriteText( m_EventGroups[i].Description.c_str() );
		j = pGroup->Type; fwrite( &j, sizeof( j ), 1, m_File );
		j = pGroup->Qualifier; fwrite( &j, sizeof( j ), 1, m_File );
		j = pGroup->Extent; fwrite( &j, sizeof( j ), 1, m_File );

		//	Lecture du montage et canal selon la version
		if( pGroup->Version < 4 )
		{	//	Version inférieure à 4

			//	Enregistrement du montage et canal
			fwrite( &pGroup->Montage, sizeof( pGroup->Montage ), 1, m_File );
			i16 = pGroup->MontageChannel; fwrite( &i16, sizeof( i16 ), 1, m_File );
		}else
		{	//	Versions >= 4, le nom du canal est directement lu
			j = pGroup->GroupChannel; fwrite( &j, sizeof( j ), 1, m_File );
			WriteText( pGroup->Channel.c_str() );
		}

		//	Couleur et accès
		fwrite( &pGroup->Color, sizeof( pGroup->Color ), 1, m_File );
		j = pGroup->GroupAccess; fwrite( &j, sizeof( j ), 1, m_File );

		//	Enregistrement des DefaultItemNames selon les versions
		if( pGroup->Version < 6 )
		{	//	Version inférieure à 6 : Il sont délimités par des virgules
			DefName.clear();
			for( j = 0; j < pGroup->DefaultItem.size(); j++ )
			{	DefName += pGroup->DefaultItem[j].Name;
				DefName += ',';
			}
			if( DefName.size() > 1 ) DefName.resize( DefName.size() - 1 );
			WriteText( DefName.c_str() );
		}else
		{	//	Version >= 6, les noms sont directement lus + 2 autres variables
			DefaultItemCount = pGroup->DefaultItem.size();
			fwrite( &DefaultItemCount, sizeof( DefaultItemCount ), 1, m_File );
			for( j = 0; j < DefaultItemCount; j++ )
				WriteText( pGroup->DefaultItem[j].Name.c_str() );
			fwrite( &DefaultItemCount, sizeof( DefaultItemCount ), 1, m_File );
			for( j = 0; j < DefaultItemCount; j++ )
				fwrite( &pGroup->DefaultItem[j].Color, sizeof( pGroup->DefaultItem[j].Color ), 1, m_File );
		}

		//	Enregistrement de Visibility qui n'est que dans les versions > 4
		if( pGroup->Version > 4 )
		{	j = pGroup->Visibility;
			fwrite( &j, sizeof( j ), 1, m_File );
		}

		//	Variable UseNameFromList pour les versions > 6
		if( pGroup->Version > 6 )
			fwrite( &pGroup->UseNameFromList, sizeof( pGroup->UseNameFromList ), 1, m_File );

		//	Traitement des ItemProperties
		ItemPropertyCount = pGroup->ItemProperty.size();
		fwrite( &ItemPropertyCount, sizeof( ItemPropertyCount ), 1, m_File );
		for( j = 0; j < ItemPropertyCount; j++ )
			WriteText( pGroup->ItemProperty[j].Key.c_str() );
		fwrite( &ItemPropertyCount, sizeof( ItemPropertyCount ), 1, m_File );
		for( j = 0; j < ItemPropertyCount; j++ )
			WriteText( pGroup->ItemProperty[j].Description.c_str() );

		//	Traitement des GroupProperties pour les versions > 1
		if( pGroup->Version > 1 )
		{	GroupPropertyCount = pGroup->GroupProperty.size();
			fwrite( &GroupPropertyCount, sizeof( GroupPropertyCount ), 1, m_File );
			for( j = 0; j < GroupPropertyCount; j++ )
				WriteText( pGroup->GroupProperty[j].Key.c_str() );
			fwrite( &GroupPropertyCount, sizeof( GroupPropertyCount ), 1, m_File );
			for( j = 0; j < GroupPropertyCount; j++ )
				WriteText( pGroup->GroupProperty[j].Description.c_str() );
			fwrite( &GroupPropertyCount, sizeof( GroupPropertyCount ), 1, m_File );
			for( j = 0; j < GroupPropertyCount; j++ )
				WriteText( pGroup->GroupProperty[j].Value.c_str() );
		}

		//	Séparateur entre les groupes SAUF POUR LE DERNIER
		if( i < Last )
			fwrite( &pGroup->Separator, sizeof( pGroup->Separator ), 1, m_File );
	}
}

//	Enregistrement de la classe 'CStatusItem'
void CHarmonieFile::WriteStatusItem()
{
	uint32_t EventItemCount;
	uint16_t ItemPropertyCount;
	uint32_t i, i32, Last;
	uint16_t Grp0, j;
	int16_t i16;
	char cnull;
	double EventTime;
	EITEM *pItem;

	//	Pointeur vers la classe de destination
	EVENTITEM *p = &m_EventItem;

	//	Décompte du nombre d'événements réels et détermination du dernier événement réel
	EventItemCount = 0; Last = 0;
	for( i = 0; i < m_EventItems.size(); i++ )
	{	if( m_EventItems[i].RealEvent == true )
		{	EventItemCount++;
			Last = i;
	}	}

	//	Nombre d'événements
	if( EventItemCount > 0xffff )
	{	j = 0xffff;
		fwrite( &j, sizeof( j ), 1, m_File );
		fwrite( &EventItemCount, sizeof( EventItemCount ), 1, m_File );
	}else
	{	j = (uint16_t)EventItemCount;
		fwrite( &j, sizeof( j ), 1, m_File );
	}

	//	Sortie immédiate s'il y a lieu
	if( !EventItemCount ) return;

	//	Enregistrement du séparateur et du nom de la classe
	fwrite( &p->Separator, sizeof( p->Separator ), 1, m_File );
	WriteClassName( p->ClassName.c_str() );

	//	Détermination de l'identificateur du groupe 0, basé sur le séparateur de groupes
	Grp0 = ( m_EventGroup.Group[0].Separator & 0x00ff ) + 1;

	//	Enregistrement de chaque événement réel
	for( i = 0; i < m_EventItems.size(); i++ )
	{	if( m_EventItems[i].RealEvent )
		{	//	Pointeur vers l'événement
			pItem = &p->Item[i];

			//	Version de la classe
			fwrite( &pItem->Version, sizeof( pItem->Version ), 1, m_File );

			//	Groupe
			i16 = m_EventItems[i].Group;
			i16 += Grp0;
			fwrite( &i16, sizeof( i16 ), 1, m_File );

			//	Positions / heures
			fwrite( &m_EventItems[i].StartSample, sizeof( m_EventItems[i].StartSample ), 1, m_File );	//	Point de début
			EventTime = CTimeToStellateTime( m_EventItems[i].StartTime );								//	Date de début...
			fwrite( &EventTime, sizeof( EventTime ), 1, m_File );										//	...au format Stellate
			fwrite( &m_EventItems[i].EndSample, sizeof( m_EventItems[i].EndSample ), 1, m_File );		//	Point de fin
			EventTime = CTimeToStellateTime( m_EventItems[i].EndTime );									//	Date de fin...
			fwrite( &EventTime, sizeof( EventTime ), 1, m_File );										//	...au format Stellate

			//	Montage / canal pour la version 1
			if( pItem->Version == 1 )
			{	fwrite( &pItem->Montage, sizeof( pItem->Montage ), 1, m_File );
				i16 = pItem->MontageChannel;
				fwrite( &i16, sizeof( i16 ), 1, m_File );
			}else
				//	Pour les versions plus récentes : Le nom du canal est lu directement
				WriteText( m_EventItems[i].Channels[0].c_str() );

			//	Nom et description
			WriteText( m_EventItems[i].Name.c_str() );
			WriteText( m_EventItems[i].Description.c_str() );

			//	Couleur et visibilité pour les versions >= 2
			if( pItem->Version >= 2 )
			{	fwrite( &pItem->Color, sizeof( pItem->Color ), 1, m_File );
				i32 = pItem->Visibility;
				fwrite( &i32, sizeof( i32 ), 1, m_File  );
			}

			//	ItemProperties selon les versions
			ItemPropertyCount = pItem->ItemPropertyValue.size();	//	Nombre d'ItemProperties

			if( m_EventItems[i].Group == m_StageGroup )						//	Si son groupe est 'm_StageGroup'
			{	
				pItem->ItemPropertyValue[m_StageProperty] = '0' + (int)m_EventItems[i].SleepStage;
			}

			if( pItem->Version < 4 )
			{	
				fwrite( &ItemPropertyCount, sizeof( ItemPropertyCount ), 1, m_File );

				//	Enregistrement de chaque ItemProperty
				for( j = 0; j < ItemPropertyCount; j++ )
					WriteText( pItem->ItemPropertyValue[j].c_str() );
			}else
			{	//	Rôle inconnu
				fwrite( &pItem->nSize, sizeof( pItem->nSize ), 1, m_File );

				//	Nombre d'ItemProperty
				i32 = ItemPropertyCount;
				fwrite( &i32, sizeof( i32 ), 1, m_File );

				//	Traitement s'il y a lieu
				if( i32 )
				{	//	Longueur totale des 'ItemProperties', avec les 'nullptr'
					i32 = 0;
					for( j = 0; j < ItemPropertyCount; j++ )
					{	i32 += pItem->ItemPropertyValue[j].length();
						i32++;
					}
					fwrite( &i32, sizeof( i32 ), 1, m_File );

					//	Enregistrement des ItemProperty : Chacune est terminée par un 'nullptr'
					cnull = 0;
					for( j = 0; j < ItemPropertyCount; j++ )
					{	fwrite( pItem->ItemPropertyValue[j].c_str(), sizeof( char ), pItem->ItemPropertyValue[j].size(), m_File );
						fwrite( &cnull, sizeof( cnull ), 1, m_File );
			}	}	}

			//	Séparateur entre les groupes SAUF POUR LE LE DERNIER
			if( i < Last )
				fwrite( &pItem->Separator, sizeof( pItem->Separator ), 1, m_File );
	}	}
}

std::string getExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        return filename.substr(dotPos + 1);
    }
    return "";
}

std::string CHarmonieFile::FindSigExtensionFile(std::string filename) {
	size_t dotPos = filename.find_last_of('.');

	if (dotPos == std::string::npos) {
        return "";
    }
	std::string sts_ext = filename.substr(dotPos + 1);
	std::string sig_filename;
	std::string sig_filename_lower = ReplaceExtension( filename.c_str(), "sig" );
	std::string sig_filename_upper = ReplaceExtension( filename.c_str(), "SIG" );
	if (sts_ext == "sts") {
		if (std::ifstream(sig_filename_lower)) {
			sig_filename = sig_filename_lower;
		} else if (std::ifstream(sig_filename_upper)){
			sig_filename = sig_filename_upper;
		} else {
			return "";
		}
	} else if (sts_ext == "STS"){
		if (std::ifstream(sig_filename_upper)) {
			sig_filename = sig_filename_upper;
		} else if (std::ifstream(sig_filename_lower)){
			sig_filename = sig_filename_lower;
		} else {
			return "";
		}
	} else {
		return "";
	}
	return sig_filename;
}

void CHarmonieFile::EndDataReading()
{
	if( m_SigFile != nullptr ) {
		fclose( m_SigFile );
		m_SigFile = nullptr;
	}
}

/*	Initialisation de la lecture des données avec le montage 'MontageIndex'.
	'MontageIndex' doit correspondre à un des montages définis dans 'CPSGFile::m_Montages'.
	Retourne 'true' si succès, 'false' en cas d'erreur.  */
bool CHarmonieFile::InitDataReading( int MontageIndex )
{
	std::string Txt1, Txt2;
	std::string Txt3;
	size_t i, j, Frq, Pos;
	int16_t *pData;
	REFMONTAGE *pRefMtg;

	//	Retour immédiat si 'MontageIndex' est invalide
	if( MontageIndex < 0 || MontageIndex > (int)m_Montages.size() - 1 ) return false;

	// Check which sig file is aviable.
	std::string sig_filename;
	sig_filename = FindSigExtensionFile(m_FileInfo.FileName);
	if (sig_filename == "") {
		return false;
	}

	//	Ouverture du fichier et assignation du buffer de lecture
	m_Data.FileName = sig_filename;
	if( m_SigFile != nullptr ) fclose( m_SigFile );
	m_SigFile = fopen( sig_filename.c_str(), "rb" );
	if( m_SigFile == nullptr )
	{	
		return false;
	}
	AssignFILEBuffer();

	//	Copie de la taille du fichier
	fseek( m_SigFile, 0, SEEK_END );
	fgetpos( m_SigFile, &m_Data.FileSize );
	rewind( m_SigFile );

	//	Effacement des vecteurs précédents
	m_Data.RecordingMontage.clear();
	m_Data.UserMontage.clear();
	m_Data.RawData.clear();

	//	Allocation des vecteurs 'RecordingMontage' et 'CurrentData' pour le montage d'enregistrement
	m_Data.RecordingMontage.resize( m_Montages[0].Channel.size() );

	//	Calcul de la longueur d'un record de données (de 'LONRECORD' points)
	m_Data.RecordLength = 0;
	for( i = 0; i < m_Montages[0].Channel.size(); i++ )		//	Pour chaque canal du montage d'enregistrement :...
	{	Frq = m_Montages[0].Channel[i].SampleFrequency;		//	Fréquence
		m_Data.RecordLength += Frq;							//	Incrémentation du nombre de points pour 1 seconde de données
		m_Data.RecordingMontage[i].SamplesPerRecord = Frq;	//	Copie de la fréquence (sera modifié ci-dessous)
	}
	m_Data.RecordLength *= RECORDLENGTH;					//	Longueur du record ...
	m_Data.RecordLength /= m_BaseSampleFrequency;			//	...pour 'LONRECORD' points
	m_Data.RecordLength *= sizeof( uint16_t );				//	Chaque donnée brute occupe 2 octets (uint16_t)
	m_Data.RecordLength += sizeof( double );				//	Chaque record commence par sa date/heure

	//	Allocation du tampon de données 'RawData' pour la lecture d'un record
	m_Data.RawData.resize( m_Data.RecordLength );	//	Vecteur 'char'...
	m_Data.pRawData = m_Data.RawData.data();		//	...et son pointeur
	m_Data.pRecordDate = (double *)m_Data.pRawData;	//	Pointeur de la date/heure du record

	/*	Pour chaque canal du montage d'enregistrement :
		- Calcul du nombre de points par record
		- Calcul du pointeur 'pRawData' vers les données de ce canal dans 'm_Data.RawData'  */
	pData = (int16_t *)( m_Data.pRawData + sizeof( double ) );					//	Pointeur vers les données du premier canal (après la date/heure) dans les données du record
	for( i = 0; i < m_Data.RecordingMontage.size(); i++ )						//	Pour chaque canal
	{	m_Data.RecordingMontage[i].SamplesPerRecord *= RECORDLENGTH;			//	Nombre de données...
		m_Data.RecordingMontage[i].SamplesPerRecord /= m_BaseSampleFrequency;	//	...pour un record de 'LONRECORD' points
		m_Data.RecordingMontage[i].pRawData = pData;							//	Copie du pointeur vers les données de ce canal dans 'm_Data.RawData'
		pData += m_Data.RecordingMontage[i].SamplesPerRecord;					//	Incrémentation de 'pData'...
	}

	//	Calcul de la facteurs de la formule de conversion de ADC en Volts
	m_Data.Raw2Volt_A = ( (double)m_RecordingCalibration.BaseCalibrationMaxVolts -
						  (double)m_RecordingCalibration.BaseCalibrationMinVolts ) /
						( (double)m_RecordingCalibration.BaseCalibrationMaxADC -
						  (double)m_RecordingCalibration.BaseCalibrationMinADC );
	m_Data.Raw2Volt_B = (double)m_RecordingCalibration.BaseCalibrationMinVolts -
						m_Data.Raw2Volt_A * (double)m_RecordingCalibration.BaseCalibrationMinADC;

	//	Calcul des facteurs des formules de conversion de Volts en unités réelles pour chaque canal du montage d'enregistrement
	for( i = 0; i < m_Data.RecordingMontage.size(); i++ )
	{	m_Data.RecordingMontage[i].Volt2Unit_A = ( (double)m_RecordingCalibration.Channel[i].InputMax -
													(double)m_RecordingCalibration.Channel[i].InputMin ) /
												 ( (double)m_RecordingCalibration.Channel[i].OutputMax -
													(double)m_RecordingCalibration.Channel[i].OutputMin );
		m_Data.RecordingMontage[i].Volt2Unit_B = (double)m_RecordingCalibration.Channel[i].InputMin -
												 m_Data.RecordingMontage[i].Volt2Unit_A *
												 (double)m_RecordingCalibration.Channel[i].OutputMin;
	}

	//	Allocation et traitement du vecteur 'DATAUSERMONTAGE' pour le montage à utiliser
	m_Data.UserMontage.resize( m_Montages[MontageIndex].Channel.size() );

	//	Traitement des canaux selon le montage à utiliser
	if( MontageIndex == 0 )																							//	Si c'est le montage d'enregistrement
	{	for( i = 0; i < m_RecordingMontage.Channel.size(); i++ )													//	Pour chaque canal
		{	m_Data.UserMontage[i].Formula.AddVariable( m_RecordingMontage.Channel[i].Electrode1Name.c_str(),
													   &m_Data.RecordingMontage[i].Data );							//	La formule 'CMathExpression' est le nom du canal
			m_Data.UserMontage[i].Formula.ParseExpression( m_RecordingMontage.Channel[i].Electrode1Name.c_str() );
			m_Data.UserMontage[i].SamplesPerRecord = m_Data.RecordingMontage[i].SamplesPerRecord;					//	Nombre de points par record
		}
	}else																											//	Si c'est un montage reformaté
	{	pRefMtg = &m_ReformatedMontages.Montage[MontageIndex-1];													//	Pointeur 'REFMONTAGE' vers le montage
		for( i = 0; i < pRefMtg->Channel.size(); i++ )																//	Pour chaque canal
		{	/*	Les formules des montages reformatés utilisent toujours le nom 'Electrode1Name' du montage d'enregistrement.
				Pour chaque canal, le nom de chaque canal du montage d'ENREGISTREMENT est ajouté à 'CMathExpression',
				puis la formule de chaque canal REFORMATTÉ est évaluée.
				NOTE : Harmonie permet (malgré un message d'avertissement) de créer un montage d'enregistrement
					avec plusieurs canaux utiliant la même électrode (ex.: ECG1-ECG2, ECG2-ECG3, ECG2-ECG3).
					L'ajout du nom des canaux du montage d'enregistrement par la fonction 'AddVariable' ci-dessous
					se fait par conséquent avec l'argument 'false' pour prévenir l'affichage d'un avertissement
					par la classe 'CMathExpression'.  */
			for( j = 0; j < m_RecordingMontage.Channel.size(); j++ )												//	Ajout du nom de chaque canal...
				m_Data.UserMontage[i].Formula.AddVariable( m_RecordingMontage.Channel[j].Electrode1Name.c_str(),	//	...du montage d'ENREGISTREMENT...
														   &m_Data.RecordingMontage[j].Data, false );				//	...et de la variable contenant sa valeur

			//	Remplacement des macros par leurs formules
			Txt1 = pRefMtg->Channel[i].ChannelFormula;										//	Copie de la formule du canal...
			CStdString::toupper( Txt1 );													//	...en majuscules
			for( j = 0; j < pRefMtg->Macro.size(); j++ )									//	Pour chaque macro de ce montage
			{	Txt2 = pRefMtg->Macro[j].MacroLabel;										//	Nom de la macro...
				CStdString::toupper( Txt2 );												//	...en majuscules
				while( ( Pos = Txt1.find( Txt2 ) ) != std::string::npos )					//	Si la macro est dans la formule du canal
				{	
                    //Txt3.Printf( "%s(%s)%s", Txt1.substr( 0,  Pos ).c_str(),				//	Partie à gauche de la macro...
					//						 pRefMtg->Macro[j].MacroFormula.c_str(),		//	... formule de la macro entre parentèses...
					//						 Txt1.substr( Pos + Txt2.length() ).c_str() );	//	... partie à droite de la macro
                    Txt3 = Txt1.substr( 0,  Pos ) + "(" + pRefMtg->Macro[j].MacroFormula + ")" + Txt1.substr( Pos + Txt2.length() );
					Txt1 = Txt3;															//	Copie dans la formule
			}	}
			m_Data.UserMontage[i].Formula.ParseExpression( Txt1.c_str() );					//	Évaluation de la formule finale

			Frq = m_Montages[MontageIndex].Channel[i].SampleFrequency;						//	Nombre de points par record...
			m_Data.UserMontage[i].SamplesPerRecord = Frq * RECORDLENGTH /
													 m_BaseSampleFrequency;					//	... pour ce canal
	}	}

	//	Tous les résulats seront copiés dans 'm_Montage.Result'
	for( i = 0; i < m_Data.UserMontage.size(); i++ )
		m_Data.UserMontage[i].Formula.AssignAnswerAddress( &m_Data.Result );

	//	Calcul du nombre de secondes par point
	m_Data.SampleDuration = 1. / m_RecordingCalibration.TrueSampleFrequency;

	//	Retour
	return true;
}

/*	Positionne le fichier au début du record contenant le 'SamplePoint'.
	Retourne 'true' en cas de succès, 'false' en cas d'erreur.  */
bool CHarmonieFile::SeekToRecord( unsigned long SamplePoint )
{
	int64_t Record, Position, FileSize;
	fpos_t Pos;

	/*	Retour immédiat si le tampon de données est nul,
		si la fonction 'InitDataReading' n'a pas été exécutée.  */
	if( m_Data.RawData.size() == 0 ) return false;

	//	Calcul du record contenant 'SamplePoint' et positionnement du fichier au début de ce record
	Record = (int64_t)SamplePoint / (int64_t)RECORDLENGTH;	//	Record contenant le 'Point'
	Position = Record * (int64_t)m_Data.RecordLength;		//	Position dans le fichier

	//	Copie de la taille du fichier dans 'FileSize' selon 'CEAMS_WINDOWS' et 'CEAMS_APPLE'
#if defined(CEAMS_WINDOWS) || defined(CEAMS_APPLE)
	FileSize = m_Data.FileSize;
#else
	FileSize = m_Data.FileSize.__pos;
#endif

	//	Message et retour en cas d'erreur
	if( Position > FileSize )
	{	m_FileInfo.FileName.swap( m_Data.FileName );
		//DisplayMessage( FileSeekError );
		m_FileInfo.FileName.swap( m_Data.FileName );
		return false;
	}

	/*	Selon http://www.cplusplus.com/reference/cstdio/fsetpos/
		l'argument 'fpos_t *Pos' est un pointeur vers un objets 'fpos_t'
		précédemment obtenu par la fonction 'fgetpos'.  */
	fgetpos( m_SigFile, &Pos );

	//	Copie de la position dans 'Pos' selon 'CEAMS_WINDOWS' et 'CEAMS_APPLE'
#if defined(CEAMS_WINDOWS) || defined(CEAMS_APPLE)
	Pos = Position;
#else
	Pos.__pos = Position;
#endif

	//	Positionnement dans le fichier
	fsetpos( m_SigFile, &Pos );

	//	Succès
	return true;
}

//	Retourne le nombre de records de données
unsigned int CHarmonieFile::GetRecordCount()
{
	return m_RecordCount;
}

/*	Lit le record contenant le point 'SamplePoint',
	ou lit le record suivant si 'SamplePoint' n'est pas précisé.

	Retourne la position de 'SamplePoint' dans le record,
	À LA FRÉQUENCE D'ENREGISTREMENT DE BASE,
	où 'UINT_MAX' en cas d'erreur.  */
unsigned int CHarmonieFile::ReadRecord( unsigned long SamplePoint )
{
	long Position;

	/*	Retour immédiat si le tampon de données est nul,
		si la fonction 'InitDataReading' n'a pas été exécutée.  */
	if( m_Data.RawData.size() == 0 ) return UINT_MAX;

	//	Positionnement au record contenant 'SamplePoint' s'il y a lieu
	if( SamplePoint != ULONG_MAX )
	{	if( SeekToRecord( SamplePoint ) == false )
			return UINT_MAX;
	}

	//	Calcul du point au début du record courant
	Position = ftell( m_SigFile );			//	Position réelle dans le fichier...
	Position /= m_Data.RecordLength;	//	...en numéro de record...
	Position *= RECORDLENGTH;			//	...en point

	//	Lecture du record dans le fichier
	if( fread( m_Data.pRawData,	sizeof( char ),	m_Data.RecordLength, m_SigFile ) !=
		(size_t)m_Data.RecordLength )
			return UINT_MAX;

	//	Retourne la position du point dans le record
	if( SamplePoint == ULONG_MAX )
		return 0;						//	Aucun point demandé : Position 0 (début du record)
	else
		return SamplePoint - Position;	//	Position du point demandé dans le record
}

/*	Retourne le nombre de points d'échantillon dans un record de données pour le canal 'Channel',
	pour le montage utilisé par la fonction 'InitDataReading'.  */
unsigned int CHarmonieFile::GetRecordSampleCountForChannel( int Channel )
{
	/*	Retour immédiat si le tampon de données est nul,
		si la fonction 'InitDataReading' n'a pas été exécutée.  */
	if( m_Data.RawData.size() == 0 ) return UINT_MAX;

	return m_Data.UserMontage[Channel].SamplesPerRecord;
}

/*	Retourne la position de 'SamplePoint' pour le canal 'Channel'
	dans le record lu par la fonction 'ReadRecord'.  */
unsigned int CHarmonieFile::GetRecordPointForChannel( unsigned int SamplePoint, int Channel )
{
	return SamplePoint * m_Data.UserMontage[Channel].SamplesPerRecord / RECORDLENGTH;
}

//	Retourne l'heure du point 'SamplePoint' du record lu par la fonction 'ReadRecord'
double CHarmonieFile::GetTimeOfPoint( unsigned int SamplePoint )
{
	double Second, TimeOfPoint;

	Second = (double)SamplePoint * m_Data.SampleDuration;	//	Temps de 'SamplePoint' dans le record courant...
	TimeOfPoint = *m_Data.pRecordDate + Second;				//	...ajouté à l'heure du record...
	return StellateTimeToCTime( TimeOfPoint );				//	...convertit au format standard C (time_t), avec les mSec
}

/*	Retourne la valeur brute du 'SamplePoint' pour le canal 'Channel' du record courant,
	ou 'DBL_MAX' si 'Channel' est invalide.  */
double CHarmonieFile::GetDataRaw( int Channel, int SamplePoint )
{
	size_t i;

	//	Retour immédiat si 'Channel' est invalide
	if( Channel > (int)m_Data.UserMontage.size() - 1 ) return DBL_MAX;

	/*	Copie des données brutes du montage d'enregistrement
		dans les variables de travail utilisée par les instances 'CMathExpression'  */
	for( i = 0; i < m_Data.RecordingMontage.size(); i++ )
		m_Data.RecordingMontage[i].Data = m_Data.RecordingMontage[i].pRawData[SamplePoint];

	//	Calcul de la donnée résultante
	m_Data.UserMontage[Channel].Formula.Compute();

	//	Retour de la variable calculée
	return m_Data.Result;
}

/*	Retourne la valeur en unités réelles du 'Point' pour le canal 'Channel' du record courant
	ou 'DBL_MAX' si 'Channel' est invalide.  */
double CHarmonieFile::GetDataUnits( int Channel, unsigned int SamplePoint )
{
	size_t i;
	double Volts, Units;

	//	Retour immédiat si 'Channel' est invalide
	if( Channel > (int)m_Data.UserMontage.size() - 1 ) return DBL_MAX;

	/*	Copie des données brutes du montage d'enregistrement
		dans les variables de travail utilisée par les instances 'CMathExpression'  */
	for( i = 0; i < m_Data.RecordingMontage.size(); i++ )
	{	Volts = m_Data.RecordingMontage[i].pRawData[SamplePoint];
		Volts = Volts * m_Data.Raw2Volt_A + m_Data.Raw2Volt_B;
		Units = Volts * m_Data.RecordingMontage[i].Volt2Unit_A + m_Data.RecordingMontage[i].Volt2Unit_B;
		m_Data.RecordingMontage[i].Data = Units;
	}

	//	Calcul de la donnée résultante
	m_Data.UserMontage[Channel].Formula.Compute();

	//	Retour de la variable calculée
	return m_Data.Result;
}

double CHarmonieFile::GetTrueSampleFrequency() {
	return m_RecordingCalibration.TrueSampleFrequency;
}