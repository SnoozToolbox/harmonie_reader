#include "StdString.h"
#include <algorithm>

using namespace Harmonie;

//	Convertit le texte OEM-850 en ANSI
void CStdString::Oem850ToANSI( std::string &s )
{
	char CharTable[256];
	unsigned char c;
	size_t i;

	//	Table des caractères OEM-850 de 128 à 255
	unsigned char CharTable850High[128] = { 0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF,
											0xEE, 0xEC, 0xC4, 0xC5, 0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9,
											0xFF, 0xD6, 0xDC, 0xF8, 0xA3, 0xD8, 0xD7, 0x83, 0xE1, 0xED, 0xF3, 0xFA,
											0xF1, 0xD1, 0xAA, 0xBA, 0xBF, 0xAE, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
											0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xC1, 0xC2, 0xC0, 0xA9, 0xA6, 0xA6, 0x2B,
											0x2B, 0xA2, 0xA5, 0x2B, 0x2B, 0x2D, 0x2D, 0x2B, 0x2D, 0x2B, 0xE3, 0xC3,
											0x2B, 0x2B, 0x2D, 0x2D, 0xA6, 0x2D, 0x2B, 0xA4, 0xF0, 0xD0, 0xCA, 0xCB,
											0xC8, 0x69, 0xCD, 0xCE, 0xCF, 0x2B, 0x2B, 0xA6, 0x5F, 0xA6, 0xCC, 0xAF,
											0xD3, 0xDF, 0xD4, 0xD2, 0xF5, 0xD5, 0xB5, 0xFE, 0xDE, 0xDA, 0xDB, 0xD9,
											0xFD, 0xDD, 0xAF, 0xB4, 0x2D, 0xB1, 0x3D, 0xBE, 0xB6, 0xA7, 0xF7, 0xB8,
											0xB0, 0xA8, 0xB7, 0xB9, 0xB3, 0xB2, 0xA6, 0xA0 };

	//	Génération de la table de caractères complète
	for( i = 0; i < 128; i++ ) CharTable[i] = (char)i;
	for( i = 128; i < 256; i++ ) CharTable[i] = (char)CharTable850High[i-128];

	//	Conversion
	for( i = 0; i < s.length(); i++ )
	{	c = s[i];
		s[i] = CharTable[c];
	}
}

//	Épure à partir du début de 's' (left)
void CStdString::ltrim( std::string &s )
{
    s.erase( 0, s.find_first_not_of( " \t\n\r\f\v" ) );
}

//	Épure à partir de la fin de 's' (right)
void CStdString::rtrim( std::string &s )
{
    s.erase( s.find_last_not_of( " \t\n\r\f\v" ) + 1 );
}

//	Épure les 2 extrémités de 's' (right puis left)
void CStdString::trim( std::string &s )
{
    rtrim( s );
	ltrim( s );
}

//	Met 's' en minuscules
void CStdString::tolower( std::wstring &s )
{
	std::transform( s.begin(), s.end(), s.begin(), ::tolower );
}

//	Met 's' en majuscules
void CStdString::toupper( std::wstring &s )
{
	std::transform( s.begin(), s.end(), s.begin(), ::toupper );
}

//	Épure à partir du début de 's' (left)
void CStdString::ltrim( std::wstring &s )
{
    s.erase( 0, s.find_first_not_of( L" \t\n\r\f\v" ) );
}

//	Épure à partir de la fin de 's' (right)
void CStdString::rtrim( std::wstring &s )
{
    s.erase( s.find_last_not_of( L" \t\n\r\f\v" ) + 1 );
}

//	Épure les 2 extrémités de 's' (right puis left)
void CStdString::trim( std::wstring &s )
{
    rtrim( s );
	ltrim( s );
}

//	Met 's' en minuscules
void CStdString::tolower( std::string &s )
{
	std::transform( s.begin(), s.end(), s.begin(), ::tolower );
}

//	Met 's' en majuscules
void CStdString::toupper( std::string &s )
{
	std::transform( s.begin(), s.end(), s.begin(), ::toupper );
}

std::wstring CStdString::RetWide;
std::string CStdString::RetMbs;

//	Convertit 's' en 'wide characters'
std::wstring &CStdString::wides( const char *s )
{
	wchar_t *p;

	size_t Size = mbstowcs( nullptr, s, 0 );	//	Taille du 'wchar_t*' après la conversion
	RetWide.clear(); RetWide.shrink_to_fit();	//	Vidage
	RetWide.resize( Size );						//	Ajustement de la taille de 'returnw' à 'Size'
	p = (wchar_t *)RetWide.data();				//	Pointeur vers les données de 'returnw'
	mbstowcs( p, s, Size );						//	Conversion
	return CStdString::RetWide;					//	Retour
}

//	Convertit 's' en 'wide characters'
std::wstring &CStdString::wides( const std::string *s )
{
	return wides( s->c_str() );
}

//	Convertit 's' en 'multibyte characters'
std::string &CStdString::mbs( const wchar_t *s )
{
	char *p;

	size_t Size = wcstombs( nullptr, s, 0 );	//	Taille du 'char*' après la conversion
	RetMbs.clear(); RetMbs.shrink_to_fit();		//	Vidage
	RetMbs.resize( Size );						//	Ajustement de la taille de 'returnw' à 'Size'
	p = (char *)RetMbs.data();					//	Pointeur vers les données de 'returnw'
	wcstombs( p, s, Size );						//	Conversion
	return CStdString::RetMbs;					//	Retour
}

//	Convertit 's' en 'multibyte characters'	
std::string &CStdString::mbs( const std::wstring *s )
{
	return mbs( s->c_str() );
}
