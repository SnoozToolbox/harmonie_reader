//	Définitions des types et des structures publiques de la classe CPSGFile
enum EPSGMessage	//	Messages (erreurs et avertissements)
{	MessageStart,
	FileNotOpened,
	FileNotSupported,
	ReadError,
	MemoryError,
	EventGroupNotDeletable,
	EventGroupDelete,
	TypeUnSavable,
	FileSeekError,
	WrongGUID,
	UnsupportedAmplifier,
	NoNoteOrMontage,
	ChannelFormulaNotFound,
	ErrorReadingEPO,
	BadFileSchema,
	MessagesCount
};

enum EPSGFileType	//	Type de fichier
{	FileTypeEclipse,
	FileTypeHarmonie,
	FileTypeEDF,
	FileTypeXltek,
	FileTypeUnknown,
	FileTypeError,
	FileTypeCount
};

enum EPSGGender	//	Sexe
{	GenderUnknown,
	GenderFemale,
	GenderMale,
	GenderCount
};

enum EPSGSleepStage	//	Stades de sommeil
{	StageWake = 0,
	StageN1 = 1,
	StageN2 = 2,
	StageN3 = 3,
	Stage3 = 3,		//	Rechtschaffen and Kales (R&K, 1968)
	Stage4 = 4,		//	Rechtschaffen and Kales (R&K, 1968)
	StageREM = 5,
	StageMT = 6,
	StageND = 9,
};

enum EPSGChannelType		//	Montage Channel Type
{	ChannelType_None,
	ChannelType_EEG,
	ChannelType_SEEG,
	ChannelType_EMG,
	ChannelType_ESP,
	ChannelType_SAO2,
	ChannelType_Movement,
	ChannelType_EOG,
	ChannelType_ECG,
	ChannelType_X1,
	ChannelType_X2,
	ChannelType_X3,
	ChannelType_X4,
	ChannelType_X5,
	ChannelType_X6,
	ChannelType_X7,
	ChannelType_X8,
	ChannelType_PhoticStim,
	ChannelType_Pressure,
	ChannelType_Volume,
	ChannelType_Acidity,
	ChannelType_Temperature,
	ChannelType_Position,
	ChannelType_Flow,
	ChannelType_Effort,
	ChannelType_Rate,
	ChannelType_Length,
	ChannelType_Light,
	ChannelType_DigitalInput,
	ChannelType_Microphone,
	ChannelType_ECoG,
	ChannelType_CO2,
	ChannelType_Pleth,
	ChannelType_RES,	// respiration band
	ChannelType_THERM,	// thermistor (for respiration)
	ChannelType_CPAP,	// for sleep studies
	ChannelType_AC,		// generic AC
	ChannelType_DC,		// generic DC
	ChannelType_PULSE,
	ChannelType_EVENTSWITCH,
	ChannelType_PH,
	ChannelType_TRIGGER,
	ChannelType_Unknown,
	ChannelType_Count
};

enum EPSGChannelUnits			//	Montage Channel Units
{	ChannelUnits_None,			//	Undefined
	ChannelUnits_MicroVolt,		//	MicroVolts
	ChannelUnits_MilliVolt,		//	MilliVolts
	ChannelUnits_Volt,			//	Volts
	ChannelUnits_Percent,		//	Percent units
	ChannelUnits_CMH2O,			//	Pressure in cm H2O
	ChannelUnits_MMHG,			//	Pressure in mm Hg
	ChannelUnits_CC,			//	in cc
	ChannelUnits_ML,			//	in mL
	ChannelUnits_PH,			//	Acidity in pH
	ChannelUnits_Celsius,		//	Temperature in degrees C
	ChannelUnits_Fahrenheit,	//	Temperature in degrees F
	ChannelUnits_BPM,			//	Heart/Respiration rate in beats/breaths per minute
	ChannelUnits_MM,			//	Length in mm
	ChannelUnits_CM,			//	Length in cm
	ChannelUnits_Lux,			//	Light intensity in Lux
	ChannelUnits_AD,			//	ADC counts
	ChannelUnits_Unknown,
	ChannelUnits_Count
};

//	Utilisé par les fonctions statiques 'SplitPath' et 'MakePath'
struct SPLITPATH
{	std::string Folder;
	std::string File;
	std::string Extension;
};
