#include "StdAfx.h"
#include "CSQLite.h"
#include <conio.h>
#include "afxcoll.h"
#include "..\logrecord\LogRecord.h"
#include <winbase.h>

HANDLE CSQLite::m_eventOpenDB = CreateEvent(NULL,FALSE,TRUE,NULL);
CSQLite::CSQLite(void)
{
	pDB = NULL;
	stmt = NULL;
	errMsg = NULL;
}

CSQLite::~CSQLite(void)
{
}

wchar_t* CSQLite::UTF8ToUnicode( const char* str)
{
	int  unicodeLen = ::MultiByteToWideChar( CP_UTF8,0,str,-1,NULL,0 );  
	wchar_t *  pUnicode;  
	pUnicode = new  wchar_t[unicodeLen+1];  
	memset(pUnicode,0,(unicodeLen+1)*sizeof(wchar_t));  
	::MultiByteToWideChar( CP_UTF8,0,str,-1,(LPWSTR)pUnicode,unicodeLen );  
	return  pUnicode;  
}

BOOL CSQLite::ExecSql( const char* sz_sql,int (*Callback)(void *notUsed, int argc, char **argv, char **azColName)/* = null*/,void* vid /*= NULL */)
{
	try
	{
		if ( SQLITE_OK != sqlite3_exec(pDB , sz_sql ,Callback ? Callback : NULL ,vid, &errMsg) )
			return FALSE;
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}

BOOL CSQLite::OpenDataBase( const char* sz_db3Path,BOOL  synchronous/* = FALSE*/)
{
	// 打开SQLite数据库
	try
	{
		WaitForSingleObject(m_eventOpenDB,INFINITE);
		if ( SQLITE_OK != sqlite3_open(sz_db3Path, &pDB))
		{
			//_cprintf("Can't open database: %s\n", sqlite3_errmsg(pDB));
			AfxMessageBox(CString(sqlite3_errmsg(pDB)));
			sqlite3_close(pDB);
			SetEvent(m_eventOpenDB);
			return FALSE;
		}
		if ( synchronous )
		{
			if ( ExecSql( "PRAGMA synchronous = OFF;") ) 
			{
				return TRUE;
			}
			return FALSE;
		}
		return TRUE;
//		sqlite3_exec(pDB,"PRAGMA synchronous = OFF;",0,0,0);   //高速——写同步(synchronous)
	}
	catch ( ... )
	{
		return FALSE;
	}
	
}

BOOL CSQLite::CreateTable( const char* creattableSql)
{
//	string strSQL= "create table test (id int, name text);";
	/*
	int res = sqlite3_exec(pDB , creattableSql ,0 ,0, &errMsg);
	if (res != SQLITE_OK)
	{
		_cprintf("Create table error: %s\n", errMsg);
		return FALSE;
	}*/
	if ( !ExecSql(creattableSql))
		return FALSE;
	return TRUE;
}

BOOL CSQLite::InsertData( const char* insertSql )   //传入一条完整的sql语句
{
	// 插入数据
//	int res = sqlite3_exec(pDB,"begin transaction;",0,0, &errMsg);
	if ( ExecSql("begin transaction;") )
	{
		if ( ExecSql(insertSql) )
		{
			if ( ExecSql( "commit transaction;" ))
			{
				return TRUE;
			}
			return FALSE;
		}
		return FALSE;
	} 
	return FALSE;
	/*
	res = sqlite3_exec(pDB, insertSql,0,0, &errMsg);
	if (res != SQLITE_OK)
	{
		return FALSE;
	}
	res = sqlite3_exec(pDB,"commit transaction;",0,0, &errMsg);
	return TRUE;*/
}

BOOL CSQLite::SelectData( const char* selectSql,int (*Callback)(void *notUsed, int argc, char **argv, char **azColName)/* = null*/ ,void* vid /*= NULL*/)
{
	// 查询数据
//	string strSQL= "select * from test;";
	/*
	int res = sqlite3_exec(pDB, selectSql, SelectCallback, 0 , &errMsg);
	if (res != SQLITE_OK)
	{
		return -1;
	}*/
 	if ( ExecSql( selectSql,Callback,vid))
 		return TRUE;
	return FALSE;
}

BOOL CSQLite::UpdataData( const char* updataSql )
{
	/*
	int res = sqlite3_exec(pDB, updataSql, SelectCallback, 0 , &errMsg);
	if (res != SQLITE_OK)
	{
		return -1;
	}*/
	if (ExecSql(updataSql))
		return TRUE;
	return FALSE;
}

BOOL CSQLite::DeleteData( const char* deleteSql )
{
	/*
	int res = sqlite3_exec(pDB, deleteSql, SelectCallback, 0 , &errMsg);
	if (res != SQLITE_OK)
	{
		return -1;
	}*/
	if ( ExecSql(deleteSql) )
		return TRUE;
	return FALSE;
}

BOOL CSQLite::Droptable( const char* droptableSql)
{
	/*
	int res = sqlite3_exec(pDB, droptableSql, SelectCallback, 0 , &errMsg);
	if (res != SQLITE_OK)
	{
		return -1;
	}
	return 0;*/
	if ( ExecSql(droptableSql))
		return TRUE;
	return FALSE;
}

BOOL CSQLite::CloseDataBase()
{
	// 关闭数据库
	try
	{
		SetEvent(m_eventOpenDB);
		if ( pDB )
		{
			if ( SQLITE_OK == sqlite3_close(pDB) )
			{
				pDB = NULL;
				return TRUE;
			}
			return FALSE;
		}
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}

int CSQLite::SelectCallback(void *notUsed, int argc, char **argv, char **azColName)
{
// 	CInvQuery* dlg = (CInvQuery*)notUsed;
// 	int count = dlg->m_listctrl_showctrl.GetItemCount();
// 	dlg->m_listctrl_showctrl.InsertItem(count,0);
// 	for(int i = 0;i < argc;++ i )
// 	{
// 		dlg->m_listctrl_showctrl.SetItemText(count,i,(CMakeInvienceDlg::UTF8ToUnicode(argv[i])));
// 	}
		
	/*
	for (int i = 0 ; i < argc ; i++)

	{
		_cprintf("%s = %s", azColName[i], (argv[i] ? argv[i] : "NULL"));
		if (i != argc -1)
		{
			_cprintf(", ");
		}
	}*/
	return 0;
}


int CSQLite::SelectCallbackStat( void *notUsed, int argc, char **argv, char **azColName )
{
// 	CSPBJ* dlg = (CSPBJ*)(notUsed);
// 	int count = dlg->m_listctrl_showctrl.GetItemCount();
// 	dlg->m_listctrl_showctrl.InsertItem(count,0);
// 	for(int i = 0;i < argc;++ i )
// 	{
// 		dlg->m_listctrl_showctrl.SetItemText(count,i,(CMakeInvienceDlg::UTF8ToUnicode(argv[i])));
// 	}
	return 0;
}
int CSQLite::SelectCallbackSpxx( void *notUsed, int argc, char **argv, char **azColName )
{
// 	CStringArray* sp = (CStringArray*)(notUsed);
// 	for(int i = 0;i < argc;++ i )
// 	{
// 		sp->SetAt(i,(CMakeInvienceDlg::UTF8ToUnicode(argv[i])));
// 	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 效率提升
//////////////////////////////////////////////////////////////////////////
//非自动提交模式，调用sqlite3_prepare_v2和sqlite3_step语句，绑定方式

BOOL CSQLite::sqlite_step()
{
	try
	{
		if (sqlite3_step(stmt) != SQLITE_DONE)
		{
			fprintf(stderr, "sqlite3_step error, %s!!!\n", sqlite3_errmsg(pDB));
			if (stmt)
				sqlite3_finalize(stmt);
			sqlite3_close(pDB);
			return FALSE;
		}
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}

BOOL CSQLite::sqlite3_prepare( const char* sql)
{
	try
	{
		if (sqlite3_prepare_v2(pDB,sql,strlen(sql),&stmt,NULL) != SQLITE_OK)
		{
			//fprintf(stderr, "sqlite3_prepare_v2 error, %s!!!\n", sqlite3_errmsg(pDB));
			//AfxMessageBox(CString(sqlite3_errmsg(pDB)));
			CLogRecord::WriteRecordToFile(CString(sqlite3_errmsg(pDB)));
			if (stmt)
				sqlite3_finalize(stmt);
			sqlite3_close(pDB);
			return FALSE;
		}
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}
/*
BOOL CSQLite::sqlite3_bind( sqlite3_stmt* stmt,string* const& strArr_in,int const& number )
{
	try
	{
		//在绑定时，最左面的变量索引值是1。
//		sqlite3_bind_int(stmt, 1, 1);   //绑定int类型
		wchar_t* wch = _T("");
		int i = 1;
		while ( i <= number )
		{
#ifdef UNICODE
			sqlite3_bind_text16(stmt, i, wch, wcslen( wch), SQLITE_TRANSIENT);
			++i;
#else
			const char* ch = strArr_in[i-1].c_str();
			sqlite3_bind_text(stmt, i, ch, strlen( ch), SQLITE_TRANSIENT);  //绑定char*类型
			++i;
#endif	
		}
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}
*/
BOOL CSQLite::sqlite3_bind( sqlite3_stmt* stmt,CStringArray* const& strArr_in,int const& number )
{
	try
	{
//		sqlite3_bind_int(stmt, 1, 1);   //绑定int类型
		int i = 1;
		while ( i <= number )
		{
			const TCHAR* ch = strArr_in->GetAt(i-1);
#ifdef UNICODE
			sqlite3_bind_text16(stmt, i, ch, _tcslen(ch)*sizeof(TCHAR), SQLITE_TRANSIENT);
#else
			sqlite3_bind_text(stmt, i, ch, _tcslen(ch)*sizeof(TCHAR), SQLITE_TRANSIENT);	
#endif // UNICODE
			++i;
		}
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}
BOOL CSQLite::QuickInsertData(const char* insertSql,CStringArray** strArr_in,int const& count,int const& number,
							  BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray* const& strArr_in,int const& number))
{
	try
	{
		int i, num=0;
		if ( !sqlite3_prepare("BEGIN TRANSACTION") || !sqlite_step())  //开启事务
			return FALSE;
		sqlite3_finalize(stmt); //创建、销毁和重置sqlite3_stmt结构
		if ( !sqlite3_prepare( insertSql ))   //插入数据
			return FALSE;;
		i = 0;
		while ( i< count )
		{
			if ( NULL != sqlite3_bind && !sqlite3_bind(stmt,strArr_in[i],number) )
				return FALSE;
			if( !sqlite_step() )
				return FALSE;
			sqlite3_reset(stmt);  //重新初始化该sqlite3_stmt对象绑定的变量。
			num++;
			fprintf(stderr, "num=%d\n", num);
			++i;
		}
		sqlite3_finalize(stmt);
		if ( !sqlite3_prepare( "commit" ) ||  !sqlite_step())  //提交
			return FALSE;
		sqlite3_finalize(stmt);
		fprintf(stderr, "insertdata5 success!!!\n");
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}

	


BOOL CSQLite::QuickDeletData(const char* insertSql,CStringArray** strArr_in,int const& count,int const& number,
							  BOOL (*sqlite3_bind)(sqlite3_stmt*,CStringArray* const& strArr_in,int const& number))
{
	try
	{
		int i, num=0;
		if ( !sqlite3_prepare("BEGIN TRANSACTION") || !sqlite_step())  //开启事务
			return FALSE;
		sqlite3_finalize(stmt); //创建、销毁和重置sqlite3_stmt结构
		if ( !sqlite3_prepare( insertSql ))   //插入数据  准备sql
			return FALSE;;
		i = 0;
		while ( i< count )
		{
			if ( !sqlite3_bind(stmt,strArr_in[i],number) )
				return FALSE;
			if( !sqlite_step() )
				return FALSE;
			sqlite3_reset(stmt);  //重新初始化该sqlite3_stmt对象绑定的变量。
			num++;
			fprintf(stderr, "num=%d\n", num);
			++i;
		}
		sqlite3_finalize(stmt);
		if ( !sqlite3_prepare( "commit" ) ||  !sqlite_step())  //提交
			return FALSE;
		sqlite3_finalize(stmt);
		fprintf(stderr, "insertdata5 success!!!\n");
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}

BOOL CSQLite::QuickSelectDataCount( const char* insertSql,int& count )
{
	try
	{
		int i, num=0;
		if ( !sqlite3_prepare("BEGIN TRANSACTION") || !sqlite_step())  //开启事务
			return FALSE;
		sqlite3_finalize(stmt); //创建、销毁和重置sqlite3_stmt结构
		if ( !sqlite3_prepare( insertSql ))   //插入数据  准备sql
			return FALSE;
		int nColumn = sqlite3_column_count(stmt);
		int rc = 0;
		int vtype = 0;
		do 
		{
			rc = sqlite3_step(stmt);
			if(rc == SQLITE_ROW)
			{
				for(i = 0 ; i < nColumn ; i++ )
				{
					vtype = sqlite3_column_type(stmt , i);
					if(vtype == SQLITE_INTEGER)
					{
						int nt = sqlite3_column_int(stmt , i);
						count = nt;
						printf("%s : %d \n" , sqlite3_column_name(stmt , i) , sqlite3_column_int(stmt , i));
					}else if ( vtype == SQLITE_FLOAT)
					{
						double v = sqlite3_column_double(stmt,i);
					}
					else if(vtype == SQLITE_TEXT)
					{
						CString s = CString(sqlite3_column_text(stmt , i));
						printf("%s : %s \n" , sqlite3_column_name(stmt , i) , sqlite3_column_text(stmt , i));
					}
					else if(vtype == SQLITE_NULL)
					{
						printf("no values\n");
					}
				}
				printf("\n****************\n");
			}
			else if(rc == SQLITE_DONE)
			{
				break;
			}
			else
			{
				sqlite3_finalize(stmt);
				break;
			}
		} while (1);
		sqlite3_finalize(stmt);
		if ( !sqlite3_prepare( "commit" ) ||  !sqlite_step())  //提交
			return FALSE;
		sqlite3_finalize(stmt);
		fprintf(stderr, "insertdata5 success!!!\n");
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CSQLite::QuickSelectDataCount( const char* insertSql,double& count )
{
	try
	{
		int i, num=0;
		if ( !sqlite3_prepare("BEGIN TRANSACTION") || !sqlite_step())  //开启事务
			return FALSE;
		sqlite3_finalize(stmt); //创建、销毁和重置sqlite3_stmt结构
		if ( !sqlite3_prepare( insertSql ))   //插入数据  准备sql
			return FALSE;
		int nColumn = sqlite3_column_count(stmt);
		int rc = 0;
		int vtype = 0;
		do 
		{
			rc = sqlite3_step(stmt);
			if(rc == SQLITE_ROW)
			{
				for(i = 0 ; i < nColumn ; i++ )
				{
					vtype = sqlite3_column_type(stmt , i);
					if(vtype == SQLITE_INTEGER)
					{
						int nt = sqlite3_column_int(stmt , i);
						count = nt;
						printf("%s : %d \n" , sqlite3_column_name(stmt , i) , sqlite3_column_int(stmt , i));
					}else if ( vtype == SQLITE_FLOAT)
					{
						double v = sqlite3_column_double(stmt,i);
						count = v;
					}
					else if(vtype == SQLITE_TEXT)
					{
						CString s = CString(sqlite3_column_text(stmt , i));
						printf("%s : %s \n" , sqlite3_column_name(stmt , i) , sqlite3_column_text(stmt , i));
					}
					else if(vtype == SQLITE_NULL)
					{
						printf("no values\n");
					}
				}
				printf("\n****************\n");
			}
			else if(rc == SQLITE_DONE)
			{
				break;
			}
			else
			{
				sqlite3_finalize(stmt);
				break;
			}
		} while (1);
		sqlite3_finalize(stmt);
		if ( !sqlite3_prepare( "commit" ) ||  !sqlite_step())  //提交
			return FALSE;
		sqlite3_finalize(stmt);
		fprintf(stderr, "insertdata5 success!!!\n");
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
	return TRUE;
}
BOOL CSQLite::QuickSelectData(const char* insertSql,CStringArray** strArr_in,int& count,void* vid,
		BOOL (*select_bind)( void* vid,sqlite3_stmt* stmt,CStringArray& strArr_in,int const& nColumn )/*= NULL*/)
{
	try
	{
		int i, num=0;
		if ( !sqlite3_prepare("BEGIN TRANSACTION") || !sqlite_step())  //开启事务
			return FALSE;
		sqlite3_finalize(stmt); //创建、销毁和重置sqlite3_stmt结构
		if ( !sqlite3_prepare( insertSql ))   //插入数据  准备sql
			return FALSE;
		int nColumn = sqlite3_column_count(stmt);
		int rc = 0;
		int vtype = 0;
		int curcnt = 0;
		while(curcnt < 1)
		{
			rc = sqlite3_step(stmt);
			if(rc == SQLITE_ROW)
			{
			
				if ( NULL != select_bind && NULL != vid)
				{
					select_bind(vid,stmt,*strArr_in[curcnt],nColumn);
				}else
				{
					strArr_in[curcnt] = new CStringArray;
					strArr_in[curcnt]->SetSize(nColumn);
					for(i = 0 ; i < nColumn ; i++ )
					{
						vtype = sqlite3_column_type(stmt , i);
						if(vtype == SQLITE_INTEGER)
						{
							int nt = sqlite3_column_int(stmt , i);
							printf("%s : %d \n" , sqlite3_column_name(stmt , i) , sqlite3_column_int(stmt , i));
						}else if ( vtype == SQLITE_FLOAT)
						{
							double v = sqlite3_column_double(stmt,i);
						}
						else if(vtype == SQLITE_TEXT)
						{
							CStringA s= CStringA(sqlite3_column_text(stmt , i));
							strArr_in[curcnt]->SetAt(i,UTF8ToUnicode(s.GetBuffer()));
							s.ReleaseBuffer();
							//printf("%s : %s \n" , sqlite3_column_name(stmt , i) , sqlite3_column_text(stmt , i));
						}
						else if(vtype == SQLITE_NULL)
						{
							//printf("no values\n");
						}
					}
				}
				//printf("\n****************\n");
				++curcnt;
			}
			else if(rc == SQLITE_DONE)
			{
				break;
			}
		} 
		sqlite3_finalize(stmt);
		if ( !sqlite3_prepare( "commit" ) ||  !sqlite_step())  //提交
			return FALSE;
		sqlite3_finalize(stmt);
		fprintf(stderr, "insertdata5 success!!!\n");
		return TRUE;
	}
	catch ( ... )
	{
		return FALSE;
	}
}

void CSQLite::findDatabyID()
{
	char *sql = "select * from testtable where (id>=1 and id<=10) or \
				(id>=1000000 and id<=1000010) or (id>=2000000 and id<=2000010) or \
				(id>=3000000 and id<=3000010) or (id>=4000000 and id<=4000010) or \
				(id>=5000000 and id<=5000010) or (id>=6000000 and id<=6000010) or \
				(id>=7000000 and id<=7000010) or (id>=8000000 and id<=8000010) or \
				(id>=9000000 and id<=9000010);";//100条    9.8s

	int nrow,ncolumn;
	char **azResult=0;
	char *zErrMsg;
	int i,j;

	int ret = sqlite3_get_table(pDB,sql,&azResult,&nrow,&ncolumn,&zErrMsg);
	if(SQLITE_OK != ret)
	{
		printf("ret=%d, operate failed: %s\n",ret,zErrMsg);
	}

	//    fprintf(stderr, "nrow=%d, ncolumn=%d\n", nrow, ncolumn);
	for (i=1;i<=nrow;i++)
	{
		for (j=0;j <ncolumn; j++)
		{
			fprintf(stderr, "%s  ",azResult[i*ncolumn+j]);
		}
		fprintf(stderr, "\n");
	}
	sqlite3_free_table(azResult);
}

void CSQLite::findDatabyID_index(int begin, int end)
{
	char sql[100];
	sprintf_s(sql, "select * from testtable where (id>=%d and id<=%d)", begin, end);

	int nrow,ncolumn;
	char **azResult=0;
	char *zErrMsg;
	int i,j;

	int ret = sqlite3_get_table(pDB,sql,&azResult,&nrow,&ncolumn,&zErrMsg);
	if(SQLITE_OK != ret)
	{
		_cprintf("ret=%d, operate failed: %s\n",ret,zErrMsg);
	}

	//    fprintf(stderr, "nrow=%d, ncolumn=%d\n", nrow, ncolumn);
	for (i=1;i<=nrow;i++)
	{
		for (j=0;j<ncolumn; j++)
		{
			fprintf(stderr, "%s  ",azResult[i*ncolumn+j]);
		}
		fprintf(stderr, "\n");
	}
	sqlite3_free_table(azResult);
}

void CSQLite::CreateIndex(const char* CreateIndxeSql)
{
	char *zErrMsg;
//	char *indexsql = "CREATE INDEX id_index ON testtable (id);";
	if(SQLITE_OK != sqlite3_exec(pDB,CreateIndxeSql,0,0,&zErrMsg))
	{
		fprintf(stderr, "createIndex failed: %s\n",zErrMsg);
		sqlite3_close(pDB);
		exit(1);
	}
}

//来一条数据，然后和以前相同ID的数据对比，如果有，更新，没有，插入。
void CSQLite::replaceTest(int id)
{
	int nrow,ncolumn;
	char **azResult=0;
	char *zErrMsg;

	char *name="xyz";
	int year=2016,month=1,day=1,hour=1,min=1,sec=1;

//	gettimeofday(&t3,NULL);
	char selectsql[100];
	sprintf_s(selectsql, "select * from testtable where id=%d;", id);

	int ret = sqlite3_get_table(pDB,selectsql,&azResult,&nrow,&ncolumn,&zErrMsg);
	if(SQLITE_OK != ret)
	{
		printf("ret=%d, operate failed: %s\n",ret,zErrMsg);
	}
//	gettimeofday(&t4,NULL);
//	timeuse1+=timeuse;

	if (nrow>0)
	{
		fprintf(stderr, "====nrow=%d====， 存在!!!\n", nrow);
		int i,j;
		for (i=0;i<=nrow;i++)
		{
			for (j=0;j<ncolumn; j++)
			{
				fprintf(stderr, "%s  ",azResult[i*ncolumn+j]);
			}
			fprintf(stderr, "\n");
		}

//		gettimeofday(&t3,NULL);
		char updatesql[100];
		sprintf_s(updatesql, "update testtable set name='%s', birthday='d-d-d d:d:d' where id=%d;",
			name, year, month, day, hour, min, sec, id);
		if(SQLITE_OK != sqlite3_exec(pDB,updatesql,0,0,&zErrMsg))
		{
			fprintf(stderr, "insertdata1 failed: %s\n",zErrMsg);
			sqlite3_close(pDB);
			exit(1);
		}
//		gettimeofday(&t4,NULL);
//		timeuse2+=timeuse;
	}
	else
	{
//		gettimeofday(&t3,NULL);
		fprintf(stderr, "====nrow=0====， 不存在!!!\n");
		char insertsql[100];
		sprintf_s(insertsql, "insert into TESTTABLE values (%d,'%s','d-d-d d:d:d');",
			id, name, year, month, day, hour, min, sec);
		if(SQLITE_OK != sqlite3_exec(pDB,insertsql,0,0,&zErrMsg))
		{
			fprintf(stderr, "insertdata1 failed: %s\n",zErrMsg);
			sqlite3_close(pDB);
			exit(1);
		}
//		gettimeofday(&t4,NULL);
//		timeuse3+=timeuse;
	}
	sqlite3_free_table(azResult);
}

//来一条数据，然后和以前相同ID的数据对比，如果有，更新，没有，插入。
BOOL CSQLite::ReplaceData(const char* sz_ReplaceSql)
{
// 	char *name="ayz";
// 	int year=2016,month=1,day=1,hour=1,min=1,sec=1;
// 	char insertsql[100];
// 	sprintf_s(insertsql, "replace into TESTTABLE values (%d,'%s','d-d-d d:d:d');",
// 		id, name, year, month, day, hour, min, sec+1);
	char *zErrMsg;
	if(SQLITE_OK != sqlite3_exec(pDB,sz_ReplaceSql,0,0,&zErrMsg))
	{
		fprintf(stderr, "insertdata1 failed: %s\n",zErrMsg);
		sqlite3_close(pDB);
		return FALSE;
	}
	return TRUE;
}

void CSQLite::DeleteThis()
{
	if ( this )
		delete this;
}

BOOL CSQLite::BeginTransaction()
{
	if ( !sqlite3_prepare("BEGIN TRANSACTION") || !sqlite_step())  //开启事务
		return FALSE;
	sqlite3_finalize(stmt);
	return TRUE;
}

BOOL CSQLite::Commit()
{
	if ( !sqlite3_prepare( "commit" ) ||  !sqlite_step())  //提交
		return FALSE;
	sqlite3_finalize(stmt);
	return TRUE;
}



extern "C" CSQLITE_API ISQLite* CreateCSQLite( const char* InvType,const char* pwd )
{
	ISQLite* sql = new CSQLite;
	return sql ? sql : NULL ;
}