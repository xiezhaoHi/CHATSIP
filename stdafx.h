
// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // �� Windows ͷ���ų�����ʹ�õ�����
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

// �ر� MFC ��ĳЩ�����������ɷ��ĺ��Եľ�����Ϣ������
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ


#include <afxdisp.h>        // MFC �Զ�����



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // �������Ϳؼ����� MFC ֧��


//������ϢЭ������
enum PRO_ENUM
{
	PRO_LOGING = 1000, //����ϵͳ �� ����ģ�� ���� ��½ע��
	PRO_CALL ,//���� ��������
	PRO_USER_STATUS, //�� ����ϵͳ���� �û���״̬
	PRO_NEW_USER, //����ϵͳ ������ģ�� ���� ���û�ע��
	PRO_ERR,//������Ϣ  
	PRO_CALL_END,  //�������ڽ��е�ͨ��
	PRO_AUDIO_DEVICE, //�����豸
	PRO_AUDIO_CHOOSE, //ѡ�������豸
	PRO_EXIT //����˳�
};

//�û�״̬ 
enum USER_STATUS
{
	USER_BEGIN = 2000, //���ڿ�ʼ��½
	USER_ON , //�û���½
	USER_OFF, //�û�����
	USER_BUSY, //�û���æ
	USER_CALL_OFF,//ͨ������
	USER_CALL_ERR//ͨ������
};

//�û�����
enum USER_TYPE
{
	USERTYPE_1=1, //������
	USERTYPE_2 //��ͨ�û�
};

//�豸����
enum DEVICE_TYPE
{
	DEVICE_PLAY =3000, //�����豸
	DEVICE_CAPTURE //�ɼ��豸
};





#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


