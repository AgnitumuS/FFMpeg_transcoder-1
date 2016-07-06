
// FFMpeg_transcoder.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CFFMpeg_transcoderApp:
// See FFMpeg_transcoder.cpp for the implementation of this class
//

class CFFMpeg_transcoderApp : public CWinApp
{
public:
	CFFMpeg_transcoderApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CFFMpeg_transcoderApp theApp;