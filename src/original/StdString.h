#pragma once

#include <string>

namespace Harmonie {

class CStdString
{
public:
	static void Oem850ToANSI( std::string &s );			//	Convertit le texte OEM-850 en ANSI
	static void ltrim( std::string &s );				//	Épure à partir du début de 's' (left)
	static void rtrim( std::string &s );				//	Épure à partir de la fin de 's' (right)
	static void trim( std::string &s );					//	Épure les 2 extrémités de 's' (right puis left)
	static void tolower( std::string &s );				//	Met 's' en minuscules
	static void toupper( std::string &s );				//	Met 's' en majuscules
	static void ltrim( std::wstring &s );				//	Épure à partir du début de 's' (left)
	static void rtrim( std::wstring &s );				//	Épure à partir de la fin de 's' (right)
	static void trim( std::wstring &s );				//	Épure les 2 extrémités de 's' (right puis left)
	static void tolower( std::wstring &s );				//	Met 's' en minuscules
	static void toupper( std::wstring &s );				//	Met 's' en majuscules
	static std::wstring &wides( const char *s );		//	Convertit 's' en 'wide characters'
	static std::wstring &wides( const std::string *s );	//	Convertit 's' en 'wide characters'
	static std::string &mbs( const wchar_t *s );		//	Convertit 's' en 'multibyte characters'
	static std::string &mbs( const std::wstring *s );	//	Convertit 's' en 'multibyte characters'

private:
	static std::wstring RetWide;
	static std::string RetMbs;
};

}