// CoFrance.h : main header file for the CoFrance DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "CoFrancePlugIn.h"

// CCoFranceApp
// See CoFrance.cpp for the implementation of this class
//

class CCoFranceApp : public CWinApp
{
public:
	CCoFranceApp();

// Overrides
public:
	virtual BOOL InitInstance();

	CoFrancePlugIn* gpMyPlugin = NULL;

	DECLARE_MESSAGE_MAP()
};
