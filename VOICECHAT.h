
// VOICECHAT.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CVOICECHATApp: 
// �йش����ʵ�֣������ VOICECHAT.cpp
//

class CVOICECHATApp : public CWinApp
{
public:
	CVOICECHATApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
	
};
extern CString gstrGUI; //1 gui  0 ��gui
extern CVOICECHATApp theApp;
extern CString gstrPath;