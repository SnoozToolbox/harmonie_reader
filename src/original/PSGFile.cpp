#include "PSGFile.h"
#include "HarmonieFile.h"

using namespace Harmonie;

CPSGFile::CPSGFile()
{
}

/*	Ouverture d'un fichier PSG.

	Arguments :
		- pFileName			: Nom du fichier
		- ReadEvents		: Si 'true', lit les événements du fichier
		- DisplayMessages	: Si 'true', affiche les messages d'avertissement ou d'erreur

	Retourne un pointeur 'CPSGFile'.
	Vérifier la valeur de la variable 'CPSGFile::m_OK' :
		- true  : Succès.
		- false : Erreur.  */
// CPSGFile *CPSGFile::OpenFile( const char *pFileName, bool ReadEvents, bool DisplayMessages )
// {
// 	std::string FileName, Ext1, Ext2;
// 	FILE *f;
// 	CPSGFile *PSGFile;

// 	//	Liste des extensions permises dans 'FilePath'
// 	std::string ExtEcl = "|stg|";		//	Stellate Eclipse
// 	std::string ExtHar = "|sts|";		//	Stellate Harmonie
// 	std::string ExtEDF = "|rec|edf|";	//	EDF
// 	std::string ExtXlt = "|eeg|";		//	Xltek

// 	//	Copie du nom du fichier
// 	FileName = pFileName;

// 	//	Recherche et copie de l'extension dans 'Ext1'
// 	std::string::size_type pExt = FileName.rfind( '.' );
// 	Ext1 = FileName.substr( pExt + 1 );

// 	//	Mise de 'Ext1' en minuscules
// 	std::transform( Ext1.begin(), Ext1.end(), Ext1.begin(),	::tolower );

// 	//	Modification des extensions pour les fichiers Eclipse et Harmonie
// 	if( !Ext1.compare( "evt" ) )								//	Fichier Eclipse :...
// 	{	Ext1 = "stg";											//	...le 'stg' est ouvert en premier
// 		FileName = ReplaceExtension( FileName.c_str(), "stg" );
// 	}else if( !Ext1.compare( "sig" ) )							//	Fichier Harmonie :...
// 	{	Ext1 = "sts";											//	...le 'sts' est ouvert en premier
// 		FileName = ReplaceExtension( FileName.c_str(), "sts" );
// 	}

// 	//	Formatage de 'Ext1' avec ses délimineurs
// 	Ext2 = "|"; Ext2 += Ext1; Ext2 += "|";

// 	//	Ouverture du fichier
// 	f = fopen( FileName.c_str(), "rb" );
// 	if( f == nullptr )
// 	{	PSGFile = new CFileError( pFileName, FileNotOpened, DisplayMessages );
// 		return PSGFile;
// 	}

// 	//	Création de l'instance de classe correspondant au type de fichier
// 	if( ExtEcl.find( Ext2 ) != std::string::npos )
// 		PSGFile = new CEclipseFile( f, pFileName, ReadEvents, DisplayMessages );
// 	else if( ExtHar.find( Ext2 ) != std::string::npos )
// 		PSGFile = new CHarmonieFile( f, pFileName, ReadEvents, DisplayMessages );
// 	else if( ExtEDF.find( Ext2 ) != std::string::npos )
// 		PSGFile = new CEDFFile( f, pFileName, ReadEvents, DisplayMessages );
// 	else if( ExtXlt.find( Ext2 ) != std::string::npos )
// 		PSGFile = new CXltekFile( f, pFileName, ReadEvents, DisplayMessages );
// 	else
// 		PSGFile = new CFileError( pFileName, FileNotSupported, DisplayMessages );

// 	//	Retour
// 	return PSGFile;
// }

//	Initialise les variables communes de classe
void CPSGFile::InitPSG()
{
    //	Initialisations
    m_FileInfo.Version = 0;

    //	Installation du buffer du fichier 'm_File'
    AssignFILEBuffer();

    //	Pas d'édition des événements
    m_Editing = false;
}

//	Assigne le vecteur 'm_FileBuffer' comme buffer du fichier 'm_File'
void CPSGFile::AssignFILEBuffer()
{
    //	Allocation et application d'un buffer de lecture de 1 Mo. pour le fichier 'm_File'
    size_t Mo = 1048576; //	1 Mo.
    if (m_File != nullptr)
    {
        if (m_FileBuffer.size() == 0)
            m_FileBuffer.resize(Mo);
        setvbuf(m_File, m_FileBuffer.data(), _IOFBF, Mo);
    }
}

//	Libère les différents objets alloués
void CPSGFile::CleanCommonObjects()
{
    //	Fermeture du fichier
    if (m_File != nullptr)
    {
        fclose(m_File);
        m_File = nullptr;
    }
}

/*	Remplace l'extension d'un nom de fichier.
	Retourne 'pFileName' dont l'extension a été modifiée pour 'pExtension'  */
const char *CPSGFile::ReplaceExtension(const char *pFileName, const char *pExtension)
{
    static char FileName[1024];
    char *pExt;

    strcpy(FileName, pFileName);   //	Copie locale
    pExt = strrchr(FileName, '.'); //	Recherhe du dernier point...
    if (pExt == nullptr)
        return FileName; //	...et retour s'il n'y en a pas

    pExt[1] = 0;                  //	Troncation de 'FileName' après le point
    strcat(FileName, pExtension); //	Ajout de l'extension
    return FileName;              //	Retour
}

/*	Sépare le chemin du fichier 'pFileName' en ses composantes
	dans une structure 'SPLITPATH' :
		Folder, File, Extension  */
CPSGFile::SPLITPATH &CPSGFile::SplitPath(const char *pFileName)
{
    static SPLITPATH sp;

    std::string FileName = pFileName; //	Copie locale

    size_t pp = FileName.find_last_of("/\\"); //	Recherche du dernier caractère de dossier
    if (pp != std::string::npos)              //	Si présent
    {
        sp.Folder = FileName;
        sp.Folder.erase(pp);               //	Chemin dans '.Folder'
        sp.File = FileName.substr(pp + 1); //	Fichier dans '.File'
    }
    else //	S'il n'y a pas de dossier
    {
        sp.Folder.clear();  //	'.Folder' vide
        sp.File = FileName; //	Fichier dans '.File'
    }

    pp = sp.File.rfind('.');     //	Recherche du point
    if (pp != std::string::npos) //	Si présent
    {
        sp.Extension = sp.File.substr(pp + 1); //	Extension dans '.Extension'
        sp.File.erase(pp);                     //	Fichier dans '.File'
    }
    else                      //	S'il n'y a pas d'extension
        sp.Extension.clear(); //	'.Extension' vide

    return sp; //	Retour
}

/*	Retourne le chemin complet du fichier dont les
	composantes sont dans une structure 'SPLITPATH'.  */
//wipconst char *CPSGFile::MakePath(CPSGFile::SPLITPATH &sp)
//wip{
//wip    static std::string Path, s;
//wip
//wip    if (sp.Folder.find('\\') != std::string::npos) //	Caractères '\' ou '/' dans le chemin...
//wip        s = '\\';
//wip    else if (sp.Folder.find('/') != std::string::npos)
//wip        s = '/';
//wip    else
//wip        s.clear(); //	...selon le système de fichiers (Windows/Linux/iOS)
//wip
//wip    if (sp.Folder.length()) //	S'il y a un dossier
//wip    {
//wip        Path = sp.Folder;                 //	Copie dans 'Path'
//wip        if (Path[Path.length() - 1] != s) //	Si 'Path' n'est pas terminé par '\' ou '/' :...
//wip            Path += s;                    //	...ajout
//wip    }
//wip    else
//wip        Path.clear(); //	Aucun dossier
//wip
//wip    Path += sp.File; //	Ajout du nom du fichier
//wip
//wip    if (sp.Extension.length()) //	S'il y a une extension
//wip    {
//wip        if (sp.Extension[0] != '.') //	Si l'extension ne commence pas par '.' :...
//wip            Path += '.';            //	...ajout
//wip        Path += sp.Extension;       //	Ajout de l'extension
//wip    }
//wip
//wip    return Path.c_str(); //	Retour
//wip}

/*	Formate et retourne l'heure 'time_t' C avec les mSec en texte.
	Si 'mSec' est 'true', ajoute les mSec au texte retourné.  */
//WIPconst char *CPSGFile::CTimeToText( double CTime_mSec, bool mSec )
//WIP{
//WIP	static char Txt[32];
//WIP	double dFrac, dInt;
//WIP	time_t cTime;
//WIP	wxDateTime wxDT;
//WIP	wxString wxS;
//WIP	std::string stdS;
//WIP
//WIP	dFrac = modf( CTime_mSec, &dInt );							//	Séparation en partie fractionnelle et entière
//WIP	sprintf( Txt, "%.3f", dFrac );								//	Formatage des mSec
//WIP	if( atof( Txt ) == 1. )										//	Si les mSec sont '1.000'
//WIP	{	dInt += 1;												//	Ajustement de 'dInt'...
//WIP		dFrac = 0.;												//	...et 'dFrac'
//WIP	}
//WIP	cTime = (time_t)dInt;										//	Partie entière dans 'cTime'
//WIP	wxDT.Set( cTime );											//	Initialisation du 'wxDateTime' avec 'cTime'
//WIP
//WIP	if( cTime == 0 )											//	Correction de la date au 1900-01-01 si 'cTime' est 0
//WIP	{	wxDT.SetYear( 1900 );
//WIP		wxDT.SetMonth( wxDateTime::Jan );
//WIP		wxDT.SetDay( 1 );
//WIP		wxDT.SetHour( 0 );
//WIP		wxDT.SetMinute( 0 );
//WIP		wxDT.SetSecond( 0 );
//WIP		wxDT.SetMillisecond( 0 );
//WIP	}
//WIP
//WIP	wxS = wxDT.Format("%Y-%m-%d %H:%M:%S", wxDateTime::EST );	//	Formatage sans correction d'heure avancée
//WIP	stdS = wxS.ToStdString();									//	Conversion en 'std::string'...
//WIP	strcpy( Txt, stdS.c_str() );								//	...et copie dans 'Txt'
//WIP	if( mSec )													//	Ajout de 'mSec' à la fin de 'Txt'
//WIP	{	sprintf( Txt + 26, "%.3f", dFrac );						//	Formatage des 'mSec' à la fin de 'Txt'...
//WIP		memcpy( Txt + 19, Txt + 27, 5 );						//	...copié après les secondes
//WIP	}
//WIP
//WIP	//	Retour
//WIP	return Txt;
//WIP}

//	Retourne le nom associé à 'EPSGFileType'
const char *CPSGFile::GetName(EPSGFileType FileType)
{
    const char *Names[FileTypeCount] = {"Eclipse", "Harmonie", "EDF", "Xltek", "Unknown", "Error"};
    return Names[FileType];
}

//	Retourne le nom associé à 'EPSGGender'
const char *CPSGFile::GetName(EPSGGender Gender)
{
    const char *Names[GenderCount] = {"Inconnu", "F\xE9minin", "Masculin"};
    return Names[Gender];
}

//	Retourne le nom associé à 'EPSGSleepStage'
const char *CPSGFile::GetName(EPSGSleepStage SleepStage)
{
    const char *Names[StageND + 1] = {"\xC9veil", "N1", "N2", "N3", "4", "REM", "MT", "", "", "ND"};
    return Names[SleepStage];
}

//	Retourne le nom associé à 'EPSGChannelType'
const char *CPSGFile::GetName(EPSGChannelType ChannelType)
{
    const char *Names[ChannelType_Count] = {"None", "EEG", "SEEG", "EMG", "ESP", "SAO2", "Movement", "EOG", "ECG",
                                            "X1", "X2", "X3", "X4", "X5", "X6", "X7", "X8", "PhoticStim", "Pressure",
                                            "Volume", "Acidity", "Temperature", "Position", "Flow", "Effort", "Rate",
                                            "Length", "Light", "DigitalInput", "Microphone", "ECoG", "CO2", "Pleth",
                                            "RES", "THERM", "CPAP", "AC", "DC", "PULSE", "EVENTSWITCH", "PH", "TRIGGER", "Unknown"};
    return Names[ChannelType];
}

//	Retourne le nom associé à 'EPSGChannelUnits'
const char *CPSGFile::GetName(EPSGChannelUnits ChannelUnits)
{
    const char *Names[ChannelUnits_Count] = {"None", "MicroVolt", "MilliVolt", "Volt", "Percent", "CMH2O", "MMHG",
                                             "CC", "ML", "PH", "Celsius", "Fahrenheit", "BPM", "MM", "CM", "Lux", "AD", "Unknown"};
    return Names[ChannelUnits];
}
