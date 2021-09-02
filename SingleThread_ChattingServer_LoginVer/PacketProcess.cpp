#include "PacketProcess.h"
#include "SerializeBuffer.h"

void MakePacket_en_PACKET_CS_CHAT_RES_LOGIN(NetPacket* packet, uint16_t type, BYTE status, int64_t accountNo)
{
    *(packet) << type << status << accountNo;
}

void MakePacket_en_PACKET_CS_CHAT_RES_SECTOR_MOVE(NetPacket* packet, uint16_t type, int64_t accountNo, uint16_t sectorX, uint16_t sectorY)
{
    *(packet) << type << accountNo << sectorX << sectorY;
}


void MakePacket_en_PACKET_CS_CHAT_RES_MESSAGE(NetPacket* packet, uint16_t type, int64_t accountNo, WCHAR* id, WCHAR* nickName, uint16_t messageLen, WCHAR* message)
{
    *(packet) << type << accountNo;

    packet->PutData((char*)id, 20 * 2);
    packet->PutData((char*)nickName, 20 * 2);

    *(packet) << messageLen;
    packet->PutData((char*)message, messageLen * 2);
}