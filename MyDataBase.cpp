#include "stdafx.h"
#include "MyDataBase.h"

 CMyDataBase*  CMyDataBase::m_singletion;

 CMyDataBase*  CMyDataBase::GetInstance()
 {
	 if (nullptr == m_singletion)
	 {
		 m_singletion = new CMyDataBase();
	 }
	 return m_singletion;
}
CMyDataBase::CMyDataBase():ErrorNum(0), ErrorInfo("ok")
{
	m_singletion = nullptr;
}
/*
初始化数据库:读取配置文件中的信息 登陆 远程数据库
strPath: 配置文件的绝对路径
*/
bool CMyDataBase::InitMyDataBase(CString const& strPath)
{
	mysql_library_init(0, NULL, NULL);
	mysql_init(&MysqlInstance);

	// 设置字符集，否则无法处理中文  
	mysql_options(&MysqlInstance, MYSQL_SET_CHARSET_NAME, "gbk");

	char buff[MAX_PATH] = { 0 };
	GetPrivateProfileString("DATABASE", "server", "127.0.0.1"
		, buff, MAX_PATH, strPath);
	MysqlConInfo.server = buff;
	memset(buff, 0, MAX_PATH);
	
	GetPrivateProfileString("DATABASE", "username", "root"
		, buff, MAX_PATH, strPath);
	MysqlConInfo.user = buff;
	memset(buff, 0, MAX_PATH);

	GetPrivateProfileString("DATABASE", "password", "root"
		, buff, MAX_PATH, strPath);
	MysqlConInfo.password = buff;
	memset(buff, 0, MAX_PATH);

	GetPrivateProfileString("DATABASE", "database", "userDB"
		, buff, MAX_PATH, strPath);
	MysqlConInfo.database = buff;
	memset(buff, 0, MAX_PATH);

	GetPrivateProfileString("DATABASE", "port", "3306"
		, buff, MAX_PATH, strPath);
	MysqlConInfo.port = atoi(buff);
	memset(buff, 0, MAX_PATH);

	return Open();
}

CMyDataBase::~CMyDataBase()
{
	mysql_server_end();
}
   

// 设置连接信息  
void CMyDataBase::SetMySQLConInfo(char* server, char* username, char* password, char* database, int port)
{
	MysqlConInfo.server = server;
	MysqlConInfo.user = username;
	MysqlConInfo.password = password;
	MysqlConInfo.database = database;
	MysqlConInfo.port = port;
}

// 打开连接  
bool CMyDataBase::Open()
{
	if (mysql_real_connect(&MysqlInstance, MysqlConInfo.server, MysqlConInfo.user,
		MysqlConInfo.password, MysqlConInfo.database, MysqlConInfo.port, 0, 0) != NULL)
	{
		return true;
	}
	else
	{
		ErrorIntoMySQL();
		return false;
	}
}

// 断开连接  
void CMyDataBase::Close()
{
	mysql_close(&MysqlInstance);
}

//读取数据  
bool CMyDataBase::Select(const std::string& Querystr, std::vector<std::vector<std::string> >& data)
{
	if (0 != mysql_query(&MysqlInstance, Querystr.c_str()))
	{
		ErrorIntoMySQL();
		return false;
	}

	Result = mysql_store_result(&MysqlInstance);

	// 行列数  
	int row = mysql_num_rows(Result);
	int field = mysql_num_fields(Result);

	MYSQL_ROW line = NULL;
	line = mysql_fetch_row(Result);

	int j = 0;
	std::string temp;
	std::vector<std::vector<std::string> >().swap(data);
	while (NULL != line)
	{
		std::vector<std::string> linedata;
		for (int i = 0; i < field; i++)
		{
			if (line[i])
			{
				temp = line[i];
				linedata.push_back(temp);
			}
			else
			{
				temp = "";
				linedata.push_back(temp);
			}
		}
		line = mysql_fetch_row(Result);
		data.push_back(linedata);
	}
	return true;
}

// 其他操作  
bool CMyDataBase::Query(const std::string& Querystr)
{
	if (0 == mysql_query(&MysqlInstance, Querystr.c_str()))
	{
		return true;
	}
	ErrorIntoMySQL();
	return false;
}


//错误信息  
void CMyDataBase::ErrorIntoMySQL()
{
	ErrorNum = mysql_errno(&MysqlInstance);
	ErrorInfo = mysql_error(&MysqlInstance);
}
//获取错误信息
const char* CMyDataBase::GetErrorInfo()
{
	return ErrorInfo;
}
