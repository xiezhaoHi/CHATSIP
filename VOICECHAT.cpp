
// VOICECHAT.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "VOICECHAT.h"
#include "VOICECHATDlg.h"
#include "logrecord/LogRecord.h"
#include <windows.h>  
#include <imagehlp.h>  
#include <stdlib.h>  
#pragma comment(lib, "dbghelp.lib")  
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVOICECHATApp

BEGIN_MESSAGE_MAP(CVOICECHATApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CVOICECHATApp ����

CVOICECHATApp::CVOICECHATApp()
{
	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CVOICECHATApp ����

CVOICECHATApp theApp;
CString gstrGUI;
CString gstrPath; //�����ļ�����·��
// CVOICECHATApp ��ʼ��
// ����Dump�ļ�  
//   
void CreateDumpFile(LPCTSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException)
{
	// ����Dump�ļ�  
	//  
	HANDLE hDumpFile = CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Dump��Ϣ  
	//  
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	dumpInfo.ExceptionPointers = pException;
	dumpInfo.ThreadId = GetCurrentThreadId();
	dumpInfo.ClientPointers = TRUE;

	// д��Dump�ļ�����  
	//  
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);

	CloseHandle(hDumpFile);
}
// ����Unhandled Exception�Ļص�����  
//  
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{
	// ���ﵯ��һ������Ի����˳�����  
	//  
	//FatalAppExit(-1, _T("*** Unhandled Exception! ***"));

	CString strFile;
	TCHAR buff[1024] = { 0 };
	GetModuleFileName(NULL, buff, 1024);
	strFile = buff;
	int index = strFile.ReverseFind('\\');
	strFile = strFile.Left(index);
	CTime time = CTime::GetCurrentTime();
	CString logStr;
	logStr.Format(_T("%s\\dmp"), strFile);

	if (!PathFileExists(logStr))
	{
		if (!CreateDirectory(logStr, NULL))
		{
			strFile.Format(_T("%s\\%s.dmp"), strFile, time.Format(_T("%Y%m%d%H%M%S")));
		}
		else
			strFile.Format(_T("%s\\dmp\\%s.dmp"), strFile, time.Format(_T("%Y%m%d%H%M%S")));
	}
	else
		strFile.Format(_T("%s\\dmp\\%s.dmp"), strFile, time.Format(_T("%Y%m%d%H%M%S")));

	
	CreateDumpFile(strFile, pException);
	//MessageBox(NULL, L"ϵͳ����,�鷳����ϵ������Ա!", L"������ʾ", MB_OK);

	Sleep(3000);
	CString strPath = CLogRecord::GetAppPath() + _T("\\VOICECHAT.exe");
	//����

	//��������  
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION procStruct;
	memset(&StartInfo, 0, sizeof(STARTUPINFO));
	StartInfo.cb = sizeof(STARTUPINFO);
	::CreateProcess(
		(LPCTSTR)strPath,
		NULL,
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS,
		NULL,
		NULL,
		&StartInfo,
		&procStruct);

	return EXCEPTION_EXECUTE_HANDLER;
}

BOOL CVOICECHATApp::InitInstance()
{
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()��  ���򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// ���� shell ���������Է��Ի������
	// �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
	CShellManager *pShellManager = new CShellManager;

	// ���Windows Native���Ӿ����������Ա��� MFC �ؼ�����������
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));
	if (!CLogRecord::InitLogRecord())
	{
		MessageBox(NULL, "��ʼ����־�ļ�ʧ��!", "��ʾ", MB_OK);
		return FALSE;
	}
	//ע��dmp����
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#ifdef DEBUGDMP
	int* p = NULL;
	*p = 0;
#endif

	CString strIni = CLogRecord::GetAppPath()+"//config//config.ini";
	gstrPath = strIni;
	char bufGUID[MAX_PATH] = { 0 };
	GetPrivateProfileString("GUID", "GUID", "{565C5805-5B48-932D-10DF-52BB9687A197}"
		, bufGUID, MAX_PATH, strIni);
	CString strGUID = bufGUID;
	//��������
	CreateEvent(nullptr, 0, 0, strGUID);
	int err = GetLastError();
	if (183 == err)
	{
		CLogRecord::WriteRecordToFile("����ظ�����,ʧ��!");
#ifndef DEBUG 
		return FALSE;
#endif
	}

	char bufGUI[MAX_PATH] = { 0 };
	GetPrivateProfileString("GUI", "GUI", "0"
		, bufGUI, MAX_PATH, strIni);
	gstrGUI = CString(bufGUI);
	CVOICECHATDlg dlg;
	
	m_pMainWnd = &dlg;
	//INT_PTR nResponse = dlg.DoModal();
	INT_PTR nResponse = dlg.Create(IDD_VOICECHAT_DIALOG);

	dlg.SetWindowText(strGUID);
	if (CString(bufGUI) == CString("1"))
		dlg.ShowWindow(SW_SHOW);
	else
		dlg.ShowWindow(SW_HIDE);
	
	
	CLogRecord::WriteRecordToFile("--------�����������--------");
	dlg.RunModalLoop();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "����: �Ի��򴴽�ʧ�ܣ�Ӧ�ó���������ֹ��\n");
		TRACE(traceAppMsg, 0, "����: ������ڶԻ�����ʹ�� MFC �ؼ������޷� #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS��\n");
	}

	// ɾ�����洴���� shell ��������
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

#ifndef _AFXDLL
	ControlBarCleanUp();
#endif

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}



int CVOICECHATApp::ExitInstance()
{
	// TODO: �ڴ����ר�ô����/����û���
	CLogRecord::WriteRecordToFile("--------��������˳�--------");
	return CWinApp::ExitInstance();
}
