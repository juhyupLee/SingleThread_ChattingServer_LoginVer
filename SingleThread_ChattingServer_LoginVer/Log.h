#pragma once
#include <iostream>
#include <Windows.h>
#include <unordered_map>
#include <string>

//--------------------------------------------------------------------------------------------------------------------------------
//���μ��� ������ ���ÿ� ������ �����Ǹ�, ��� �α״� �ϳ��� ���Ͽ� �� ����Ǵ� ����̴�
// 0�̸� Type������ �α� ������ �����Ǹ�, ������ ���ŵȴ�. ���μ����� ����ǵ�, ������ ���� ���������ʰ�, �α����Ͽ� ��� �߰��Ѵ�
//--------------------------------------------------------------------------------------------------------------------------------
#define SERVER_ON_LOG 1
//--------------------------------------------------------------------------------------------------------------------------------						

#define _LOG  SysLog::GetInstance()

class SysLog
{
public:
	SysLog();
	~SysLog();

	struct LogFileName
	{
		std::wstring _FileName; //�α� ���� �̸� 
		int32_t _LastMonth;		//�� ������ �������ֱ����� ����
	};
	enum class eLogLevel
	{
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_SYSTEM
	};
	enum
	{
		BUFFER_SIZE = 1000
	};
	
public:
	static SysLog* GetInstance();
private:
	eLogLevel m_LogLevel;
	WCHAR m_ServerONLogFileName[MAX_PATH];
	
	WCHAR m_DirectoryName[MAX_PATH];
	//std::unordered_map<std::wstring, std::wstring> m_FileNameMap;
	std::unordered_map<std::wstring, LogFileName*> m_FileNameMap;

public:
	void WriteLog(const WCHAR* type, eLogLevel logLevel, const WCHAR* stringFormat, ...);
	void WriteLogHex(const WCHAR* type, eLogLevel logLevel, const WCHAR* string,BYTE* byte, int32_t byteLen);
	void SetDirectory(const WCHAR* directoryName);
	void SetLogLevel(eLogLevel logLevel);
	SRWLOCK m_Lock;
	uint64_t m_LogCount;

};

