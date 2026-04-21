#pragma once

#include "PSGFile.h"

#include <cstdio>
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <locale>
#include <vector>
#include <cfloat>

namespace Harmonie {

class CHarmonieFile : public CPSGFile
{

public:
	CHarmonieFile( FILE *pFile, const char *pFileName, bool ReadEvents = true, bool DisplayMessages = true );
	~CHarmonieFile();

protected:

private:
	FILE *m_SigFile;					//	Pointeur 'FILE'
	///////////////////////////////////////////
	//	Valeurs fixes fréquemment utilisées  //
	///////////////////////////////////////////
	double TIMECONSTANT;
	uint16_t	RECORDLENGTH;	//	Longueur d'un record, en points (toujours 64)
	int16_t RECMTG_ID;			//	ID du montage d'enregistrement dans 'm_EventGroup.Group[n].Montage'
	int16_t REFMTG0_ID;			//	ID du premier montage reformatté dans 'm_EventGroup.Group[n].Montage'

	//	Variables de classe
	uint32_t m_RecordCount;			//	Nombre de records de 64 points (LONRECORD) dans le fichier
	unsigned int m_SampSectGroup;	//	Index du groupe des sections d'échantillons, ou 'UINT_MAX' si non ititialisé
	unsigned int m_StageGroup;		//	Index du groupe des stades de sommeil, ou 'UINT_MAX' si non ititialisé
	unsigned int m_StageProperty;	//	Index de la propriété de l'item contenant le stade, ou 'UINT_MAX' si non ititialisé

	//////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CFileHeader'.  Voir la fonction 'ReadFileHeader'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_FileInfo'.		//
	//////////////////////////////////////////////////////////////////////////////////
	struct FILEHEADER
	{	uint32_t Separator;
		std::string ClassName;
		int16_t Version;
		int16_t FileType;	//	Type de fichier: EEG CSA etc...
	}m_FileHeader;

	////////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CPatientInfo'.  Voir la fonction 'ReadPatientInfo'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_PatientInfo'.	  //
	////////////////////////////////////////////////////////////////////////////////////
	enum EPatientSex
	{	PatientSex_Unknown = 0,
		PatientSex_Male =    1,
		PatientSex_Female =  2
	};
	struct PATIENTINFO
	{	uint32_t Separator;
		std::string ClassName;
		int16_t Version;
		std::string UserFieldP1, UserFieldP2, UserFieldP3, UserFieldP4, UserFieldP5;
	}m_PatientInfo;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Structure non documentée équivalente à 'CStdCoDec' et 'CDosCoDec'.  Voir la fonction 'ReadCoDec'.  //
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct CCODEC
	{	uint32_t Separator;
		std::string ClassName;
		int16_t Version;
		int16_t CompRecSize;					//	Compressed record
		int16_t SamplesRecord;					//	Sample/record for highest sample channel
		std::vector<int16_t> SamplesChannel;	//	Sample/channel size
		std::vector<char> DosCoDec;				//	Données supplémentaires (28 octets) pour un fichier 'STS' associé à un fichier RHYTHM 'EEG'

		/* NOTES :
		CompRecSize		: Taille d'un record de données (Fichiers SIG = somme de *SammplesChannel X 2 + 8, Fichiers EEG = somme de *SammplesChannel X 2)
		SamplesRecord	: Toujours 64
		SamplesChannel	: Nombre de points par record pour chaque canal (ex. 64, 64, 16, 32)  */
	}m_CoDec;

	////////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CCalibration'.  Voir la fonction 'ReadCalibration'.  //
	////////////////////////////////////////////////////////////////////////////////////
	struct CHANNELCALIBRATION
	{	float OutputMin, InputMin, OutputMax, InputMax;
		int16_t awUnit;
		int32_t als1, als2;
	};
	struct BODYPOSITION
	{	int16_t BposThres, BposUids;
		std::string BposKeys, BposNames;
	};
	struct RECORDINGCALIBRATION
	{	uint32_t Separator;
		std::string ClassName;
		int16_t Version;
		double TrueSampleFrequency;
		int16_t BaseCalibrationMinADC, BaseCalibrationMaxADC;
		float BaseCalibrationMinVolts, BaseCalibrationMaxVolts;
		std::vector<CHANNELCALIBRATION> Channel;
		std::vector<BODYPOSITION> BodyPosition;
	}m_RecordingCalibration;

	////////////////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CPhysicalMontage'.  Voir la fonction 'ReadPhysicalMontage'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_PhysicalMontage'.		  //
	////////////////////////////////////////////////////////////////////////////////////////////
	enum EMapType	//	Montage Map Type
	{	MapType_None =   0,
		MapType_Circle = 1,
		MapType_Grid =   2
	};
	struct PMELECTRODE
	{	double Coord1, Coord2, Coord3;
	};
	struct PHYSICALMONTAGE
	{	uint32_t Separator;
		std::string ClassName;
		int16_t Version;
		std::vector<PMELECTRODE> Electrode;
		EMapType MapType;
		int16_t ElectrodeCountX3;
	}m_PhysicalMontage;

	//////////////////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CRecordingMontage'.  Voir la fonction 'ReadRecordingMontage'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_Montages[0]'.				//
	//////////////////////////////////////////////////////////////////////////////////////////////
	enum EChannelType		//	Montage Channel Type
	{	ChannelType_None =          0,
		ChannelType_EEG =           1,
		ChannelType_SEEG =          2,
		ChannelType_EMG =           3,
		ChannelType_ESP =           4,
		ChannelType_SAO2 =          5,
		ChannelType_Movement =      6,
		ChannelType_EOG =           7,
		ChannelType_EKG =           8,
		ChannelType_X1 =            9,
		ChannelType_X2 =           10,
		ChannelType_X3 =           11,
		ChannelType_X4 =           12,
		ChannelType_X5 =           13,
		ChannelType_X6 =           14,
		ChannelType_X7 =           15,
		ChannelType_X8 =           16,
		ChannelType_PhoticStim =   17,
		ChannelType_Pressure =     18,
		ChannelType_Volume =       19,
		ChannelType_Acidity =      20,
		ChannelType_Temperature =  21,
		ChannelType_Position =     22,
		ChannelType_Flow =         23,
		ChannelType_Effort =       24,
		ChannelType_Rate =         25,
		ChannelType_Length =       26,
		ChannelType_Light =        27,
		ChannelType_DigitalInput = 28,
		ChannelType_Microphone =   29,
		ChannelType_ECoG =         30,
		ChannelType_CO2 =          31,
		ChannelType_Pleth =        32,
	};
	enum EChannelUnits					//	Montage Channel Units
	{	ChannelUnits_None =        0,	//	Undefined or unknown units
		ChannelUnits_MicroVolt =   1,	//	MicroVolts
		ChannelUnits_MilliVolt =   2,	//	MilliVolts
		ChannelUnits_Volt =        3,	//	Volts
		ChannelUnits_Percent =     4,	//	Percent units
		ChannelUnits_CMH2O =       5,	//	Pressure in cm H2O
		ChannelUnits_MMHG =        6,	//	Pressure in mm Hg
		ChannelUnits_CC =          7,	//	in cc
		ChannelUnits_ML =          8,	//	in mL
		ChannelUnits_PH =          9,	//	Acidity in pH
		ChannelUnits_Celsius =    10,	//	Temperature in degrees C
		ChannelUnits_Fahrenheit = 11,	//	Temperature in degrees F
		ChannelUnits_BPM =        12,	//	Heart/Respiration rate in beats/breaths per minute
		ChannelUnits_MM =         13,	//	Length in mm
		ChannelUnits_CM =         14,	//	Length in cm
		ChannelUnits_Lux =        15,	//	Light intensity in Lux
		ChannelUnits_AD =         16,	//	ADC counts
	};
	struct CHANNEL
	{	int16_t SpikeOptions, SeizureOptions;
	};
	struct RECCHANNEL : CHANNEL
	{	std::string Electrode1Name, Electrode2Name;
		int16_t UnderSamplingMethod;
		std::string Preprocessing;
	};
	struct RECORDINGMONTAGE
	{	uint32_t Separator;
		std::string ClassName;
		uint16_t RecordingMontageCount;
		int16_t Version;
		int32_t SamplesRecord;
		std::vector<RECCHANNEL> Channel;
	}m_RecordingMontage;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CReformatedMontage'.  Voir la fonction 'ReadReformatedMontages'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_Montages[1 à n]'.			   //
	/////////////////////////////////////////////////////////////////////////////////////////////////
	struct REFCHANNEL : CHANNEL
	{	std::string ChannelFormula;
	};
	struct MACRO
	{	std::string MacroLabel, MacroFormula;
	};
	struct REFMONTAGE
	{	int16_t Version;
		int16_t Twelve;
		std::vector<MACRO> Macro;
		uint16_t Separator;
		std::vector<REFCHANNEL> Channel;
	};
	struct REFORMATEDMONTAGE
	{	uint32_t Separator;
		std::string ClassName;
		std::vector<REFMONTAGE> Montage;
	}m_ReformatedMontages;

	///////////////////////////////////////////////////////////////////////////////////////////////
	//	Structure non documentée équivalente à 'CElecGroup'.  Voir la fonction 'ReadElecGroup'.  //
	//	(such as subdural grids, subdural strips, depth electrodes, or epidural electrodes)		 //
	///////////////////////////////////////////////////////////////////////////////////////////////
	enum EElectrodeGroupType	//	Electrode and Electrode Group definitions
	{	ElectrodeGroupType_Grid = 0,
		ElectrodeGroupType_Strip = 1,
		ElectrodeGroupType_Depth = 2,
		ElectrodeGroupType_Epidural = 3,
	};
	enum EElectrodeGroupSequence	//	Electrode name sequence direction
	{	Sequence_RowWise = 1,
		Sequence_ColWise = 2,
	};
	enum EElectrodeGroupOriginPoint	//	Electrode Group original point
	{	Origin_TopLeft = 1,
		Origin_TopRight = 2,
		Origin_BottomLeft = 3,
		Origin_BottomRight = 4,
	};
	struct ELECTRODE
	{	int32_t Separator;
		std::string ClassName;
		int16_t Version;
		std::string ElecName;
		int32_t PositionX, PositionY;
	};
	struct ELGROUP
	{	int16_t Version;
		std::string GroupName;
		double ContactSize;
		double InterDistance;
		EElectrodeGroupType Type;
		int32_t NumCol, NumRow, Color;
		EElectrodeGroupSequence RowOrColWise;
		EElectrodeGroupOriginPoint OriginPoint;
		std::vector<ELECTRODE> Electrode;
		uint16_t Separateur1, Separateur2, Separateur3;
	};
	struct ELECTRODEGROUP
	{	uint32_t Separator;
		std::string ClassName;
		std::vector<ELGROUP> ElecGroup;
		bool First;
	}m_ElectrodeGroup;

	////////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CStatusGroup'.  Voir la fonction 'ReadStatusGroup'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_EventGroups'.	  //
	////////////////////////////////////////////////////////////////////////////////////
	enum EGroupType	//	Event Group Type
	{	GroupType_None =     -1,
		GroupType_Break =     0,
		GroupType_Section =   1,
		GroupType_Detection = 2,
		GroupType_Label =     3,
		GroupType_Stage =     4,
		GroupType_Ruler =     5
	};
	enum EGroupQualifier	//	Event Group Qualifier
	{	GroupQualifier_None =              -1,
		GroupQualifier_UserSection =        0,
		GroupQualifier_UserDetection =      0,
		GroupQualifier_CalibrationSection = 1
	};
	enum EGroupExtent	//	Event Group Extent
	{	GroupExtent_None =         -1,
		GroupExtent_Instantaneous = 0,
		GroupExtent_Interval =      1
	};
	enum EMontageChannel			//	Montage Channel
	{	MontageChannel_None =  -3,
		MontageChannel_Any =   -2,	//	Group linked to a single montage channel
		MontageChannel_All =   -1	//	Group/Item not linked to any particular montage channel
		// 0						//	Group/Item linked to specified (zero-based) montage channel
	};
	enum EGroupChannel				// Group Channel Type
	{	GroupChannel_Any =   -2,	// Group/Item linked to a channel specified later at EventItem creation time
		GroupChannel_All =   -1,	// Group/Item not linked to any particular channel
		GroupChannel_Some = 0		// Group/Item linked to specified channel
	};
	enum EGroupAccess						//	Event Group Access
	{	GroupAccess_None =                       0,	//	No access
		GroupAccess_Creatable =                  1,	//	Set if event belonging to group can be created by user
		GroupAccess_Editable =                   2,	//	Set if event belonging to group can be edited by user
		GroupAccess_EditableCreatable =          GroupAccess_Editable | GroupAccess_Creatable,
		GroupAccess_Deletable =                  4,	//	Set if event belonging to group can be deleted by user
		GroupAccess_DeletableCreatable =         GroupAccess_Deletable | GroupAccess_Creatable,
		GroupAccess_DeletableEditable =          GroupAccess_Deletable | GroupAccess_Editable,
		GroupAccess_DeletableEditableCreatable = GroupAccess_Deletable | GroupAccess_Editable | GroupAccess_Creatable
	};
	enum EVisibilityStatus	//	Event or Group Visibility
	{	StatusHidden = 0,
		StatusVisible = 1
	};
	struct DEFITEM
	{	std::string Name;
		int32_t Color;
	};
	struct ITEMPROPERTY
	{	std::string Key, Description;
	};
	struct GROUPPROPERTY
	{	std::string Key, Description, Value;
	};
	struct EGROUP
	{	int16_t Version;
		std::string Id;
		EGroupType Type;
		EGroupQualifier Qualifier;
		EGroupExtent Extent;
		int16_t Montage;
		EMontageChannel MontageChannel;
		EGroupChannel GroupChannel;
		std::string Channel;
		int32_t Color;
		EGroupAccess GroupAccess;
		std::vector<DEFITEM> DefaultItem;
		EVisibilityStatus Visibility;
		int32_t UseNameFromList;
		std::vector<GROUPPROPERTY> GroupProperty;
		std::vector<ITEMPROPERTY> ItemProperty;
		uint16_t Separator;
	};
	struct EVENTGROUP
	{	uint32_t Separator;
		std::string ClassName;
		std::vector<EGROUP> Group;
	}m_EventGroup;

	//////////////////////////////////////////////////////////////////////////////////
	//	Structure équivalente à 'CStatusItem'.  Voir la fonction 'ReadStatusItem'.  //
	//	Les autres variables de la structure sont dans 'CPSGFile::m_EventItems'.	//
	//////////////////////////////////////////////////////////////////////////////////
	struct EITEM
	{	int16_t Version;
		int16_t Montage;
		EMontageChannel MontageChannel;
		int32_t Color;
		EVisibilityStatus Visibility;
		int16_t nSize;
		std::vector<std::string> ItemPropertyValue;
		uint16_t Separator;
		uint32_t SortIndex;
	};
	struct EVENTITEM
	{	uint32_t Separator;
		std::string ClassName;
		std::vector<EITEM> Item;
	}m_EventItem;

	////////////////////////////////////////////////////
	//	Structures permettant la lecture des données  //
	////////////////////////////////////////////////////
	struct DATARECORDINGMONTAGE								//	Structure de lecture des données pour chaque canal du montage d'enregistrement
	{	double Volt2Unit_A, Volt2Unit_B;					//	Facteurs de conversion des Volts aux unités réelles
		unsigned int SamplesPerRecord;						//	Nombre de points par record pour ce canal
		int16_t *pRawData;									//	Pointeur vers le premier point de la donnée brute 'DATAREADING::pRawData' de ce canal dans le record
		double Data;										//	Contient les données courantes d'un canal du montage d'enregistrement.  Utilisé par les instances 'CMathExpression'
	};
	struct DATAUSERMONTAGE									//	Structure de lecture des données pour chaque canal du montage utilisé dans la fonction 'InitDataReading'
	{	CMathExpression Formula;							//	Objet 'CMathExpression' calculant la valeur selon la définition du canal
		unsigned int SamplesPerRecord;						//	Nombre de points par record pour ce canal
	};
	struct DATAREADING										//	Structure de lecture des données
	{	std::vector<DATARECORDINGMONTAGE>RecordingMontage;	//	Vecteur 'DATARECORDINGMONTAGE' pour chaque canal du montage d'enregistrement
		std::vector<DATAUSERMONTAGE>UserMontage;			//	Vecteur 'DATAUSERMONTAGE' pour chaque canal du montage utilisé
		std::vector<char>RawData;							//	Données brutes pour 1 record lu dans le fichier
		char *pRawData;										//	Pointeur vers 'RawData' ci-dessus
		long RecordLength;									//	Longueur d'un record, en octets
		double *pRecordDate;								//	Pointeur vers la date/heure du record courant
		double Raw2Volt_A, Raw2Volt_B;						//	Facteurs de conversion des unités ADC brutes en Volts
		double SampleDuration;								//	Durée d'un point d'échantillon, en secondes
		double Result;										//	Variable de destination des calculs effectués par les objets 'CMathExpression'
		std::string FileName;								//	Nom du fichier de données
		fpos_t FileSize;									//	Taille du fichier de données
	}m_Data;

	//	Initialisation des variables de classe
	void InitHarmonie();

	//	Fonctions utilitaires
	void ReadClassName( std::string *Dest );
	void WriteClassName( const char *ClassName );
	void ReadText( std::string *Dest );
	void WriteText( const char *Text );
	double StellateTimeToCTime( double StellateTime );
	double CTimeToStellateTime( double CTime_mSec );
	CPSGFile::EPSGChannelType EChannelType2EPSGChannelType( EChannelType Type );
	CHarmonieFile::EChannelType EPSGChannelType2EChannelType( CPSGFile::EPSGChannelType Type );
	CPSGFile::EPSGChannelUnits EChannelUnits2EPSGChannelUnits( EChannelUnits Unit );
	CHarmonieFile::EChannelUnits EPSGChannelUnits2EChannelUnits( CPSGFile::EPSGChannelUnits Unit );

	//	Fonctions de lecture de chaque classe
	bool ReadFileHeader();
	bool ReadPatientInfo();
	bool ReadCoDec();
	bool ReadCalibration();
	bool ReadPhysicalMontage();
	bool ReadRecordingMontage();
	bool ReadReformatedMontages();
	bool ReadElecGroup();
	bool ReadElectrode( ELECTRODE *pElectrode );
	bool ReadStatusGroup();
	bool ReadStatusItem();

	//	Fonctions d'enregistrement de chaque classe
	void WriteFileHeader();
	void WritePatientInfo();
	void WriteCoDec();
	void WriteCalibration();
	void WritePhysicalMontage();
	void WriteRecordingMontage();
	void WriteReformatedMontages();
	void WriteElecGroup();
	void WriteElectrode( ELECTRODE *pElectrode );
	void WriteStatusGroup();
	void WriteStatusItem();

	//	Fonctions accessoires supplémentaires
	void FindGroupVariables();
	void SortEventItems();
	static bool SortPublicEventItemsFunction( const CPSGFile::EVITEM &p1, const CPSGFile::EVITEM &p2 );
	static bool SortPrivateEventItemsFunction( const EITEM &p1, const EITEM &p2 );
	void BuildSampleSections();
	void BuildEpochEvents();
	void AddMissingEpochs();
	void AssignSleepStagesToEvents();
	void ComputeEventsDuration();
	std::string FindSigExtensionFile(std::string filename);

	//	Fonctions associées aux groupes d'événements
	int GetEventGroupPropertyIndex( int Group, const char *Name );			//	Retourne l'index de la propriété de groupe 'Name' du groupe 'Group' ou -1 si la propriété n'a pas été trouvée
	const char *GetEventGroupPropertyValue( int Group, const char *Name );	//	Retourne la valeur de la propriété 'Name' du groupe 'Group' ou nullptr si la propriété n'a pas été trouvée
	int GetEventItemPropertyIndex( int Group, const char *Name );			//	Retourne l'index de la propriété d'événement 'Name' du groupe 'Group' ou -1 si la propriété n'a pas été trouvée

	//	Fonctions associées aux événements
	const char *GetEventItemPropertyValue( int Item, int Index );			//	Retourne la valeur de la propriété 'Index' de l'événement 'Item' ou nullptr si la propriété n'a pas été trouvée

public:
	//	Déclaration des fonctions virtuelles
	void BeginGroupsEventEditing( bool Start );
	uint32_t AddEventGroup( const char *Name, const char *Description, int32_t Color = 0x7FFF7F );
	int DeleteEventGroup( uint32_t Group, bool DisplayMessage = true );
	uint32_t AddEventItem( uint32_t Group, const char *Name, const char *Description, uint32_t StartSample, uint32_t SampleLenght, double StartTime, double TimeLenght, const char *Channel = nullptr );
	void DeleteEventItem( uint32_t Item );
	bool SaveFile( const char *pFileName = nullptr );
	bool InitDataReading( int MontageIndex = 0 );
	void EndDataReading();
	bool SeekToRecord( unsigned long SamplePoint );
	unsigned int GetRecordCount();
	unsigned int ReadRecord( unsigned long SamplePoint = ULONG_MAX );
	unsigned int GetRecordSampleCountForChannel( int Channel );
	unsigned int GetRecordPointForChannel( unsigned int SamplePoint, int Channel );
	double GetTimeOfPoint( unsigned int SamplePoint );
	double GetDataRaw( int Channel, int SamplePoint );
	double GetDataUnits( int Channel, unsigned int SamplePoint );
	double GetTrueSampleFrequency();

	// New interface
	int DeleteEventsByName(std::string eventName, uint32_t group_index);

	std::string m_lastError;
};

}