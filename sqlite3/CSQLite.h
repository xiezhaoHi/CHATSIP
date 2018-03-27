#pragma once
#ifndef SQLITE_EXPORTS
#define SQLITE_EXPORTS
#endif
#include <string> 
#include "ISQLite.h"
#pragma  comment(lib, "sqlite3/SQLITE3.LIB")
typedef BOOL (*select_bind)( void* vid,sqlite3_stmt* stmt,CStringArray& strArr_in,int const& nColumn );
class  CSQLITE_API CSQLite : public ISQLite
{
public:
	CSQLite(void);
	~CSQLite(void);
	static HANDLE m_eventOpenDB;
private:
	sqlite3 * pDB;
	char* errMsg;
	sqlite3_stmt* stmt;// = NULL;
private:
	BOOL    ExecSql(const char* sz_sql,int (*Callback)(void *notUsed, int argc, char **argv, char **azColName) = NULL,void *vid = NULL);
	BOOL	sqlite_step();   //sqlite3_step(stmt)
	BOOL	sqlite3_prepare( const char* sql ); //sqlite3_prepare_v2(pDB,sql,strlen(sql),&stmt,NULL) != SQLITE_OK))
	
public:
	BOOL	OpenDataBase( const char* sz_db3Path,BOOL  synchronous = FALSE);
	BOOL	CreateTable( const char* sz_creattableSql );
	BOOL	BeginTransaction();
	BOOL	Commit();
	BOOL	InsertData( const char* sz_insertSql );
	BOOL	SelectData( const char* sz_selectSql,int (*Callback)(void *notUsed, int argc, char **argv, char **azColName) = NULL,void* vid = NULL);
	BOOL	UpdataData( const char* sz_updataSql );
	BOOL	DeleteData( const char* sz_deletaSql );
	BOOL	Droptable( const char* sz_droptableSql );
	BOOL	CloseDataBase();
	static int SelectCallback(void *notUsed, int argc, char **argv, char **azColName);
	static int SelectCallbackStat(void *notUsed, int argc, char **argv, char **azColName);
	static int SelectCallbackSpxx(void *notUsed, int argc, char **argv, char **azColName);
//	static BOOL	sqlite3_bind( sqlite3_stmt* stmt,string* const& strArr_in,int const& number );
	static BOOL sqlite3_bind( sqlite3_stmt* stmt,CStringArray* const& strArr_in,int const& number );
	BOOL	QuickInsertData(const char* insertSql,CStringArray** strArr_in,int const& count,int const& number,
		BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray* const& strArr_in,int const& number));
	BOOL	QuickDeletData(const char* insertSql,CStringArray** strArr_in,int const& count,int const& number,
		BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray* const& strArr_in,int const& number));

	BOOL	QuickSelectDataCount(const char* insertSql,int& count);
	BOOL	QuickSelectDataCount( const char* insertSql,double& count );
	BOOL	QuickSelectData(const char* insertSql,CStringArray** strArr_in,int& count,void* vid=NULL,
		BOOL (*select_bind)( void* vid,sqlite3_stmt* stmt,CStringArray& strArr_in,int const& nColumn )= NULL);
	void	findDatabyID();
	void	findDatabyID_index(int begin, int end);
//	void	createIndex();
	void    CreateIndex(const char* CreateIndxeSql);
	void	replaceTest(int id);
	BOOL	ReplaceData(const char* sz_ReplaceSql);
	void    DeleteThis();
	wchar_t* UTF8ToUnicode( const char* str);
};

