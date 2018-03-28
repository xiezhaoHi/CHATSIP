
// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

// 关闭 MFC 对某些常见但经常可放心忽略的警告消息的隐藏
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展


#include <afxdisp.h>        // MFC 自动化类



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // 功能区和控件条的 MFC 支持


//定义消息协议类型
enum PRO_ENUM
{
	PRO_LOGING = 1000, //呼叫系统 向 语音模块 发送 登陆注册
	PRO_CALL ,//发起 语音聊天
	PRO_USER_STATUS, //向 呼叫系统发送 用户的状态
	PRO_NEW_USER, //呼叫系统 向语音模块 发送 新用户注册
	PRO_ERR,//错误信息  
	PRO_CALL_END,  //结束正在进行的通话
	PRO_AUDIO_DEVICE, //语音设备
	PRO_AUDIO_CHOOSE, //选择语音设备
	PRO_EXIT //软件退出
};

//用户状态 
enum USER_STATUS
{
	USER_BEGIN = 2000, //正在开始登陆
	USER_ON , //用户登陆
	USER_OFF, //用户离线
	USER_BUSY, //用户繁忙
	USER_CALL_OFF,//通话结束
	USER_CALL_ERR//通话错误
};

//用户类型
enum USER_TYPE
{
	USERTYPE_1=1, //管理者
	USERTYPE_2 //普通用户
};

//设备类型
enum DEVICE_TYPE
{
	DEVICE_PLAY =3000, //播放设备
	DEVICE_CAPTURE //采集设备
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


