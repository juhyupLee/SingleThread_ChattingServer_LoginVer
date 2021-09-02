#include "RedisConnector.h"
#include "Log.h"
#include "CharacterEncoding.h"

RedisConnector::RedisConnector()
{
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(version, &data);
	m_Client = new cpp_redis::client;
}

RedisConnector::~RedisConnector()
{
	delete m_Client;
}

bool RedisConnector::Connect(const std::string& ip, size_t port)
{
	if (m_Client->is_connected())
	{
		return false;
	}
	try
	{
		m_Client->connect(ip, port);
		m_Client->sync_commit();
		return true;
	}
	catch (std::exception e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"Redis", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Connect() Error :%s ErrorCode:%d", errorMessage.c_str());
		return false;
	}
}

bool RedisConnector::Disconnect()
{
	try
	{
		if (m_Client->is_connected())
		{
			m_Client->disconnect();
			return true;
		}
		return true;
	}
	catch (std::exception e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"Redis", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Disconnect() Error :%s", errorMessage.c_str());
		return false;
	}
}

cpp_redis::reply RedisConnector::Get(const std::string& key)
{
	std::future<cpp_redis::reply> future;
	try
	{
		future = m_Client->get(key);
		m_Client->sync_commit();
		return future.get();
	}
	catch (std::exception e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"Redis", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Get() Error :%s", errorMessage.c_str());
		return future.get();
	}
}

bool RedisConnector::Set(const std::string& key, const std::string& value)
{
	try
	{
		m_Client->set(key, value);
		m_Client->sync_commit();
		return true;
	}
	catch (std::exception e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"Redis", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Set() Error :%s", errorMessage.c_str());
		return false;

	}
}
bool RedisConnector::SetEx(const std::string& key,int64_t second, const std::string& value)
{
	try
	{
		m_Client->setex(key, second, value);
		m_Client->sync_commit();
		return true;
	}
	catch (std::exception e)
	{
		std::wstring errorMessage;
		UTF8ToUTF16(e.what(), errorMessage);
		_LOG->WriteLog(L"Redis", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SetEx() Error :%s", errorMessage.c_str());
		return false;
	}
}

void TLSRedisConnector::Crash()
{
	int* p = nullptr;
	*p = 10;

}

RedisConnector* TLSRedisConnector::GetPointerTLS()
{
	RedisConnector* redisConnector = (RedisConnector*)TlsGetValue(m_TlsIndex);
	if (redisConnector == nullptr)
	{
		redisConnector = new RedisConnector();
		m_RedisConArray[InterlockedIncrement(&m_RedisConIndex)] = redisConnector;

		std::string ip;
		UTF16ToUTF8(m_IP.c_str(), ip);

		while (!redisConnector->Connect(ip,m_Port))
		{

		}
		TlsSetValue(m_TlsIndex, redisConnector);
	}

	return redisConnector;
}

TLSRedisConnector::TLSRedisConnector(const WCHAR* ip, size_t port, int32_t threadCount)
{
	m_IP = ip;
	m_Port = port;

	m_RedisConIndex = -1;
	m_RedisConArray = new RedisConnector * [threadCount];
	
	m_TlsIndex = TlsAlloc();
	if (m_TlsIndex == TLS_OUT_OF_INDEXES)
	{
		Crash();
	}
}

TLSRedisConnector::~TLSRedisConnector()
{
	TlsFree(m_TlsIndex);

	for (int i = 0; i <= m_RedisConIndex; ++i)
	{
		m_RedisConArray[i]->Disconnect();
		delete m_RedisConArray[i];
	}

	delete[] m_RedisConArray;
}


cpp_redis::reply TLSRedisConnector::Get(const std::string& key)
{

	RedisConnector* redisCon = GetPointerTLS();

	return redisCon->Get(key);
}

bool TLSRedisConnector::Set(const std::string& key, const std::string& value)
{
	RedisConnector* redisCon = GetPointerTLS();

	return redisCon->Set(key,value);
}

bool TLSRedisConnector::SetEx(const std::string& key, int64_t second, const std::string& value)
{
	RedisConnector* redisCon = GetPointerTLS();

	return redisCon->SetEx(key,second,value);
}
