#pragma once
#include "NetServerLib.h"
#include <list>
#include "RedisConnector.h"

#define REDIS_IP "127.0.0.1"
#define REDIS_PORT 6379

class MyChattingServer : public NetServer
{
public:

	enum class eJobTYPE
	{
		ON_CLIENT_JOIN,
		ON_CLIENT_LEAVE,
		ON_MESSAGE,
		ON_TIMEOUT,
		ON_LOGIN_RESULT
	};
	struct Job
	{
		eJobTYPE _TYPE;
		int64_t _SessionID;
		NetPacket* _NetPacketPtr;
	};

	struct RedisJob
	{
		MyChattingServer* _ChatServer;
		int64_t _SessionID;
		int64_t _AccoutAno;
		char _SessionKey[64];
	};
	enum
	{
		MAX_IP_COUNT = 100,
		MAX_WHITE_IP_COUNT = 3

	};
	struct Client
	{
		Client()
			:_ID{0,}, _NickName{0,}, _SessionKey{0,}
		{
			_SessionID = 0;
			_SectorX = 0xffff;
			_SectorY = 0xffff;
			_AccoutNo = 0;

		}
		uint64_t _SessionID;
		uint16_t _SectorX;
		uint16_t _SectorY;

		int64_t _AccoutNo;
		WCHAR _ID[20];
		WCHAR _NickName[20];
		char _SessionKey[64];
		//WCHAR _IP[17];
		//uint16_t _Port;

	};

	struct Black_IP
	{
		WCHAR _IP[17];
		uint16_t _Port;
	};
	struct White_IP
	{
		WCHAR _IP[17];
		uint16_t _Port;
	};

public:
	MyChattingServer();
	~MyChattingServer();
	void ServerMonitorPrint();

public:
	bool ChattingServerStart(WCHAR* ip, uint16_t port, DWORD runningThread, SocketOption& option, DWORD workerThreadCount, DWORD maxUserCount, TimeOutOption& timeOutOption);
	bool ChattingServerStop();
public:

	//------------------------------------------
	// Contentes
	//------------------------------------------
	virtual bool OnConnectionRequest(WCHAR* ip, uint16_t port);
	virtual void OnClientJoin(uint64_t sessionID, WCHAR* ip, uint16_t port);
	virtual void OnClientLeave(uint64_t sessionID);
	virtual void OnRecv(uint64_t sessionID, NetPacket* packet);
	virtual void OnError(int errorcode, WCHAR* errorMessage);
	virtual void OnTimeOut(uint64_t sessionID);

private:
	//-----------------------------------------------------
	// 컨텐츠에서 생성하는 스레드들.
	//-----------------------------------------------------
	static unsigned int __stdcall UpdateThread(LPVOID param);
	static unsigned int __stdcall RedisAPCThread(LPVOID param);
	static void CALLBACK APCCompleteCallBack(ULONG_PTR param);
	static RedisConnector* m_RedisCon;

private:
	Client* FindClient(uint64_t sessionID);

	void CreateNewUser(uint64_t sessionID);
	void DeleteUser(uint64_t sessionID);

	void MessageMarshalling(uint64_t sessionID, NetPacket* netPacket);

	void MakeJobNEnQ(eJobTYPE type, uint64_t sessionID, NetPacket* netPacket);

	void PacketProcess_en_PACKET_CS_CHAT_REQ_LOGIN(uint64_t sessionID, NetPacket* netPacket);
	void PacketProcess_en_PACKET_CS_CHAT_REQ_SECTOR_MOVE(uint64_t sessionID, NetPacket* netPacket);
	void PacketProcess_en_PACKET_CS_CHAT_REQ_MESSAGE(uint64_t sessionID, NetPacket* netPacket);

	void SendSector(uint16_t sectorX, uint16_t sectorY, NetPacket* packet);

	
public:

private:
	MemoryPool_TLS<Job> m_JobPool;
	LockFreeQ<Job*> m_ThreadQ;
	MemoryPool_TLS<Client> m_ClientPool;

	std::unordered_map<uint64_t, Client*> m_ClientMap;
	std::list<Client*>m_Sector[dfSECTOR_Y_MAX][dfSECTOR_X_MAX];

	HANDLE m_UpdateThread;
	HANDLE m_EventUpdateThread[2];
	WCHAR m_BlackIPList[MAX_IP_COUNT][17];
	WCHAR m_WhiteIPList[MAX_WHITE_IP_COUNT][17];

	//---------------------------------------------------------
	// 	   For Debug
	//---------------------------------------------------------
	LONG m_UpdateTPS;


	int64_t m_MaxTCPRetrans ;
	int64_t m_Min_MaxTCPRetrans;

	DWORD m_WakeTime;

	
};
