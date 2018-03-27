#pragma once
#include <vector>
#include <atlstr.h>


class CLogRecord
{
public:
	~CLogRecord(void);
private:
	CLogRecord(void);
	CString		m_apppath;
	CString		m_logFileDir; //日志文件夹 路径
	static		CLogRecord	singleton;

public:
	static CString GetAppPath();
	static BOOL InitLogRecord();
	static void WriteRecordToFile(CString strLog);
	static CString ReturnOCXPath();
	static CString ReturnCALLPath();
private:
	CFile	m_logFile;

public:
	/*
	date: 20160921
	author: xiezhao
	add: 定期清理 日志功能 
	参数说明: "0"表示只保留当天的 "1"表示 保留昨天 ... "5" 表示保留前5天的 以此类推
	*/
	BOOL   ClearLog(int day);
	void   TraverseDir(CString& dir, std::vector<CString>& vec);
};


