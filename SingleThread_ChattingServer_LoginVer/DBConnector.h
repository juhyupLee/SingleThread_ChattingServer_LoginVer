#pragma once
//--------------------------------------------------------
// MySQL DB ���� Ŭ����
//������Ʈ-> �Ӽ� -> ����� -> ȯ��
// PATH=~~\MYSQLLib;%PATH%  DLL �˻��Ҽ��ֵ��� Path����
//--------------------------------------------------------

#pragma comment(lib,"MYSQLLib/mysqlcppconn.lib")
#include <Windows.h>
#include <stdlib.h>
#include <iostream>
#include <string>


#include "MYSQLLib/Connector C++ 8.0/include/jdbc/cppconn/driver.h"
#include "MYSQLLib/Connector C++ 8.0/include/jdbc/cppconn/exception.h"
#include "MYSQLLib/Connector C++ 8.0/include/jdbc/cppconn/resultset.h"
#include "MYSQLLib/Connector C++ 8.0/include/jdbc/cppconn/statement.h"


class DBConnector
{
public:
	enum
	{
		QUERY_MAX_LEN = 2048,
		LOG_TIME = 500
	};

	//----------------------------------------------------
	// 	   Memory Pool�� �Ⱦ��ٸ�, �׳� �����ڿ� �Ű����� ������ �ʱ�ȭ
	//----------------------------------------------------
	DBConnector(const WCHAR* dbIP, const WCHAR* dbPort, const WCHAR* user, const WCHAR* password, const WCHAR* dbName);
	virtual		~DBConnector();


	//////////////////////////////////////////////////////////////////////
	// MySQL DB ����
	//////////////////////////////////////////////////////////////////////
	bool		Connect(void);

	//////////////////////////////////////////////////////////////////////
	// MySQL DB ����
	//////////////////////////////////////////////////////////////////////
	bool		Disconnect(void);


	//----------------------------------------------------
	// 	   Quert������ ����� ����.
	//----------------------------------------------------
	bool		Query(WCHAR* stringFormat, ...);
	bool		Query(const WCHAR* stringFormat);

	//----------------------------------------------------
	// 	   Quert������ ����� ���� FetchResult�ؼ� ������̾ƿ;���
	//----------------------------------------------------
	bool		Query_Result(const WCHAR* stringFormat, ...);	
	bool		Query_Result(WCHAR* stringFormat);
	sql::ResultSet* FetchResult();


private:
	std::wstring m_UserID;
	std::wstring m_DBIP;
	std::wstring m_DBPORT;
	std::wstring m_DBPassword;
	std::wstring m_DBName;

	sql::Statement* m_Query;
	sql::ResultSet* m_Result;
	LARGE_INTEGER m_QueryStartTime;
	LARGE_INTEGER m_QueryEndTime;
	LARGE_INTEGER m_QPF;

	static sql::Driver* m_Driver;
	sql::Connection* m_Connect;

};

class TLSDBConnector
{

	enum
	{
		QUERY_MAX_LEN = 2048,
		LOG_TIME = 500
	};

public:
	TLSDBConnector(const WCHAR* dbIP, const WCHAR* dbPort, const WCHAR* user, const WCHAR* password, const WCHAR* dbName, int threadCount);
	~TLSDBConnector();
	void Crash();
public:

	//----------------------------------------------------
	// 	   Quert������ ����� ����.
	//----------------------------------------------------
	bool		Query(WCHAR* stringFormat, ...);


	//----------------------------------------------------
	// 	   Quert������ ����� ���� FetchResult�ؼ� ������̾ƿ;���
	//----------------------------------------------------
	bool		Query_Result(const WCHAR* stringFormat, ...);

	sql::ResultSet* FetchResult();

	DBConnector* GetPointerTLS();
private:
	DWORD m_TlsIndex;
	DBConnector** m_DBConArray;
	LONG m_DBConIndex;

private:
	std::wstring m_UserID;

	std::wstring m_DBIP;
	std::wstring m_DBPORT;
	std::wstring m_DBPassword;
	std::wstring m_DBName;

};