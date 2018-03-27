#pragma once
#include <string>    
#include <vector>  
#include "mysql.h"
// 定义MySQL连接信息  
typedef CString MYCHAR;
typedef struct
{
	MYCHAR server;
	MYCHAR user;
	MYCHAR password;
	MYCHAR database;
	int port;
}MySQLConInfo;


class CMyDataBase
{
public:

	~CMyDataBase();

	void SetMySQLConInfo(char* server, char* username, char* password, char* database, int port);// 设置连接信息  
	bool Open();  // 打开连接  
	void Close(); // 关闭连接  

	bool Select(const std::string& Querystr, std::vector<std::vector<std::string> >& data);      // 读取数据  
	bool Query(const std::string& Querystr);     // 其他操作  
	
	void ErrorIntoMySQL();       // 错误消息  
	const char* GetErrorInfo();  //获取错误信息
public:
	int ErrorNum;                // 错误代号    
	const char* ErrorInfo;       // 错误提示    

private:
	MySQLConInfo MysqlConInfo;   // 连接信息  
	MYSQL MysqlInstance;         // MySQL对象  
	MYSQL_RES *Result;           // 用于存放结果 
private:
	CMyDataBase();
	static CMyDataBase* m_singletion;
public:
	static CMyDataBase* GetInstance();
	/*
	初始化数据库:读取配置文件中的信息 登陆 远程数据库
	strPath: 配置文件的绝对路径
	*/
	bool InitMyDataBase(CString const& strPath);
};

