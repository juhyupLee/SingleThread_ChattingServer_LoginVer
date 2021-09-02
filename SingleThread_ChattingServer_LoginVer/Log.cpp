#include "Log.h"
#include <time.h>
#include <strsafe.h>


SysLog::SysLog()
	:m_ServerONLogFileName{0,}
	,m_LogLevel(eLogLevel::LOG_LEVEL_DEBUG)
	,m_DirectoryName{0,}
{

	InitializeSRWLock(&m_Lock);
	m_LogCount = 0;

	//-------------------------------------------------------------
	// 	   ������ ���������ÿ� �α� ���� ����
	//-------------------------------------------------------------
#if SERVER_ON_LOG ==1
	time_t curTime;
	tm today;
	FILE* filePtr = nullptr;

	time(&curTime);
	localtime_s(&today, &curTime);
		
	wsprintf(m_ServerONLogFileName, L"ServerLog[20%d%02d%02d -%d[h]%d[m]%d[s]].txt", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec);
	_wfopen_s(&filePtr, m_ServerONLogFileName, L"wt,ccs=UTF-16LE");
	while (filePtr == nullptr)
	{
		_wfopen_s(&filePtr, m_ServerONLogFileName, L"wt,ccs=UTF-16LE");
	}
	fclose(filePtr);
	
#endif

}

SysLog::~SysLog()
{
	
}

SysLog* SysLog::GetInstance()
{
	static SysLog log;

	return &log;
}

void SysLog::WriteLog(const WCHAR* type, eLogLevel logLevel, const WCHAR* stringFormat, ...)
{
	if (m_LogLevel > logLevel)
	{
		return;
	}
	time_t curTime;
	tm today;
	FILE* filePtr = nullptr;
	va_list va;
	va_start(va, stringFormat);
	WCHAR message[BUFFER_SIZE] = { 0, };

	//--------------------------------------------------------
	// 	   ���� �������� �ð��� ���Ѵ�.
	//--------------------------------------------------------
	time(&curTime);
	localtime_s(&today, &curTime);

	//-------------------------------------------------
	// 	   ���� ��.
	//-------------------------------------------------
	AcquireSRWLockExclusive(&m_Lock);
	//-------------------------------------------------
#if SERVER_ON_LOG== 1
	//--------------------------------------------------------
	// 	   �α� ���н�, �α׿� ���� �α׸� ����°�.
	//--------------------------------------------------------
	if (type == nullptr)
	{
		WCHAR fileName[MAX_PATH] = { 0, };
		HRESULT resultFile = StringCchPrintf(fileName, MAX_PATH, L"%s[20%d0%d].txt", L"LogFail", today.tm_year - 100, today.tm_mon + 1);

		//--------------------------------------------------------------
		// 	   File Open
		//--------------------------------------------------------------
		_wfopen_s(&filePtr, fileName, L"at,ccs=UTF-16LE");
		while (filePtr == nullptr)
		{
			_wfopen_s(&filePtr, fileName, L"at,ccs=UTF-16LE");
		}

		//--------------------------------------------------------------
		// 	  Safe String���� ���ڿ� ���� 
		//--------------------------------------------------------------
		HRESULT result = StringCchVPrintf(message, BUFFER_SIZE, stringFormat, va);

		if (result == STRSAFE_E_INVALID_PARAMETER)
		{
		}
		else if (result == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			//-------------------------------------------------------
			// ���� �غ��� ���� �������� ���ڿ��� �߷��� ���, �� �߸��� ���� �α׸� �����.
			// �׸��� ������ ��ġ�� �����Ѵ�.
			//-------------------------------------------------------
			WCHAR mustSucMessage[BUFFER_SIZE] = { 0, };
			StringCchVPrintf(mustSucMessage, 255, L"���ڿ� ���� �������� ���ڿ��� �߷Ƚ��ϴ١�", va);
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"LogFail", L"ERROR", message);
		}

		switch (logLevel)
		{
		case eLogLevel::LOG_LEVEL_DEBUG:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"LogFail", L"DEBUG", message);
			break;
		case eLogLevel::LOG_LEVEL_ERROR:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"LogFail", L"ERROR", message);
			break;
		case eLogLevel::LOG_LEVEL_SYSTEM:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"LogFail", L"SYSTEM", message);
			break;
		}
		fclose(filePtr);
	}
	else
	{
		//--------------------------------------------------------------
		// 	   File Open
		//--------------------------------------------------------------
		_wfopen_s(&filePtr, m_ServerONLogFileName, L"at,ccs=UTF-16LE");
		while (filePtr == nullptr)
		{
			_wfopen_s(&filePtr, m_ServerONLogFileName, L"at,ccs=UTF-16LE");
		}

		//--------------------------------------------------------------
		// 	  Safe String���� ���ڿ� ���� 
		//--------------------------------------------------------------
		HRESULT result = StringCchVPrintf(message, BUFFER_SIZE, stringFormat, va);

		if (result == STRSAFE_E_INVALID_PARAMETER)
		{
		}
		else if (result == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			//-------------------------------------------------------
			// ���� �غ��� ���� �������� ���ڿ��� �߷��� ���, �� �߸��� ���� �α׸� �����.
			// �׸��� ������ ��ġ�� �����Ѵ�.
			//-------------------------------------------------------
			WCHAR mustSucMessage[BUFFER_SIZE] = { 0, };
			StringCchVPrintf(mustSucMessage, 255, L"���ڿ� ���� �������� ���ڿ��� �߷Ƚ��ϴ١�", va);
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"LogFail", L"ERROR", message);
		}

		switch (logLevel)
		{
		case eLogLevel::LOG_LEVEL_DEBUG:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s//%010lld] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, type, L"DEBUG", ++m_LogCount, message);
			break;
		case eLogLevel::LOG_LEVEL_ERROR:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s//%010lld] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, type, L"ERROR", ++m_LogCount, message);
			break;
		case eLogLevel::LOG_LEVEL_SYSTEM:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s//%010lld] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, type, L"SYSTEM", ++m_LogCount, message);
			break;
		}

		fclose(filePtr);
		
	}
#endif 
#if SERVER_ON_LOG== 0
	//--------------------------------------------------------
	// 	   �α� ���н�, �α׿� ���� �α׸� ����°�.
	//--------------------------------------------------------
	if (type == nullptr)
	{
		WCHAR fileName[MAX_PATH] = { 0, };
		HRESULT resultFile = StringCchPrintf(fileName, MAX_PATH, L"%s[20%d%02d].txt",L"LogFail", today.tm_year - 100, today.tm_mon + 1);

		//--------------------------------------------------------------
		// 	   File Open
		//--------------------------------------------------------------
		_wfopen_s(&filePtr, fileName, L"at,ccs=UTF-16LE");
		while (filePtr == nullptr)
		{
			_wfopen_s(&filePtr, fileName, L"at,ccs=UTF-16LE");
		}

		//--------------------------------------------------------------
		// 	  Safe String���� ���ڿ� ���� 
		//--------------------------------------------------------------
		HRESULT result = StringCchVPrintf(message, BUFFER_SIZE, stringFormat, va);

		if (result == STRSAFE_E_INVALID_PARAMETER)
		{
		}
		else if (result == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			//-------------------------------------------------------
			// ���� �غ��� ���� �������� ���ڿ��� �߷��� ���, �� �߸��� ���� �α׸� �����.
			// �׸��� ������ ��ġ�� �����Ѵ�.
			//-------------------------------------------------------
			WCHAR mustSucMessage[BUFFER_SIZE] = { 0, };
			StringCchVPrintf(mustSucMessage, 255, L"���ڿ� ���� �������� ���ڿ��� �߷Ƚ��ϴ١�", va);
			fwprintf(filePtr, L"[%s][%d/%d/%d %d:%d:%d][%s] %s\n", L"LogFail", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"ERROR", mustSucMessage);
		}

		switch (logLevel)
		{
		case eLogLevel::LOG_LEVEL_DEBUG:
			fwprintf(filePtr, L"[%s][%d/%d/%d %d:%d:%d][%s] %s\n", L"LogFail", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"DEBUG", message);
			break;
		case eLogLevel::LOG_LEVEL_ERROR:
			fwprintf(filePtr, L"[%s][%d/%d/%d %d:%d:%d][%s] %s\n", L"LogFail", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"ERROR", message);
			break;
		case eLogLevel::LOG_LEVEL_SYSTEM:
			fwprintf(filePtr, L"[%s][%d/%d/%d %d:%d:%d][%s] %s\n", L"LogFail", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"SYSTEM", message);
			break;
		}
		fclose(filePtr);
	}
	//--------------------------------------------------------
	// 	   �Ϲ����� �ý��� �α׸� ����°�.
	//--------------------------------------------------------
	else
	{
		//--------------------------------------------------------
		// 	  type�� �ִ� �α������� �˻��Ѵ� ������ �����߰��Ѵ�.
		//--------------------------------------------------------
		auto iter = m_FileNameMap.find(type);
		if (iter == m_FileNameMap.end())
		{
			WCHAR fileName[MAX_PATH] = { 0, };

			//-------------------------------------------------------
			// MAX_PATH �̹Ƿ� �����ϸ� �ȵ�.
			//-------------------------------------------------------
			HRESULT result = StringCchPrintf(fileName, MAX_PATH, L"%s%s[20%d%02d].txt", m_DirectoryName, type, today.tm_year - 100, today.tm_mon + 1);

			LogFileName* logfileNamePtr = new LogFileName;

			logfileNamePtr->_FileName = fileName;
			logfileNamePtr->_LastMonth = today.tm_mon + 1;

		
			m_FileNameMap.insert(std::make_pair(type, logfileNamePtr));

			iter = m_FileNameMap.find(type);
		}

		LogFileName* tempLogFileNamePtr = (*iter).second;

		//--------------------------------------------------------------
		// 	   ��(Month)�� ����Ǹ� LastMonth�� �������ְ� ���� �̸��� ���� �������ص�,
		//     ���� �������ؾߵȴ�.
		//--------------------------------------------------------------
		if (tempLogFileNamePtr->_LastMonth != today.tm_mon + 1)
		{
			WCHAR fileName[MAX_PATH] = { 0, };

			//-------------------------------------------------------
			// MAX_PATH �̹Ƿ� �����ϸ� �ȵ�.
			//-------------------------------------------------------
			HRESULT result = StringCchPrintf(fileName, MAX_PATH, L"%s%s[20%d%02d].txt", m_DirectoryName, type, today.tm_year - 100, today.tm_mon + 1);
			tempLogFileNamePtr->_FileName = fileName;
			tempLogFileNamePtr->_LastMonth = today.tm_mon + 1;
		}

		//--------------------------------------------------------------
		// 	   File Open
		//--------------------------------------------------------------
		const WCHAR* cstringFileName = tempLogFileNamePtr->_FileName.c_str();


		_wfopen_s(&filePtr, cstringFileName, L"at,ccs=UTF-16LE");
		while (filePtr == nullptr)
		{
			_wfopen_s(&filePtr, cstringFileName, L"at,ccs=UTF-16LE");
		}

		//--------------------------------------------------------------
		// 	  �α׳����� Safe String���� ���ڿ� ���� 
		//--------------------------------------------------------------
		HRESULT result = StringCchVPrintf(message, BUFFER_SIZE, stringFormat, va);

		if (result == STRSAFE_E_INVALID_PARAMETER)
		{
		}
		else if (result == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			//-------------------------------------------------------
			// ���� �غ��� ���� �������� ���ڿ��� �߷��� ���, �� �߸��� ���� �α׸� �����.
			// �׸��� ������ ��ġ�� �����Ѵ�.
			//-------------------------------------------------------
			WCHAR mustSucMessage[BUFFER_SIZE] = { 0, };
			StringCchVPrintf(mustSucMessage, 255, L"���ڿ� ���� �������� ���ڿ��� �߷Ƚ��ϴ١�", va);
			fwprintf(filePtr, L"[%s][%d/%d/%d %d:%d:%d][%s] %s\n", type, today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, L"ERROR", mustSucMessage);

		}

		//--------------------------------------------------------------
		// 	  ���� �α� ������, File�� ���پ� fwprintf�Ѵ�.
		//--------------------------------------------------------------
		switch (logLevel)
		{
		case eLogLevel::LOG_LEVEL_DEBUG:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s//%010lld] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, type, L"DEBUG", ++m_LogCount, message);
			break;
		case eLogLevel::LOG_LEVEL_ERROR:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s//%010lld] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, type, L"ERROR", ++m_LogCount, message);
			break;
		case eLogLevel::LOG_LEVEL_SYSTEM:
			fwprintf(filePtr, L"[%d/%d/%d %d:%d:%d][%s//%s//%010lld] %s\n", today.tm_year - 100, today.tm_mon + 1, today.tm_mday, today.tm_hour, today.tm_min, today.tm_sec, type, L"SYSTEM", ++m_LogCount, message);
			break;
		}

		fclose(filePtr);
		
	}
#endif
	ReleaseSRWLockExclusive(&m_Lock);
}

void SysLog::WriteLogHex(const WCHAR* type, eLogLevel logLevel, const WCHAR* string, BYTE* byte, int32_t byteLen)
{
	//-----------------------------------------------------------------------
	// 	   HexLog Title���� ex) RingBuffer : ByteLen[100] - 0xab 0xcd.....
	//-----------------------------------------------------------------------
	std::wstring str;
	WCHAR initStr[100] = { 0, };
	HRESULT result =StringCchPrintf(initStr, 100, L"%s:ByteLen[%d] -",string, byteLen);
	
	if (result == STRSAFE_E_INSUFFICIENT_BUFFER)
	{
		WriteLog(nullptr, SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Hex String �߸�");
	}

	str = initStr;


	for (int i = 0; i < byteLen; ++i)
	{
		WCHAR buffer[20] = { 0, };
		StringCchPrintf(buffer, 8, L" 0x%02x", *(byte +i));
		str += buffer;
	}

	WriteLog(type, logLevel, str.c_str());

}

void SysLog::SetDirectory(const WCHAR* directoryName)
{
	HRESULT result = StringCchPrintf(m_DirectoryName, MAX_PATH, L"./%s/",directoryName);

	if (-1 == _wmkdir(m_DirectoryName))
	{
		WriteLog(nullptr, eLogLevel::LOG_LEVEL_ERROR, L"Make Directory Fail :%d", GetLastError());
	}

}

void SysLog::SetLogLevel(eLogLevel logLevel)
{
	m_LogLevel = logLevel;
}
