#pragma once

#include <string>
#include <vector>

namespace Harmonie {

class CMathExpression
{
public:
	CMathExpression();
	~CMathExpression();

	void AddVariable( const char *VarName, double *pVar, bool Message = true );
	bool ParseExpression( const char *Expr );
	double Compute();
	void AssignAnswerAddress( double *pAddress );

private:

	//	Caractères de remplacement dans les noms de variables
	enum EMathExprReplace
	{	OpParOpen = 1,	//	Parenthèse : Ouverture
		OpParClose,		//	Parenthèse : Fermeture
		OpMult,			//	Opérateur : *
		OpDiv,			//	Opérateur : /
		OpPlus,			//	Opérateur : +
		OpMinus,		//	Opérateur : -
		n0, n1, n2,	n3,	//	Chiffres 0...
		n4, n5, n6, n7,
		n8,	n9,			//	...à 9
		DynVar			//	Premier caractère du nom des variables dynamiques
	};

	//	Structure du vecteur 'm_Variables'
	struct MathExprVar
	{	std::string OriginalName;	//	Nom original de la variable
		std::string InternalName;	//	Nom de travail interne
		double *Address;			//	Adresse de la variable
	};

	// Structure du vecteur 'm_Steps'
	typedef void MathExprFunc( double *, double *, double * );
	struct MathExprStep
	{	double *v1, *v2, *v3;
		MathExprFunc *Function;
	};

	//	Type des fonctions (opérateurs)

	//	Messages
	enum EMathExprMessage
	{	MessageStart,
		ExistingVariable,
		UnbalancedParenthesis,
		VariableNotFound,
		DivisionByZero,
		MessagesCount
	};

	MathExprVar *FindVariable( const char *InternalName );
	bool Step1( std::string *Expr );
	bool Step2( std::string *Expr );
	const char *AddDynamicVariable( double Value );

	//	Fonctions de calcul
	static void fAdd( double *a, double *b, double *c );
	static void fSub( double *a, double *b, double *c );
	static void fMul( double *a, double *b, double *c );
	static void fDiv( double *a, double *b, double *c );
	static void fAbs( double *a, double *b, double *c );
	static void fCop( double *a, double *b, double *c );

	std::string m_Expression, m_Operators, m_DynVarPref, m_DynVarName;
	std::vector<MathExprVar> m_Variables;
	std::vector<MathExprStep>m_Steps;
	double m_Answer, *m_pAnswer;
	bool m_OK;
};

}