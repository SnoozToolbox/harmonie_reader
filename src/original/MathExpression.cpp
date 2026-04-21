#include "MathExpression.h"
#include "StdString.h"

#include <algorithm>
#include <cfloat>
#include <cstring>
#include <climits>
#include <math.h>

using namespace Harmonie;

CMathExpression::CMathExpression()
{
	char DynVarPref[32];

	//	Initialisation du texte des messages
	// m_Messages.Alloc( MessagesCount );
	// m_Messages.Add( _( "CMathExpression class\n\n" ) );
	// m_Messages.Add( _( "%s\n\nThis variable already exists." ) );
	// m_Messages.Add( _( "%s\n\nThe parentheses of this expression are not balanced." ) );
	// m_Messages.Add( _( "%s\n\nThis expression variable was not found." ) );
	// m_Messages.Add( _( "%s\n\nDivision by zero." ) );

	//	Assignation des opérateurs EN ORDRE DE PRIORITÉ
	m_Operators = "|()*/+-";

	//	Préfixe des variables dynamiques
	std::sprintf( DynVarPref, "%cdyn", DynVar );
	m_DynVarPref = DynVarPref;

	//	La réponse est entreposée dans 'm_Answer' par défaut
	m_pAnswer = &m_Answer;

	//	Aucune expression n'est encore évaluée
	m_OK = false;
}

CMathExpression::~CMathExpression()
{
	size_t i;

	//	Libération des variables dynamiques allouées dans 'AddDynamicVariable'
	for( i = 0; i < m_Variables.size(); i++ )
	{	if( m_Variables[i].OriginalName.substr( 0, 4 ) == m_DynVarPref )
			delete m_Variables[i].Address;
	}
}

/*	Ajoute une variable nommée 'VarName' dont le pointeur est 'pVar'.
	Si 'Message' (true par défaut), affiche un message si la variable 'VarName' existe déjà.  */
void CMathExpression::AddVariable( const char *VarName, double *pVar, bool Message )
{
	size_t i;
	char c;
	std::string Name;
	MathExprVar *Var;

	//	Liste des caractères numériques
	std::string cNum = "01234567890.";

	//	Enlèvement des espaces
	for( i = 0; i < strlen( VarName ); i++ )
	{	c = VarName[i];
		if( c != ' ' ) Name += c;
	}

	//	Name en minuscules
	CStdString::tolower( Name );

	/*	Construction du nom interne de la variable :
		Les caractères d'opérateurs et les chiffres en premier
		caractères sont modifiés.  */
	std::string InternalName = Name;												//	Copie de 'Name'
	std::replace( InternalName.begin(), InternalName.end(), '(', (char)OpParOpen );	//	Remplacement des...
	std::replace( InternalName.begin(), InternalName.end(), ')', (char)OpParClose );
	std::replace( InternalName.begin(), InternalName.end(), '*', (char)OpMult );
	std::replace( InternalName.begin(), InternalName.end(), '/', (char)OpDiv );
	std::replace( InternalName.begin(), InternalName.end(), '+', (char)OpPlus );
	std::replace( InternalName.begin(), InternalName.end(), '-', (char)OpMinus );	//	...caractères d'opérateurs
	if( InternalName[0] == '0' ) InternalName[0] = (char)n0;						//	Remplacement des chiffres...
	if( InternalName[0] == '1' ) InternalName[0] = (char)n1;
	if( InternalName[0] == '2' ) InternalName[0] = (char)n2;
	if( InternalName[0] == '3' ) InternalName[0] = (char)n3;
	if( InternalName[0] == '4' ) InternalName[0] = (char)n4;
	if( InternalName[0] == '5' ) InternalName[0] = (char)n5;
	if( InternalName[0] == '6' ) InternalName[0] = (char)n6;
	if( InternalName[0] == '7' ) InternalName[0] = (char)n7;
	if( InternalName[0] == '8' ) InternalName[0] = (char)n8;
	if( InternalName[0] == '9' ) InternalName[0] = (char)n9;						//	...en premiers caractères

	//	Vérification de l'existence de la variable
	Var = FindVariable( InternalName.c_str() );
	if( Var != nullptr )
	{	if( Message ) //DisplayMessage( ExistingVariable, VarName );
		return;
	}

	//	Ajout de la variable au vecteur 'm_Variables'
	Var = new MathExprVar;
	Var->OriginalName = Name;
	Var->InternalName = InternalName;
	Var->Address = pVar;
	m_Variables.push_back( *Var );
	delete Var;
}

//	Analyse l'expression 'Expr'
bool CMathExpression::ParseExpression( const char *Expr )
{
	int i;
	std::string Txt;

	//	Si l'expression est la même : sortie immédiate car elle a déjà été évaluée
	if( !strcmp( Expr, m_Expression.c_str() ) ) return m_OK;

	//	C'est une nouvelle expression
	m_Expression = Expr;	//	Mémorisation de l'expression
	m_Steps.clear();		//	Effacement des étapes de traitement

	/*	Effacement des variables dynamiques précédentes en commençant par la fin.
		Le nom des variables dynamiques commence toujours par 'm_DynVarPref'.  */
	for( i = m_Variables.size() - 1; i >= 0; i-- )
	{	if( m_Variables[i].OriginalName.substr( 0, 4 ) == m_DynVarPref )
		{	delete m_Variables[i].Address;
			m_Variables.erase( m_Variables.begin() + i );
	}	}
	m_DynVarName.clear();	//	Remise à zéro du nom de variable dynamique

	//	L'expression n'est pas encore évaluée
	m_OK = false;

	//	Premier traitement de l'expression (préparation)
	Txt = Expr;
	if( !Step1( &Txt ) ) return false;

	/*	Si l'expression n'était composée que d'un chiffre, elle a
		été convertie en variable dynamique et 'Txt' == 'm_DynVarName'.
		Il faut donc modifier 'm_DynVarName' pour poursuivre le traitement ci-dessous.  */
	if( Txt == m_DynVarName ) m_DynVarName += "loremipsum";

	//	Second traitement de l'expression (analyse)
	while( Txt != m_DynVarName )
	{	if( !Step2( &Txt ) )
		{	m_Steps.clear();
			return false;
	}	}

	//	Remplacement de la dernière variable des étapes par 'm_Reponse'
	m_Steps[m_Steps.size()-1].v3 = m_pAnswer;

	//	Succès de l'analyse
	m_OK = true;
	return m_OK;
}

/*	Calcule et retourne le résultat de l'expression,
	ou 'DBL_MAX' en cas d'erreur.  */
double CMathExpression::Compute()
{
	size_t i;
	MathExprFunc *Func;
	double *a, *b, *c;

	//	Retour immédiat si l'expression n'a pas encore été évaluée ou est erronée
	if( !m_OK ) return DBL_MAX;

	for( i = 0; i < m_Steps.size(); i ++ )	//	Pour chaque étape (4 éléments chacune)
	{	Func = m_Steps[i].Function;			//	Fonction à exécuter
		a = m_Steps[i].v1;					//	Première variable
		b = m_Steps[i].v2;					//	Deuxième variable
		c = m_Steps[i].v3;					//	Variable du résultat
		Func( a, b, c );					//	Exécution du calcul
	}

	//	Retourne le résultat
	return *m_pAnswer;
}

//	Modifie l'adresse de réponse par 'pAddress', ou par 'CMathExpression::m_Answer' si 'pAddress' est nullptr.
void CMathExpression::AssignAnswerAddress( double *pAddress )
{
	//	Modification de l'adresse de réponse
	if( pAddress == nullptr )
		m_pAnswer = &m_Answer;
	else
		m_pAnswer = pAddress;

	//	Remplacement de la dernière variable des étapes s'il y a lieu
	if( m_Steps.size() > 0 )
		m_Steps[m_Steps.size()-1].v3 = m_pAnswer;
}

//	Affiche le message 'MessageId' formaté avec le texte 'Txt'
//void CMathExpression::DisplayMessage( int MessageId, const char *Txt )
//{
	//	Composition du message avec des objets 'wxWidgets'
	//wxString Format, Mess;
	//Format.Printf( "%s%s", m_Messages[MessageStart], m_Messages[MessageId] );
	//Mess.Printf( Format, Txt );

	//	Affichage du message
	//wxMessageBox( Mess, L"CMathExpression", wxOK|wxICON_EXCLAMATION );
//}

/*	Cherche la variable 'InternalName' dans le vecteur 'm_Variables'
	Retourne la variable, ou nullptr si elle n'est pas trouvée.  */
CMathExpression::MathExprVar *CMathExpression::FindVariable( const char *InternalName )
{
	size_t i;

	bool Found = false;
	for( i = 0; i < m_Variables.size(); i++ )
	{	if( m_Variables[i].InternalName == InternalName )
		{	Found = true;
			break;
	}	}

	if( Found )
		return &m_Variables[i];
	else
		return nullptr;
}

//	Premier traitement préparatoire de l'expression 'Expr'
bool CMathExpression::Step1( std::string *Expr )
{
	std::string Txt, Var;
	size_t i, Pos;
	int np1, np2, Deb, Fin;
	char c;
	double Val;

	//	Liste des caractères numériques
	std::string cNum( "01234567890." );

	//	Suppression des espaces dans 'Expr'
	for( i = 0; i < Expr->length(); i++ )
	{	c = Expr->at( i );
		if( c != ' ' ) Txt += c;
	}

	//	Mise en minuscules
	CStdString::tolower( Txt );

	//	Remplacement de l'opérateur de valeur absolue par son symbole
	while( ( Pos = Txt.find( "abs(" ) ) != std::string::npos )
		Txt.replace( Pos, 4, "0|(", 3 );

	//	Remplacement des noms originaux des variables par leurs noms de travail internes
	for( i = 0; i < m_Variables.size(); i++ )
	{	Pos = 0;
		while( ( Pos = Txt.find( m_Variables[i].OriginalName, Pos ) ) != std::string::npos )
		{	Txt.replace( Pos, m_Variables[i].OriginalName.size(), m_Variables[i].InternalName );
			Pos += m_Variables[i].InternalName.length();
	}	}

	//	Vérification du balancement des parenthèses
	np1 = 0; np2 = 0;
	for( i = 0; i < Txt.size(); i++ )
	{	if( Txt[i] == '(' ) np1++;
		if( Txt[i] == ')' ) np2++;
	}
	if( np1 != np2 )
	{	//DisplayMessage( UnbalancedParenthesis, m_Expression.c_str() );
		return false;
	}

	//	Ajout temporaire de parenthèses pour l'étape qui suit
	Txt = "(" + Txt + ")";

	//	Remplacement des chiffres par des variables
	for( i = 1; i < Txt.size(); i++ )									//	Pour chaque caractère de l'expression
	{	if( cNum.find( Txt[i] ) != std::string::npos &&					//	Si c'est un chiffre...
			m_Operators.find( Txt[i-1] ) != std::string::npos )			//	...précédé d'un opérateur
		{	Deb = i;													//	Début
			Fin = Txt.substr( Deb ).find_first_of( m_Operators ) + Deb;	//	Fin : prochain opérateur
			Var = Txt.substr( Deb, Fin - Deb );							//	Copie de la variable numérique dans 'Var'
			Val = atof( Var.c_str() );									//	Valeur de 'Var'
			Var = AddDynamicVariable( Val );							//	Ajout de la variable dans une variable dynamique
			Txt = Txt.substr( 0, Deb ) + Var + Txt.substr( Fin );		//	Ajustement de l'expression
			i = 0;														//	Re-lecture complète de l'expression
	}	}

	//	Suppression des parenthèses ajoutées plus haut
	Txt = Txt.substr( 1, Txt.size() - 2 );

	//	Copie du résultat
	*Expr = Txt;

	//	Succès
	return true;
}

//	Second traitement (analyse) de l'expression 'Expr'
bool CMathExpression::Step2( std::string *Expr )
{
	std::string Txt, t1, t2, t3;
	std::string wxExpr;
	int Niv, Max, Deb, Fin;
	size_t i, pOp, Pr, Pos;
	MathExprVar *Var;
	MathExprStep *Step;

	//	Liste des opérateurs et de leur fonction associée
	std::string cOp = "|*/+-";
	MathExprFunc *fOp[5] = { CMathExpression::fAbs,
							 CMathExpression::fMul, CMathExpression::fDiv,
							 CMathExpression::fAdd, CMathExpression::fSub };

	//	Copie locale de l'expression
	Txt = *Expr;

	//	Détermination de l'emplacement du niveau de parenthèse le plus interne
	Niv = 0; Max = -1; Deb = 0;
	for( i = 0; i < Txt.size(); i++ )		//	Pour chaque caractère de l'expression
	{	if( Txt[i] == '(' )					//	Si la parenthèse est ouverte...
		{	Niv++;							//	...incrémentation du niveau
			if( Niv > Max )					//	Si c'est un nouveau maximum...
			{	Deb = i;					//	...mémorisation de sa position...
				Max = Niv;					//	...et de sa valeur
		}	}
		if( Txt[i] == ')' ) Niv--;			//	Si la parenthèse est fermée : décrémentation du niveau
	}

	//	S'il y a des parenthèses à traiter
	if( Max != -1 )
	{	//	Décomposition des parties de l'expression
		Fin = Txt.substr( Deb ).find( ')' );	//	Position de la fermeture de parenthèse
		t1 = Txt.substr( 0, Deb );				//	Partie gauche
		t2 = Txt.substr( Deb, Fin + 1 );		//	Partie à traiter
		t3 = Txt.substr( Deb + Fin + 1 );		//	Partie droite

		//	Suppression des parenthèses de la partie à traiter
		t2 = t2.substr( 1, t2.size() - 2 );

		//	Traitement de l'expression
		while( t2 != m_DynVarName )
		{	if( !Step2( &t2 ) )
			{	m_Steps.clear();
				return false;
		}	}

		//	Composition et copie de la nouvelle expression
		//wxExpr.Printf( "%s%s%s", t1, m_DynVarName, t3 );
        wxExpr = t1 + m_DynVarName + t3;
		*Expr = wxExpr;

		//	Succès
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	//	Rendu ici, il n'y a plus de parenthèses : Traitement des opérateurs  //
	///////////////////////////////////////////////////////////////////////////

	//	S'il n'y a aucun opérateur : Copie de la variable
	if( Txt.find_first_of( m_Operators ) == std::string::npos )
	{	Var = FindVariable( Txt.c_str() );						//	Recherche de la variable
		if( Var == nullptr )									//	Si elle n'est pas trouvée
		{	//DisplayMessage( VariableNotFound, Txt.c_str() );	//	Erreur
			return false;										//	Sortie
		}else
		{	Step = new MathExprStep;							//	Nouvel objet 'MathExprStep'
			Step->Function = CMathExpression::fCop;				//	Fonction : Copie
			Step->v1 = Var->Address;							//	Variable à copier
			Step->v2 = Var->Address;							//	Variable bidon non utilisée
			t1 = AddDynamicVariable( 0 );						//	Création de la variable de destination
			Var = FindVariable( t1.c_str() );					//	Recherche de cette nouvelle variable
			Step->v3 = Var->Address;							//	Variable de destination
			m_Steps.push_back( *Step );							//	Ajout à 'm_Steps'...
			delete Step;										//	... et libération de 'Step'
			*Expr = t1;											//	Expression résultante
			return true;										//	OK
	}	}

	/*	Détermination de l'emplacement de la première opération à effectuer
		selon l'ordre des priorités des opérations.  */
	Pr = UINT_MAX; Pos = 0;
	for( i = 0; i < Txt.size(); i++ )									//	Pour chaque caractère de l'expression
	{	if( ( pOp = m_Operators.find( Txt[i] ) ) != std::string::npos )	//	Si c'est une opération...
		{	if( pOp < Pr )												//	...et qu'elle est prioritaire à 'Pr'...
			{	Pr = pOp;												//	...mémorisation de la priorité...
				Pos = i;												//	...et de l'emplacement
	}	}	}

	//	Ajout temporaire de parenthèses pour l'étape qui suit et ajustement de 'Pos'
	Txt = "(" + Txt + ")";
	Pos++;

	//	Recherche de l'opérateur à gauche
	for( Deb = (int)Pos - 1; Deb >= 0; Deb-- )
	{	if( m_Operators.find( Txt[Deb] ) != std::string::npos ) break;
	}
	Deb++;

	//	Recherche de l'opérateur à droite
	for( i = (int)Pos + 1; i < Txt.size(); i++ )
	{	if( m_Operators.find( Txt[i] ) != std::string::npos ) break;
	}
	Fin = i;

	//	Décomposition des parties de l'expression
	t1 = Txt.substr( 0, Deb );			//	Partie gauche
	t2 = Txt.substr( Deb, Fin - Deb );	//	Partie à traiter
	t3 = Txt.substr( Fin );				//	Partie droite

	//	Composition des opérations pour la partie centrale à traiter
	Step = new MathExprStep;								//	Nouvel objet 'MathExprStep'
	Pos = t2.find( Txt[Pos] );								//	Position de l'opérateur
	i = cOp.find( t2[Pos] );								//	Recherche de l'opérateur dans le tableau d'opérateurs
	Step->Function = fOp[i];								//	Ajout de la fonction correspondante
	Txt = t2.substr( 0, Pos );								//	Variable de gauche
	Var = FindVariable( Txt.c_str() );						//	Recherche de la variable
	if( Var == nullptr )									//	Si elle n'est pas trouvée
	{	//DisplayMessage( VariableNotFound, Txt.c_str() );	//	Erreur
		delete Step;										//	Libération de 'Step'
		return false;										//	Sortie
	}
	Step->v1 = Var->Address;								//	Ajout de la variable
	Txt = t2.substr( Pos + 1 );								//	Variable de droite
	Var = FindVariable( Txt.c_str() );						//	Recherche de la variable
	if( Var == nullptr )									//	Si elle n'est pas trouvée
	{	if( Txt.empty() )									//	Si elle est vide :...
			Txt = "(none)";									//	...Remplacement pour l'affichage de l'erreur ci-dessous
		//DisplayMessage( VariableNotFound, Txt.c_str() );	//	Erreur
		delete Step;										//	Libération de 'Step'
		return false;										//	Sortie
	}
	Step->v2 = Var->Address;								//	Ajout de la variable
	Txt = AddDynamicVariable( 0 );							//	Création de la variable de destination
	Var = FindVariable( Txt.c_str() );						//	Recherche de cette nouvelle variable
	Step->v3 = Var->Address;								//	Ajout de la variable

	//	Ajout à 'm_Steps' et libération de 'Step'
	m_Steps.push_back( *Step );
	delete Step;

	//	Composition de la nouvelle expression
	//wxExpr.Printf( "%s%s%s", t1, m_DynVarName, t3 );
    wxExpr = t1 + m_DynVarName + t3;

	//	Suppression des parenthèses ajoutées ci-haut
	Txt = wxExpr;
	Txt = Txt.substr( 1, Txt.size() - 2 );

	//	Copie dans la destination
	*Expr = Txt;

	//	Succès
	return true;
}

/*	Ajoute une variable dynamique de valeur 'Value' au tableau 'm_Variables'.
	Retourne le nom unique de la variable.  */
const char *CMathExpression::AddDynamicVariable( double Value )
{
	double *pDouble;
	char Name[128];
	MathExprVar *Var;

	//	Nom unique de la variable
	std::sprintf( Name, "%cdyn%i", DynVar, (int)m_Variables.size() + 1 );

	//	Création de la variable
	pDouble = new double;
	*pDouble = Value;

	//	Ajout de la variable au vecteur
	Var = new MathExprVar;
	Var->OriginalName = Name;
	Var->InternalName = Name;
	Var->Address = pDouble;
	m_Variables.push_back( *Var );
	delete Var;

	//	Mémorise et retourne le nom de la variable
	m_DynVarName = Name;
	return m_DynVarName.c_str();
}

//	Fonctions de calcul : Addition
void CMathExpression::fAdd( double *a, double *b, double *c )
{
	*c = *a + *b;
}

//	Soustraction
void CMathExpression::fSub( double *a, double *b, double *c )
{
	*c = *a - *b;
}

//	Multiplication
void CMathExpression::fMul( double *a, double *b, double *c )
{
	*c = *a * *b;
}

//	Valeur absolue
void CMathExpression::fAbs( double *a, double *b, double *c )
{
	*c = fabs( *b );
}

//	Division avec vérification de division par zéro
void CMathExpression::fDiv( double *a, double *b, double *c )
{
	//wxString Txt;
    std::string Txt;

	//	Si le dénominateur est zéro
	if( *b == 0 )
	{	
        //Txt.Printf( "%g / %g", *a, *b );
        Txt = std::to_string(*a) + "/" + std::to_string(*b);
		//DisplayMessage( DivisionByZero, Txt );
		*c = DBL_MAX;
		return;
	}

	//	Suite
	*c = *a / *b;
}

//	Copie
void CMathExpression::fCop( double *a, double *b, double *c )
{
	*c = *a;
}
