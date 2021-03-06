
#include "ChattingServer.h"
#include <conio.h>
#include "MemoryDump.h"
#include "Global.h"
#include "Log.h"

CrashDump dump;

int main()
{
	MyChattingServer* chattingServer = new  MyChattingServer();
	TimeOutOption timeOutOption;
	
	SocketOption sockOption;
	timeOutOption._LoginTimeOut = 20000;
	timeOutOption._HeartBeatTimeOut = 60000;
	timeOutOption._OptionOn = false;

	sockOption._KeepAliveOption.onoff = 0;
	sockOption._Linger = true;
	sockOption._TCPNoDelay = false;
	sockOption._SendBufferZero = false;
	bool bServerStartFlag = false;

	int threadNum = 0;
	int maxUser = 0;
	int runningThread = 0;
	
	wprintf(L"워커스레드 갯수:");
	wscanf_s(L"%d", &threadNum);

	wprintf(L"러닝 스레드 갯수:");
	wscanf_s(L"%d", &runningThread);

	wprintf(L"최대 유저수:");
	wscanf_s(L"%d", &maxUser);


	chattingServer->ChattingServerStart(nullptr, 12001, runningThread, sockOption, threadNum, maxUser, timeOutOption);
	bServerStartFlag = true;


	while (true)
	{
		if (GetAsyncKeyState(VK_F4))
		{
			break;
		}
		if (_kbhit())
		{
			WCHAR temp = _getwch();

			if (temp == L'Q' || temp == L'q')
			{
				if (bServerStartFlag)
				{
					bServerStartFlag = false;
					chattingServer->ChattingServerStop();
				}
				else
				{
					wprintf(L"이미 서버가 종료되었습니다\n");
				}
			}
			if (temp == L's' || temp == L'S')
			{
				if (!bServerStartFlag)
				{
					bServerStartFlag = true;
					chattingServer->ChattingServerStart(nullptr, 12001, runningThread, sockOption, threadNum, maxUser, timeOutOption);
				}
				else
				{
					wprintf(L"서버가 이미 가동중입니다\n");
				}
				
				
			}
		}
		chattingServer->ServerMonitorPrint();
		Sleep(999);
	}

	if (bServerStartFlag)
	{
		chattingServer->ServerStop();
	}
	delete chattingServer;

	return 0;

}