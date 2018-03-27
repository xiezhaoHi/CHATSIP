#ifndef ISQLITE_H
#define ISQLITE_H
//#include "stdafx.h"
#include "sqlite3.h"					//引入sqlite3.h 头文件 不同项目 可能路径不同
	//引入SQLITE3.LIB 头文件 不同项目 可能路径不同
using namespace std;
// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 SQLITE_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// SQLITE_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#ifdef  SQLITE_EXPORTS
#define CSQLITE_API __declspec(dllexport)
#else
#define CSQLITE_API __declspec(dllimport) 
#endif
//////////////////////////////////////////////////////////////////////////
// sqlite3 操作数据库
//////////////////////////////////////////////////////////////////////////
class ISQLite
{
public:
	//************************************
	// Method:    OpenDataBase 打开数据库
	// Parameter: const char * sz_db3Path 数据库路径
	// Parameter: BOOL synchronous 是否开启高速——写同步
	//************************************
	virtual BOOL	OpenDataBase( const char* sz_db3Path,BOOL  synchronous = FALSE ) = 0;
	//************************************
	// Method:    CreateTable 创建表
	// Parameter: const char * sz_creattableSql 创建表的sql语句  例：create table test (id int, name text)
	//************************************
	virtual BOOL	CreateTable( const char* sz_creattableSql ) = 0;
	//************************************
	// Method:    InsertData  像表中插入数据
	// Parameter: const char * sz_insertSql  插入数据sql 例：INSERT INTO test VALUES(?,?)
	//************************************
	virtual BOOL	InsertData( const char* sz_insertSql ) = 0;
	//************************************
	// Method:    SelectData  查询表中数据
	// Parameter: const char * sz_selectSql 查询sql 例：select * from test where ...
	// Parameter: int 
	// Parameter: * Callback 回调函数
	// Parameter: void * notUsed
	// Parameter: int argc 查询到数据的条数
	// Parameter: char * * argv 字段值
	// Parameter: char * * azColName 字段名
	//************************************
	virtual BOOL	SelectData( const char* sz_selectSql,
		int (*Callback)(void *notUsed, int argc, char **argv, char **azColName) = NULL,void* vid = NULL) = 0;
	//************************************
	// Method:    UpdataData 更新数据
	// Parameter: const char * sz_updataSql 更新数据sql 例：update test set name='%s', birthday='d-d-d d:d:d' where id=%d;
	//************************************
	virtual BOOL	UpdataData( const char* sz_updataSql ) = 0;
	//************************************
	// Method:    ReplaceData  更新数据 有则更新 无则插入
	// Parameter: int id 根据id 更新  replace into TESTTABLE values (%d,'%s','d-d-d d:d:d')
	//************************************
	virtual BOOL    ReplaceData(const char* sz_ReplaceSql) = 0;
	//************************************
	// Method:    DeleteData  删除数据
	// Parameter: const char * sz_deleteSql 删除数据sql 例：DELETE FROM test
	//************************************
	virtual BOOL	DeleteData( const char* sz_deleteSql ) = 0;
	//************************************
	// Method:    Droptable 删除表
	// Parameter: const char * sz_droptableSql 删除表sql  例：DROP TABLE TEST
	//************************************
	virtual BOOL	Droptable(const char* sz_droptableSql ) = 0;
	//************************************
	// Method:    CloseDataBase 关闭数据库
	//************************************
	virtual BOOL	CloseDataBase() = 0;
	//************************************
	// Method:    QuickInsertData  快速插入 使用结构绑定方式
	// Parameter: const char * insertSql 插入sql 例：INSERT INTO test VALUES(?,?)  对应的值用？表示
	// Parameter: CStringArray * const & strArr_in 待绑定的CStringArray*
	// Parameter: int const & count 待绑定CStringArray* 的个数
	// Parameter: * sqlite3_bind 回调函数 自己定义绑定CStringArray中的字符串
	// Parameter: sqlite3_stmt * 绑定的结构
	// Parameter: CStringArray const & strArr_in 待绑定的CStringArray
	//************************************
//	virtual BOOL	QuickInsertData(const char* insertSql,CStringArray* const& strArr_in,int const& count,
//										BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray const& strArr_in)) = 0;
	virtual BOOL	QuickInsertData(const char* insertSql,CStringArray** strArr_in,int const& count,int const& number,
		BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray* const& strArr_in,int const& number)) = 0;


	virtual BOOL	QuickDeletData(const char* insertSql,CStringArray** strArr_in,int const& count,int const& number,
		BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray* const& strArr_in,int const& number)) = 0;

	virtual BOOL	QuickSelectData(const char* insertSql,CStringArray** strArr_in,int& count,void* vid = NULL,
		BOOL (*select_bind)( void* vid,sqlite3_stmt* stmt,CStringArray& strArr_in,int const& nColumn )= NULL) = 0;
	//************************************
	// Method:    DeleteThis 删除自己对象  删除dll文件方式使用CreateCSQLite返回的对象
	//************************************
	//BEGIN TRANSACTION
	virtual BOOL	BeginTransaction() = 0;
	//commit
	virtual BOOL	Commit() = 0;
	//
	virtual void	CreateIndex(const char* CreateIndxeSql) = 0;
	virtual void    DeleteThis() = 0;
};

extern "C" CSQLITE_API ISQLite* CreateCSQLite(const char* InvType,const char* pwd );
#endif


/*
// 使用QuickInsertData函数 绑定类型实例
BOOL CSQLite::sqlite3_bind( sqlite3_stmt* stmt,CStringArray const& strArr_in )
{
	try
	{
		//在绑定时，最左面的变量索引值是1。
		sqlite3_bind_int(stmt, 1, 1);   //绑定int类型
		//使用了非UNICODE 编码
		//cstring to char* 问题
		sqlite3_bind_text(stmt, 2, strArr_in.GetAt(1), strlen(strArr_in.GetAt(1)), SQLITE_TRANSIENT);  //绑定char*类型
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}
//查询回调函数使用实例
int CSQLiteDlg::SelectCallback(void *notUsed, int argc, char **argv, char **azColName)
{
	for (int i = 0 ; i < argc ; i++)
	{
		_cprintf("%s = %s  ", azColName[i], (argv[i] ? argv[i] : "NULL"));
		if (i != argc -1)
		{
			_cprintf(", ");
			_cprintf("\n");
		}
	}
	return 0;
}
*/