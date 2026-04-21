#pragma once

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <string>
#include <locale>
#include <vector>
#include <cfloat>

//	Pour la lecture du signal des fichiers
#include "MathExpression.h"

#define PSG_VERSION "0.0.3.14"
#define ECL_VERSION "1.0.0.0"
#define EDF_VERSION "0.0.0.1"
#define HAR_VERSION "1.0.0.0"
#define XLT_VERSION "0.9.0.2"

#define PSG_SECJOUR 86400.

namespace Harmonie {

class CPSGFile
{
	friend class CHarmonieFile;

public:

	//	Définitions des types et des structures publiques de la classe CPSGFile
    #include "PSGFileDefs.h"

	CPSGFile();
	virtual ~CPSGFile() {};	//	Destructeur virtuel
							//	Implémenter le destructeur pour chaque classe héritée

	//	Fonction statique d'ouverture d'un fichier PSG et ses variables
	// static CPSGFile *OpenFile( const char *pFileName, bool ReadEvents = true, bool DisplayMessages = true );

	//	Fonctions virtuelles pures de la classe de base.
	//	Elles doivent être implémentées dans chaque classe dérivée.
	//	Exemple : virtual int GetSomething() = 0;

	//	Fonctions d'ajout de groupes d'événements et d'événements
	virtual void BeginGroupsEventEditing( bool Start ) = 0;
	virtual uint32_t AddEventGroup( const char *Name, const char *Description, int32_t Color = 0x7FFF7F ) = 0;	//	RGB 127, 255, 127 : Vert avec saturation à 50%
	virtual int DeleteEventGroup( uint32_t Group, bool DisplayMessage = true ) = 0;
	virtual uint32_t AddEventItem( uint32_t Group, const char *Name, const char *Description, uint32_t StartSample, uint32_t SampleLenght, double StartTime, double TimeLenght, const char *Channel = nullptr ) = 0;
	virtual void DeleteEventItem( uint32_t Item ) = 0;

	//	Fonction d'enregistrement du fichier
	virtual bool SaveFile( const char *pFileName = nullptr ) = 0;

	//	Fonctions de lecture du signal
	virtual bool InitDataReading( int MontageIndex = 0 ) = 0;
	virtual bool SeekToRecord( unsigned long SamplePoint ) = 0;
	virtual unsigned int GetRecordCount() = 0;
	virtual unsigned int ReadRecord( unsigned long SamplePoint = ULONG_MAX ) = 0;
	virtual unsigned int GetRecordSampleCountForChannel( int Channel ) = 0;
	virtual unsigned int GetRecordPointForChannel( unsigned int SamplePoint, int Channel ) = 0;
	virtual double GetTimeOfPoint( unsigned int SamplePoint ) = 0;
	virtual double GetDataRaw( int Channel, int SamplePoint ) = 0;
	virtual double GetDataUnits( int Channel, unsigned int SamplePoint ) = 0;

	//	Fonctions utilitaires publiques
	static const char *ReplaceExtension( const char *pFileName, const char *pExtension );	//	Remplace l'extension de 'pFileName' pour 'pExtension'
	static SPLITPATH &SplitPath( const char *pFileName );
	//wip static const char *MakePath( SPLITPATH &SplittedPath );
	//WIP const char *CTimeToText( double CTime_mSec, bool mSec = true );	//	Formate et retourne l'heure 'CTime_mSec' C avec les mSec en texte

	//	Fonctions publiques retournant les noms des énumérations
	const char *GetName( EPSGFileType FileType );			//	Retourne le nom associé à 'EPSGFileType'
	const char *GetName( EPSGGender Gender );				//	Retourne le nom associé à 'EPSGGender'
	const char *GetName( EPSGSleepStage SleepStage );		//	Retourne le nom associé à 'EPSGSleepStage'
	const char *GetName( EPSGChannelType ChannelType );		//	Retourne le nom associé à 'EPSGChannelType'
	const char *GetName( EPSGChannelUnits ChannelUnits );	//	Retourne le nom associé à 'EPSGChannelUnits'

	//	Variables publiques
	bool m_OK;							//	Indique le succès de la création de l'instance de classe
	int	m_LengthOfSleepEpochs;			//	Longueur des époques de sommeil, ou -1 si indéfini
	uint16_t m_BaseSampleFrequency;		//	Fréquence d'échantillonnage de base, ou 0 si indéfini

	struct FILEINFO						//	Structure d'information à propos du fichier
	{	std::string FileName;
		EPSGFileType FileType;
		int Version;
		std::string Description, Institution;
		double CreationTime;
		std::string CreatedBy, LastModifiedBy;
	}m_FileInfo;

	struct PATIENTINFO					//	Structure d'information à propos du patient
	{	std::string Id1, Id2, Id, LastName, MiddleName, FirstName, Name;
		double BirthDate;
		EPSGGender Gender;
		std::string Address, City, State, Country, ZipCode, HomePhone, WorkPhone, Comments;
		std::string Height, Weight;
	}m_PatientInfo;

	// TODO c'est probablyment ici pour rien, c'est seulelemtn utilisé dans Harmonie.
	// On pourrait l'enlever d'ici et le mettre seuelemnt dans harmonie.
	// Dans Xlteck le premier montage dans m2_Montages est le montage physique.
	struct PHYSICALMONTAGE				//	Structure détaillant le montage physique (trous d'électrodes)
	{	std::string MontageName;		// XLteck: nom de l'ampli. Harmonie: nom du montage phsique décidé par les techniciens. EDF: Mettre un nom par défaut "physicalEDFMontage".
		std::vector<std::string> ElectrodeName;
	}m_PhysicalMontage;

	struct CHANNEL						//	Définition d'un canal
	{	std::string ChannelName;
		int16_t SampleFrequency;
		double TrueSampleFrequency;
		EPSGChannelType ChannelType;
		EPSGChannelUnits ChannelUnits;
		int Unit;
		int32_t ChannelColor;
	};
	struct MONTAGE						//	Structure détaillant un montage
	{	std::string MontageName;
		int Index;
		std::vector<CHANNEL> Channel;
	};
	std::vector<MONTAGE> m_Montages;	//	Vecteur de montages 'CPSGFile::MONTAGE'
										//	(0 = Recording Montage, 1... = Reformatted montages)

	struct EVGROUP						//	Structure détaillant les groupes d'événements
	{	std::string Name, Description;
		bool IsDeletable, SleepStageGroup, SampleSectionGroup;
	};
	std::vector<EVGROUP> m_EventGroups;	//	Vecteur des groupes 'CPSGFile::EVGROUP'

	struct EVITEM						//	Structure détaillant les événements
	{	bool RealEvent;
		EPSGSleepStage SleepStage;
		unsigned int Group;
        std::string GroupName;
		uint32_t StartSample, EndSample, SampleLength;
		double StartTime, EndTime, TimeLength;
		std::string Name, Description, guid;
		std::vector<std::string>Channels;
		void *pPrivate;
	};
	std::vector<EVITEM> m_EventItems;	//	Vecteur des événements 'CPSGFile::EVITEM'

	std::vector<uint32_t> m_SampleSectionEvents;	//	Index des événements de sections d'échantillon
	std::vector<uint32_t> m_SleepEpochEvents;		//	Index des événements d'époques de sommeil
	std::string m_ScoringSetName;					//	Nom du 'ScoringSet' Xltek SleepWorks

protected:

	void InitPSG();																							//	Initialise les variables communes de classe
	void AssignFILEBuffer();																				//	Assigne le vecteur 'm_FileBuffer' comme buffer du fichier 'm_File'
	//int //DisplayMessage( int MessageId, const char *Txt = nullptr, int Style = wxOK|wxICON_INFORMATION );	//	Affiche le message 'MessageId' et le texte 'Txt' s'il y a lieu, avec le style 'Style'
	void CleanCommonObjects();																				//	Libère les différents objets alloués

	FILE *m_File;					//	Pointeur 'FILE'
	std::vector<char>m_FileBuffer;	//	Buffer de lecture du fichier
	bool m_DisplayMessages;			//	Affiche les messages

private:

	bool m_Editing;	//	Modifié par la fonction 'BeginGroupsEventEditing'
};

}