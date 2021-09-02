#include "ChattingServer.h"
#include <process.h>
#include "Log.h"
#include "CommonProtocol.h"
#include "CPUUsage.h"
#include "MonitorPDH.h"
#include "PacketProcess.h"

CPUUsage g_CPUUsage;
MonitorPDH g_MonitorPDH;

RedisConnector* MyChattingServer::m_RedisCon = nullptr;

MyChattingServer::MyChattingServer()
    :NetServer(this),
    m_JobPool(500),
    m_ClientPool(500,true)
{
    wcscpy_s(m_BlackIPList[0], L"185.216.140.27"); // 네덜란드 블랙 IP

    wcscpy_s(m_WhiteIPList[0], L"127.0.0.1"); // 루프팩
    wcscpy_s(m_WhiteIPList[1], L"10.0.1.2"); // Unit 1
    wcscpy_s(m_WhiteIPList[2], L"10.0.2.2"); // unit 1

   
    m_UpdateTPS = 0;

   m_MaxTCPRetrans =0;
   m_Min_MaxTCPRetrans=INT64_MAX;

   m_WakeTime = 0;

}

MyChattingServer::~MyChattingServer()
{
}

void MyChattingServer::ServerMonitorPrint()
{
    g_CPUUsage.UpdateCPUTime();
    g_MonitorPDH.ReNewPDH();

    wprintf(L"==========OS Resource=========\n");
    wprintf(L"UpdateThread SleepTime :%d\n", m_WakeTime);
    wprintf(L"HardWare U:%.1f  K:%.1f  Total:%.1f\n", g_CPUUsage.ProcessorUser(), g_CPUUsage.ProcessorKernel(), g_CPUUsage.ProcessorTotal());
    wprintf(L"Process U:%.1f  K:%.1f  Total:%.1f\n", g_CPUUsage.ProcessUser(), g_CPUUsage.ProcessKernel(), g_CPUUsage.ProcessTotal());
    wprintf(L"Private Mem :%lld[K]\n", g_MonitorPDH.GetPrivateMemory() / 1024);
    wprintf(L"NP Mem :%lld[K]\n", g_MonitorPDH.GetNPMemory() / 1024);
    wprintf(L"Available Mem :%.1lf[G]\n", g_MonitorPDH.GetAvailableMemory() / 1024);

    int64_t retransValue = g_MonitorPDH.GetTCPRetrans();
    if (m_MaxTCPRetrans < retransValue)
    {
        m_MaxTCPRetrans = retransValue;
    }
    if (m_Min_MaxTCPRetrans > retransValue)
    {
        m_Min_MaxTCPRetrans = retransValue;
    }
    wprintf(L"TCP Retransmit/sec  :%lld\n", retransValue);
    wprintf(L"Max TCP Retransmit/sec  :%lld\n", m_MaxTCPRetrans);
    wprintf(L"Min TCP Retransmit/sec  :%lld\n", m_Min_MaxTCPRetrans);


    wprintf(L"==========TPS=========\n");
    wprintf(L"(1) Accept TPS : %d\n(2) Update TPS:%d\n(3) Send TPS:%d\n(4) Recv TPS:%d\n"
        , GetAcceptTPS()
        , m_UpdateTPS
        , GetSendTPS()
        , GetRecvTPS());

    wprintf(L"==========Memory=========\n");
    wprintf(L"(1) Packet PoolAlloc:%d \n(2) Packet Pool Count :%d \n(3) Packet Pool Use Count :%d \n(4) Client Pool Alloc:%d \n(5) Client Pool Count:%d \n(6) Client Use Count:%d \n(7) LockFreeQ Memory:%d \n(8) LockFreeStack Memory:%d \n(9) Update Q Alloc Memory:%d\n(10)Alloc Memory Count:%d\n(11)Free Memory Count:%d\n"
        , NetPacket::GetMemoryPoolAllocCount()
        , NetPacket::GetPoolCount()
        , NetPacket::GetUseCount()
        , m_ClientPool.GetChunkCount()
        , m_ClientPool.GetPoolCount()
        , m_ClientPool.GetUseCount()
        , GetSendQMeomryCount()
        , GetLockFreeStackMemoryCount()
        , m_ThreadQ.GetMemoryPoolAllocCount()
        , g_AllocMemoryCount
        , g_FreeMemoryCount);

    wprintf(L"==========Network =========\n");
    wprintf(L"(1) NetworkTraffic(Send) :%d \n(2) TCP TimeOut ReleaseCount :% d \n"
        , GetNetworkTraffic()
        , g_TCPTimeoutReleaseCnt);


    wprintf(L"==========Count ===========\n");
    wprintf(L"(1) Accept Count:%lld\t(2) Disconnect Count :%d\n(3) Session Count:%d\t(4) Client Count:%llu\n(5) UpdateThread Q Count:%d\n"
        , GetAcceptCount()
        , g_DisconnectCount
        , m_SessionCount
        , m_ClientMap.size()
        , m_ThreadQ.GetQCount());
        
    wprintf(L"==============================\n");

    if (GetAsyncKeyState(VK_F2))
    {
        for (DWORD i = 0; i < m_MaxUserCount; ++i)
        {
            wprintf(L"Session[%d] SendQ UsedSize:%d\n", i, m_SessionArray[i]._SendQ.GetQCount());
        }
    }
    m_UpdateTPS = 0;
    //g_FreeMemoryCount = 0;
    //g_AllocMemoryCount = 0;

  /*  InterlockedExchange(&g_FreeMemoryCount, 0);
    InterlockedExchange(&g_AllocMemoryCount, 0);*/
}

bool MyChattingServer::OnConnectionRequest(WCHAR* ip, uint16_t port)
{
    for (int i = 0; i < MAX_WHITE_IP_COUNT; ++i)
    {
        // White IP와 같으면 바로 Return true
        if (!wcscmp(m_WhiteIPList[i], ip))
        {
            return true;
        }
    }
    _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_SYSTEM, L"외부 IP:%s", ip);

    return false;
}

void MyChattingServer::OnClientJoin(uint64_t sessionID, WCHAR* ip, uint16_t port)
{
#if MEMORYLOG_USE ==1 
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::ON_CLIENT_JOIN, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::ON_CLIENT_JOIN, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
    curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

    MakeJobNEnQ(eJobTYPE::ON_CLIENT_JOIN, sessionID, nullptr);
}

void MyChattingServer::OnClientLeave(uint64_t sessionID)
{
#if MEMORYLOG_USE ==1 
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::ON_CLIENT_LEAVE, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::ON_CLIENT_LEAVE, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
    curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

    MakeJobNEnQ(eJobTYPE::ON_CLIENT_LEAVE, sessionID, nullptr);
}

void MyChattingServer::OnRecv(uint64_t sessionID, NetPacket* packet)
{
 
#if MEMORYLOG_USE ==1 
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::ON_RECV, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::ON_RECV, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
    curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

    MakeJobNEnQ(eJobTYPE::ON_MESSAGE, sessionID, packet);
    
}

void MyChattingServer::OnError(int errorcode, WCHAR* errorMessage)
{


}
void MyChattingServer::OnTimeOut(uint64_t sessionID)
{
    MakeJobNEnQ(eJobTYPE::ON_TIMEOUT, sessionID, nullptr);
}
MyChattingServer::Client* MyChattingServer::FindClient(uint64_t sessionID)
{
    Client* findClient = nullptr;

    auto iter = m_ClientMap.find(sessionID);
    if (iter == m_ClientMap.end())
    {
        return nullptr;
    }
    findClient = (*iter).second;

    return findClient;
}


unsigned int __stdcall MyChattingServer::RedisAPCThread(LPVOID param)
{
    while (true)
    {
        SleepEx(INFINITE, TRUE);
    }
}
void CALLBACK MyChattingServer::APCCompleteCallBack(ULONG_PTR param)
{
    //------------------------------------------------------
    // APC Q에 인큐가되면, 레디스 검색을 하고,
    // 그 검색 결과에따라서 클라이언트에게 보낼 로그인 결과 패킷을만들고
    // Job을 만들어 Update 스레드에게 Job을 던진다.
    //------------------------------------------------------
    RedisJob* redisJob = (RedisJob*)param;
    std::string temp = std::to_string(redisJob->_AccoutAno);
    MyChattingServer* chatServer = redisJob->_ChatServer;

    cpp_redis::reply result = m_RedisCon->Get(temp);

    BYTE status = -1;
    
    NetPacket* packet = NetPacket::Alloc();
    
    if (result.is_null())
    {
        status = 0;
    }
    else
    {
        if (result.as_string() != redisJob->_SessionKey)
        {
            status = 0;
        }
        else
        {   //--------------------------------------------
            // 로그인 성공
            //--------------------------------------------
            status = 1;
        }
    }
    MakePacket_en_PACKET_CS_CHAT_RES_LOGIN(packet, en_PACKET_CS_CHAT_RES_LOGIN, status, redisJob->_AccoutAno);
    chatServer->MakeJobNEnQ(MyChattingServer::eJobTYPE::ON_LOGIN_RESULT, redisJob->_SessionID, packet);

    delete redisJob;


}

unsigned  __stdcall MyChattingServer::UpdateThread(LPVOID param)
{
    MyChattingServer* chatServer = (MyChattingServer*)param;
    DWORD lastWakeTime = 0;
    DWORD diffTime = 0;

    while (true)
    {
        lastWakeTime = timeGetTime();
        DWORD rtnWait = WaitForMultipleObjects(2,chatServer->m_EventUpdateThread,FALSE, INFINITE);
      
        chatServer->m_WakeTime = timeGetTime() - lastWakeTime;

        if (rtnWait == WAIT_FAILED)
        {
            _LOG->WriteLog(L"ChattingServer",SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Wait Fail UpdateThread:%d", GetCurrentThreadId());
            chatServer->Crash();
        }
        //-----------------------------------------------------
        // 종료 신호 왔을시,  Update 스레드종료
        //-----------------------------------------------------
        if (rtnWait - WAIT_OBJECT_0 == 0)
        {
            break;
        }
        Job* jobPtr;

        while (chatServer->m_ThreadQ.DeQ(&jobPtr))
        {
#if MEMORYLOG_USE ==1 
            Session* curSession = chatServer->FindSession(jobPtr->_SessionID);
            IOCP_Log log;
            log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::UPDATETHREAD_JOB_SWITCH, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, jobPtr->_SessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag, (int32_t)jobPtr->_TYPE, chatServer->m_ThreadQ.GetQCount());
            g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
            Session* curSession = chatServer->FindSession(jobPtr->_SessionID);
            IOCP_Log log;
            log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::UPDATETHREAD_JOB_SWITCH, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, jobPtr->_SessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag, (int32_t)jobPtr->_TYPE, chatServer->m_ThreadQ.GetQCount());
            g_MemoryLog_IOCP.MemoryLogging(log);
            curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif
            switch (jobPtr->_TYPE)
            {
            case eJobTYPE::ON_CLIENT_JOIN:
                chatServer->CreateNewUser(jobPtr->_SessionID);
                break;
            case eJobTYPE::ON_CLIENT_LEAVE:
                chatServer->DeleteUser(jobPtr->_SessionID);
                break;
            case eJobTYPE::ON_MESSAGE:
                chatServer->MessageMarshalling(jobPtr->_SessionID, jobPtr->_NetPacketPtr);
                break;
            case eJobTYPE::ON_TIMEOUT:
                chatServer->Disconnect(jobPtr->_SessionID);
                break;
            case eJobTYPE::ON_LOGIN_RESULT:
                chatServer->SendUnicast(jobPtr->_SessionID, jobPtr->_NetPacketPtr);
                break;
            }
            chatServer->m_JobPool.Free(jobPtr);

            chatServer->m_UpdateTPS++;
        }

    }
    _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_SYSTEM, L"Update[Chatting] Thread[%d] 종료", GetCurrentThreadId());
    return 0;
}


bool MyChattingServer::ChattingServerStart(WCHAR* ip, uint16_t port, DWORD runningThread, SocketOption& option, DWORD workerThreadCount, DWORD maxUserCount, TimeOutOption& timeOutOption)
{

    //--------------------------------------------------------------------
    // Redis Connector 생성 및 Connect
    //--------------------------------------------------------------------

    m_RedisCon = new RedisConnector();
    while (!m_RedisCon->Connect(REDIS_IP, REDIS_PORT))
    {

    }

    m_EventUpdateThread[0] = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!m_EventUpdateThread)
    {
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Event Create Fail:%d", GetCurrentThreadId());
    }


    m_EventUpdateThread[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!m_EventUpdateThread)
    {
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Event Create Fail:%d", GetCurrentThreadId());
    }

    //-------------------------------------------------------------------
    // Update Thread 가동
    //-------------------------------------------------------------------
    m_UpdateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, NULL);

    //-------------------------------------------------------------------
    // 워커스레드, Accept Thread  , Monitoring 스레드 가동
    //-------------------------------------------------------------------
    ServerStart(ip,port, runningThread,option,workerThreadCount,maxUserCount, timeOutOption);



    return true;
}

bool MyChattingServer::ChattingServerStop()
{
    //-------------------------------------------------------------------
    // Update Thread 중지
    //-------------------------------------------------------------------
    ServerStop();

    //------------------------------------------------------
    //Update 스레드 종료
    //------------------------------------------------------
    SetEvent(m_EventUpdateThread[0]);
    DWORD waitRtn = WaitForSingleObject(m_UpdateThread, INFINITE);
    CloseHandle(m_UpdateThread);

    //--------------------------------------------------------------------
    // Redis Connector 연결 종료  및 Delete : Update스레드쪽이 완전히 끝난뒤 연결을 종료함
    //--------------------------------------------------------------------
    m_RedisCon->Disconnect();
    delete m_RedisCon;

    CloseHandle(m_EventUpdateThread[0]);
    CloseHandle(m_EventUpdateThread[1]);

    return true;
}

void MyChattingServer::CreateNewUser(uint64_t sessionID)
{
#if MEMORYLOG_USE ==1 
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::CREATE_NEW_CLIENT, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::CREATE_NEW_CLIENT, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
    curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

    Client* newClient = m_ClientPool.Alloc();

    newClient->_SessionID = sessionID;
    m_ClientMap.insert(std::make_pair(sessionID, newClient));

}

void MyChattingServer::DeleteUser(uint64_t sessionID)
{
    InterlockedIncrement(&g_OnClientLeaveCall);

#if MEMORYLOG_USE ==1 
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::DELETE_CLIENT, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::DELETE_CLIENT, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag);
    g_MemoryLog_IOCP.MemoryLogging(log);
    curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

    auto iter = m_ClientMap.find(sessionID);
    if (iter == m_ClientMap.end())
    {
        Crash();
    }

    Client* delClient = (*iter).second;
    m_ClientMap.erase(iter);

    //---------------------------------------------------------
    // 로그인패킷을 받기전 or 섹터이동을 받기전에 연결이끊겼을 때
    // 섹터탐색 없이 그대로 메모리 지우고 return
    //---------------------------------------------------------
    if (delClient->_SectorY<0 || delClient->_SectorY >dfSECTOR_Y_MAX - 1 ||
        delClient->_SectorX<0 || delClient->_SectorX >dfSECTOR_X_MAX - 1 )
    {
        m_ClientPool.Free(delClient);
        return;
    }


    //---------------------------------------------------------
    // 해당 Client가있는 섹터를 탐색한뒤 그 섹터 리스트에서 노드제거한다
    //---------------------------------------------------------

    auto iterSector = m_Sector[delClient->_SectorY][delClient->_SectorX].begin();
    auto iter_end = m_Sector[delClient->_SectorY][delClient->_SectorX].end();

    for (; iterSector != iter_end;++iterSector)
    {
        if (*iterSector == delClient)
        {
            m_Sector[delClient->_SectorY][delClient->_SectorX].erase(iterSector);
            break;
        }
    }

    m_ClientPool.Free(delClient);
}

void MyChattingServer::MessageMarshalling(uint64_t sessionID, NetPacket* netPacket)
{
    WORD type;
    if ((*netPacket).GetPayloadSize() < sizeof(WORD))
    {
        Crash();
    }

    (*netPacket) >> type;
    

#if MEMORYLOG_USE ==1 
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::MESSAGE_MARSHAL, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag, (int32_t)2, -1, (eRecvMessageType)type, -1);
    g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
    Session* curSession = FindSession(sessionID);
    IOCP_Log log;
    log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::MESSAGE_MARSHAL, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag, (int32_t)2, -1, (eRecvMessageType)type, -1);
    g_MemoryLog_IOCP.MemoryLogging(log);
    curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

    switch (type)
    {
    case en_PACKET_CS_CHAT_REQ_LOGIN:
        PacketProcess_en_PACKET_CS_CHAT_REQ_LOGIN(sessionID, netPacket);
        break;
    case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
        PacketProcess_en_PACKET_CS_CHAT_REQ_SECTOR_MOVE(sessionID, netPacket);
        break;
    case en_PACKET_CS_CHAT_REQ_MESSAGE:
        PacketProcess_en_PACKET_CS_CHAT_REQ_MESSAGE(sessionID, netPacket);
        break;
    case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
        Crash();
        break;
    default:
#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"메시지마샬링: 존재하지않는 메시지타입 [Session ID:%llu] [Type:%d]", curSession->_ID, type);
#endif
        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);
        break;
    }
}


void MyChattingServer::MakeJobNEnQ(eJobTYPE type, uint64_t sessionID, NetPacket* netPacket)
{
    Job* jobPtr = m_JobPool.Alloc();

    jobPtr->_TYPE = type;
    jobPtr->_SessionID = sessionID;
    jobPtr->_NetPacketPtr = netPacket;

    m_ThreadQ.EnQ(jobPtr);
    SetEvent(m_EventUpdateThread[1]);
}


void MyChattingServer::PacketProcess_en_PACKET_CS_CHAT_REQ_LOGIN(uint64_t sessionID, NetPacket* netPacket)
{
    Client* client = FindClient(sessionID);

    if (client == nullptr)
    {
        Crash();
    }
    if (netPacket->GetPayloadSize() != sizeof(int64_t) + sizeof(WCHAR) * 20 + sizeof(WCHAR) * 20 + sizeof(char) * 64)
    {

#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"LoginPacket: 직렬화패킷 규격에 안맞는 메시지 [Session ID:%llu] [PayloadSize:%d]", sessionID, netPacket->GetPayloadSize());
#endif
        netPacket->Free(netPacket);
        InterlockedIncrement(&g_FreeMemoryCount);
        Disconnect(sessionID);
        return; 
    }
    (*netPacket) >> client->_AccoutNo;
    (*netPacket).GetData((char*)client->_ID, sizeof(WCHAR) * 20);
    (*netPacket).GetData((char*)client->_NickName, sizeof(WCHAR) * 20);
    (*netPacket).GetData(client->_SessionKey, 64);

    netPacket->Clear();

    if (netPacket->DecrementRefCount() == 0)
    {
        netPacket->Free(netPacket);
    }

    RedisJob* redisJob = new RedisJob;
    redisJob->_ChatServer = this;
    redisJob->_AccoutAno = client->_AccoutNo;
    redisJob->_SessionID = sessionID;
    memcpy(redisJob->_SessionKey, client->_SessionKey, sizeof(redisJob->_SessionKey));

    QueueUserAPC(APCCompleteCallBack, RedisAPCThread, (ULONG_PTR)redisJob);

    //----------------------------------------
    // For debug
    //----------------------------------------
    Session* tempSession = FindSession(sessionID);
    tempSession->_AccountNo = client->_AccoutNo;


    MakePacket_en_PACKET_CS_CHAT_RES_LOGIN(netPacket, en_PACKET_CS_CHAT_RES_LOGIN, 1, client->_AccoutNo);
    SendUnicast(sessionID, netPacket);

    //SendNDiscon(sessionID, netPacket);
    //SetHeartBeat(sessionID);
}


void MyChattingServer::PacketProcess_en_PACKET_CS_CHAT_REQ_SECTOR_MOVE(uint64_t sessionID, NetPacket* netPacket)
{
    Client* client = FindClient(sessionID);

    if (client == nullptr)
    {
        Crash();
    }

    if (netPacket->GetPayloadSize() != sizeof(int64_t) + sizeof(WORD)+sizeof(WORD))
    {
#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SectorMove: 직렬화패킷 규격에 안맞는 메시지 [Session ID:%llu] [PayloadSize:%d]", sessionID, netPacket->GetPayloadSize());
#endif
        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);
    
        return;
    }

    int64_t accountNo;

    (*netPacket) >> accountNo;


    if (accountNo != client->_AccoutNo)
    {
#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"SectorMove: Abnormal Account 번호 [Session ID:%llu][Cur Account Num:%d] [In Account Num:%d]", sessionID, client->_AccoutNo,accountNo);
#endif
        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);

        return;
    }
    //--------------------------------------------------------
    // 이전에 있던 섹터에서 Client 를 지우고 새롭게 갱신해준다.
    //--------------------------------------------------------

    if (client->_SectorX != 0xffff && client->_SectorY != 0xffff)
    {
        auto iter = m_Sector[client->_SectorY][client->_SectorX].begin();
        auto iter_end = m_Sector[client->_SectorY][client->_SectorX].end();

        for (; iter != iter_end; ++iter)
        {
            if (*iter == client)
            {
                m_Sector[client->_SectorY][client->_SectorX].erase(iter);
                break;
            }
        }
    }
  


    (*netPacket) >> client->_SectorX >> client->_SectorY;

    if (0 > client->_SectorX || client->_SectorX > dfSECTOR_X_MAX - 1
        || 0 > client->_SectorY || client->_SectorY > dfSECTOR_Y_MAX - 1)
    {
        Crash();
        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);

        return; 
    }


    m_Sector[client->_SectorY][client->_SectorX].push_back(client);



    //--------------------------------------------------------
    // 확인용 메시지를 Client에 전송.
    //--------------------------------------------------------
    netPacket->Clear();
    
    MakePacket_en_PACKET_CS_CHAT_RES_SECTOR_MOVE(netPacket, en_PACKET_CS_CHAT_RES_SECTOR_MOVE, client->_AccoutNo, client->_SectorX, client->_SectorY);
    SendUnicast(sessionID, netPacket);

}



void MyChattingServer::SendSector(uint16_t sectorX, uint16_t sectorY, NetPacket* packet)
{
    //-----------------------------------------
    // Sector 주변 9개 섹터에 Send하기
    //-----------------------------------------

    if (sectorY > dfSECTOR_Y_MAX - 1 || sectorY < 0)
    {
        Crash();
    }

    if (sectorX > dfSECTOR_X_MAX - 1 || sectorX < 0)
    {
        Crash();
    }
    int16_t leftUpX = sectorX - 1;
    int16_t leftUpY = sectorY - 1;

    int16_t rightDnX = sectorX + 1;
    int16_t rightDnY = sectorY + 1;

    int sendCount = 0;

    for (int16_t y = leftUpY; y <= rightDnY; ++y)
    {
        if (y > dfSECTOR_Y_MAX - 1 || y < 0)
        {
            continue;
        }
        for (int16_t x = leftUpX; x <= rightDnX; ++x)
        {
            if (x > dfSECTOR_X_MAX - 1 || x< 0 )
            {
                continue;
            }
            auto iter = m_Sector[y][x].begin();
            auto iter_end = m_Sector[y][x].end();
            
            for (; iter != iter_end; ++iter)
            {
                Client* client = *iter;
                uint64_t tempSessionID = client->_SessionID;
                packet->IncrementRefCount();

                if (packet->GetPayloadSize() <= 0)
                {
                    Crash();
                }
                //--------------------------------------------------------
                // SendPacket에 실패한거는, AcqurieSession에 실패한것이다
                // 그렇다면 PostQ를 해도 실패할것이다.
                //--------------------------------------------------------
                if (!SendPacket(tempSessionID, packet))
                {
                    continue;
                }
                PostQueuedCompletionStatus(m_IOCP, -1, NULL, (LPOVERLAPPED)tempSessionID);

                //SendPost(tempSessionID);

                sendCount++;
            }
        }
    }
    

   
    bool bFree = false;

    //wprintf(L"Player Sector[X:%d, Y:%d] SendCount:%d\n", sectorX, sectorY,sendCount);
    //_LOG(LOG_LEVEL_ERROR, L"SendSector Packet Decremetnt(%d->%d)", packet->m_RefCount, packet->m_RefCount - 1);
    if (packet->DecrementRefCount() == 0)
    {
        InterlockedIncrement(&g_FreeMemoryCount);
        bFree = true;
        packet->Free(packet);
    }

    //-----------------------------------------------
    // For Debug
    // 메시지를 보내지도않았는데 ,직렬화패킷이 반납안되면 오류
    //-----------------------------------------------
    if (sendCount == 0 &&bFree== false)
    {
        Crash();
    }
 
  
}


void MyChattingServer::PacketProcess_en_PACKET_CS_CHAT_REQ_MESSAGE(uint64_t sessionID, NetPacket* netPacket)
{
    Client* client = FindClient(sessionID);

    if (client == nullptr)
    {
        Crash();
    }

    if (netPacket->GetPayloadSize() < sizeof(int64_t) + sizeof(WORD))
    {
        
#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"Req Message: 직렬화패킷 규격에 안맞는 메시지 [Session ID:%llu] [PayloadSize:%d]", sessionID, netPacket->GetPayloadSize());
#endif
        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);
        return;
    }


    int64_t accountNo;

    (*netPacket) >> accountNo;

    if (accountNo != client->_AccoutNo)
    {
#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"ReQ Message: Abnormal Account 번호 [Session ID:%llu][Cur Account Num:%d] [In Account Num:%d]", sessionID, client->_AccoutNo, accountNo);
#endif

        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);

        return;
    }

    uint16_t messageLen;

    (*netPacket) >> messageLen;


    if (netPacket->GetPayloadSize() < messageLen)
    {
        
#if DISCONLOG_USE ==1
        _LOG->WriteLog(L"ChattingServer", SysLog::eLogLevel::LOG_LEVEL_ERROR, L"ReQ Message: 보낸 채팅메시지길이만큼 안옴 [Session ID:%llu][MessageLen:%d] [Payload Size%d]", sessionID, messageLen, netPacket->GetPayloadSize());
#endif
        InterlockedIncrement(&g_FreeMemoryCount);
        netPacket->Free(netPacket);
        Disconnect(sessionID);
        return;
    }

    WCHAR tempMessage[5000] = { 0, };

    (*netPacket).GetData((char*)tempMessage, messageLen);


    //-------------------------------------------------------------
    // // ForDebug
    // 더미클라이언트에선, 끊기전에 '=' 메시지를 보냄
    //-------------------------------------------------------------
    if (!wcscmp(tempMessage, L"="))
    {
        Session* findSession = nullptr;
        if (AcquireSession(sessionID, &findSession))
        {
            findSession->_bReserveDiscon = true;

#if MEMORYLOG_USE ==1 
            Session* curSession = FindSession(sessionID);
            IOCP_Log log;
            log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::MESSAGE_MARSHAL, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag, (int32_t)2, -1, (eRecvMessageType)type, -1);
            g_MemoryLog_IOCP.MemoryLogging(log);
#endif

#if MEMORYLOG_USE  ==2
            Session* curSession = FindSession(sessionID);
            IOCP_Log log;
            log.DataSettiong(InterlockedIncrement64(&g_IOCPMemoryNo), eIOCP_LINE::DISCON_RELEASE, GetCurrentThreadId(), curSession->_Socket, curSession->_IOCount, (int64_t)curSession, sessionID, (int64_t)&curSession->_RecvOL, (int64_t)&curSession->_SendOL, curSession->_SendFlag, (int32_t)2, -1, (eRecvMessageType)1, -1);
            g_MemoryLog_IOCP.MemoryLogging(log);
            curSession->_MemoryLog_IOCP.MemoryLogging(log);
#endif

            if (0 == InterlockedDecrement(&curSession->_IOCount))
            {
                ReleaseSession(curSession);
            }
        }

    }
    netPacket->Clear();

    if (netPacket->m_bSetHeader)
    {
        Crash();
    }
    MakePacket_en_PACKET_CS_CHAT_RES_MESSAGE(netPacket, en_PACKET_CS_CHAT_RES_MESSAGE, client->_AccoutNo, client->_ID, client->_NickName, messageLen, tempMessage);

    SendSector(client->_SectorX, client->_SectorY, netPacket);

}
