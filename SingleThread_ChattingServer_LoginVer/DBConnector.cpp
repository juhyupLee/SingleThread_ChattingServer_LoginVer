#include "DBConnector.h"
#include "Log.h"
#include "CharacterEncoding.h"
#include <strsafe.h>

sql::Driver* DBConnector::m_Driver = get_driver_instance(); 


DBConnector::DBConnector(const WCHAR* dbIP,const WCHAR* dbPort, const WCHAR* user, const WCHAR* password, const WCHAR* dbName)
{
	QueryPerformanceFrequency(&m_QPF);
	m_DBIP = dbIP;
	m_DBPORT = dbPort;
	m_UserID = user;
	m_DBPassword = password;
	m_DBName = dbName;

}
DBConnector::~DBConnector()
{
	if (m_Connect != nullptr)
	{
		if (!m_Connect->isClosed())
		{
			m_Connect->close();
		}
	}
	//---------------------------------------
	// m_Result�� ����ڰ� �޸𸮰����ϴ°����� �̰�.
	//---------------------------------------
	delete m_Connect;
	delete m_Query;
}

bool DBConnector::Connect(void)
{
	//-----------------------------------------------------
	//�������̶�� �״�� �� Ŀ�ؼ��� Ȱ���Ѵ�
	//-----------------------------------------------------
	if (m_Connect !=nullptr)
	{
		if (!m_Connect->isClosed())
		{
			return true;
		}
	}
	std::string ip;
	std::string dbPort;
	std::string user;
	std::string password;
	std::string dbName;

	UTF16ToUTF8(m_DBIP.c_str(), ip);
	UTF16ToUTF8(m_DBPORT.c_str(), dbPort);
	UTF16ToUTF8(m_UserID.c_str(), user);
	UTF16ToUTF8(m_DBPassword.c_str(), password);
	UTF16ToUTF8(m_DBName.c_str(), dbName);

	try
	{
		char hostAddr[100] = { 0, };
		sprintf_s(hostAddr, "tcp://%s:%s", ip.c_str(), dbPort.c_str());
		m_Connect = m_Driver->connect(hostAddr, user, password);

		m_Connect->setSchema(dbName);

		//---------------------------------------------------
		// 	  Connect��, CreateStatement�� �����ϰ�, Release���� delete�Ѵ�
		//---------------------------------------------------
		m_Query = m_Connect->createStatement();
	}
	catch (sql::SQLException& e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL Connect() [ErroMessage:%s][ErrorCode:%d]", errorMessage.c_str(), e.getErrorCode());

		return false;
	}

	return true;
}

bool DBConnector::Disconnect(void)
{
	if (m_Connect != nullptr)
	{
		if (!m_Connect->isClosed())
		{
			m_Connect->close();
			return true;
		}
	}

	return false;
}

bool DBConnector::Query(WCHAR* stringFormat, ...)
{
	va_list va;
	sql::ResultSet* result;

	va_start(va, stringFormat);

	WCHAR message[QUERY_MAX_LEN];
	StringCchVPrintf(message, QUERY_MAX_LEN, stringFormat, va);

	std::string outString;
	UTF16ToUTF8(message, outString);

	try
	{
		//ping Ȯ��
		if (m_Connect->isClosed() == true)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Query_Result() Ping Error :�������");
			return false;
		}

		// ����� ���� ������

		QueryPerformanceCounter(&m_QueryStartTime);
		m_Query->executeUpdate(outString.c_str());
		QueryPerformanceCounter(&m_QueryEndTime);

		int64_t queryTime = (m_QueryEndTime.QuadPart - m_QueryStartTime.QuadPart) / 10000;

		
		//--------------------------------------------------------------
		// ������ ó���ϴ½ð��� ���� ������ �ð����� ũ�ٸ�, �����α׸������.(���� �����������)
		// ���� ��ü�� �α׷� �����.
		//--------------------------------------------------------------
		if (LOG_TIME < queryTime)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL Query() [�ɸ��ð�:%lld] [Query:%s]", queryTime, message);
		}


	}

	catch (sql::SQLException& e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);

		_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL Query():[Query:%s] [ErrorMessage:%s] [ErrorCode:%d]", message, errorMessage.c_str(), e.getErrorCode());
		return false;
	}


	return true;
}

bool DBConnector::Query(const WCHAR* stringFormat)
{
	std::string outString;
	UTF16ToUTF8(stringFormat, outString);
	try
	{
		//ping Ȯ��
		if (m_Connect->isClosed() == true)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Query_Result() Ping Error :�������");
			return false;
		}

		// ����� ���� ������

		QueryPerformanceCounter(&m_QueryStartTime);
		m_Query->executeUpdate(outString.c_str());
		QueryPerformanceCounter(&m_QueryEndTime);

		int64_t queryTime = (m_QueryEndTime.QuadPart - m_QueryStartTime.QuadPart) / 10000;


		//--------------------------------------------------------------
		// ������ ó���ϴ½ð��� ���� ������ �ð����� ũ�ٸ�, �����α׸������.(���� �����������)
		// ���� ��ü�� �α׷� �����.
		//--------------------------------------------------------------
		if (LOG_TIME < queryTime)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL Query() [�ɸ��ð�:%lld] [Query:%s]", queryTime, stringFormat);
		}
	}

	catch (sql::SQLException& e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);

		_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL Query():[Query:%s] [ErrorMessage:%s] [ErrorCode:%d]", stringFormat, errorMessage.c_str(), e.getErrorCode());
		return false;
	}

	return true;
}

bool DBConnector::Query_Result(const WCHAR* stringFormat, ...)
{
	va_list va;
	va_start(va, stringFormat);

	WCHAR message[QUERY_MAX_LEN];
	HRESULT result = StringCchVPrintf(message, QUERY_MAX_LEN, stringFormat, va);

	std::string outString;
	UTF16ToUTF8(message, outString);

	try
	{
		//ping Ȯ��
		if (m_Connect->isClosed() == true)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Query_Result() Ping Error :�������");
			return false;
		}
		// ����� ������

		QueryPerformanceCounter(&m_QueryStartTime);
		m_Result = m_Query->executeQuery(outString.c_str());
		QueryPerformanceCounter(&m_QueryEndTime);

		int64_t queryTime = (m_QueryEndTime.QuadPart - m_QueryStartTime.QuadPart) / 10000;
		
		//--------------------------------------------------------------
		// ������ ó���ϴ½ð��� ���� ������ �ð����� ũ�ٸ�, �����α׸������.(���� �����������)
		// ���� ��ü�� �α׷� �����.
		//--------------------------------------------------------------
		if (LOG_TIME < queryTime)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL QueryResult() [�ɸ��ð�:%lld] [Query:%s]", queryTime, message);
		}
	}

	catch (sql::SQLException& e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL QueryResult()[Query:%s] [ErrorMessage:%s] [ErrorCode:%d]", message, errorMessage.c_str(), e.getErrorCode());
		return false;
	}

	return true;
}

bool DBConnector::Query_Result(WCHAR* stringFormat)
{
	std::string outString;
	UTF16ToUTF8(stringFormat, outString);

	try
	{
		//ping Ȯ��
		if (m_Connect->isClosed() == true)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Query_Result() Ping Error :�������");
			return false;
		}
		// ����� ������

		QueryPerformanceCounter(&m_QueryStartTime);
		m_Result = m_Query->executeQuery(outString.c_str());
		QueryPerformanceCounter(&m_QueryEndTime);

		int64_t queryTime = (m_QueryEndTime.QuadPart - m_QueryStartTime.QuadPart) / 10000;

		//--------------------------------------------------------------
		// ������ ó���ϴ½ð��� ���� ������ �ð����� ũ�ٸ�, �����α׸������.(���� �����������)
		// ���� ��ü�� �α׷� �����.
		//--------------------------------------------------------------
		if (LOG_TIME < queryTime)
		{
			_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL QueryResult() [�ɸ��ð�:%lld] [Query:%s]", queryTime, stringFormat);
		}
	}

	catch (sql::SQLException& e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"SQL", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SQL QueryResult()[Query:%s] [ErrorMessage:%s] [ErrorCode:%d]", stringFormat, errorMessage.c_str(), e.getErrorCode());
		return false;
	}

	return true;
}

sql::ResultSet* DBConnector::FetchResult()
{
	if (m_Result == nullptr)
	{
		return nullptr;
	}
	return m_Result;
}


TLSDBConnector::TLSDBConnector(const WCHAR* dbIP, const WCHAR* dbPort, const WCHAR* user, const WCHAR* password, const WCHAR* dbName, int threadCount)
{
	m_DBConIndex = -1;
	m_DBConArray = new DBConnector * [threadCount];

	m_TlsIndex = TlsAlloc();
	if (m_TlsIndex == TLS_OUT_OF_INDEXES)
	{
		Crash();
	}

	m_DBIP = dbIP;
	m_DBPORT = dbPort;
	m_UserID = user;
	m_DBPassword = password;
	m_DBName = dbName;
}

TLSDBConnector::~TLSDBConnector()
{
	TlsFree(m_TlsIndex);

	for (int i = 0; i <= m_DBConIndex; ++i)
	{
		m_DBConArray[i]->Disconnect();
		delete m_DBConArray[i];
	}

	delete[]m_DBConArray;
}

void TLSDBConnector::Crash()
{
	int* p = nullptr;
	*p = 10;
}

bool TLSDBConnector::Query(WCHAR* stringFormat, ...)
{
	DBConnector* dbConnector = GetPointerTLS();

	va_list va;
	sql::ResultSet* result;
	va_start(va, stringFormat);

	WCHAR message[QUERY_MAX_LEN];
	StringCchVPrintf(message, QUERY_MAX_LEN, stringFormat, va);

	//-----------------------------------------
	// 	   �������� ���ø��� �Ű������� �ѱ�� �����Ƿ�.. 
	// 	   �̸� �����ؼ� �����ش�.
	//-----------------------------------------
	return dbConnector->Query((const WCHAR*)message);
}

bool TLSDBConnector::Query_Result(const WCHAR* stringFormat, ...)
{
	DBConnector* dbConnector = GetPointerTLS();

	va_list va;
	va_start(va, stringFormat);

	WCHAR message[QUERY_MAX_LEN];
	HRESULT result = StringCchVPrintf(message, QUERY_MAX_LEN, stringFormat, va);


	return dbConnector->Query_Result((const WCHAR*)message);
}

sql::ResultSet* TLSDBConnector::FetchResult()
{
	DBConnector* dbConnector = GetPointerTLS();

	return dbConnector->FetchResult();
}

DBConnector* TLSDBConnector::GetPointerTLS()
{
	DBConnector* dbConnector = (DBConnector*)TlsGetValue(m_TlsIndex);
	if (dbConnector == nullptr)
	{
		dbConnector = new DBConnector(m_DBIP.c_str(), m_DBPORT.c_str(), m_UserID.c_str(), m_DBPassword.c_str(), m_DBName.c_str());
		m_DBConArray[InterlockedIncrement(&m_DBConIndex)] = dbConnector;

		while (!dbConnector->Connect())
		{

		}
		TlsSetValue(m_TlsIndex, dbConnector);
	}

	return dbConnector;

}
