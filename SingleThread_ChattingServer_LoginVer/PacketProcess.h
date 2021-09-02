#pragma once
#include <iostream>
#include <Windows.h>

class NetPacket;

void MakePacket_en_PACKET_CS_CHAT_RES_LOGIN(NetPacket* packet, uint16_t type, BYTE status, int64_t accountNo);

void MakePacket_en_PACKET_CS_CHAT_RES_SECTOR_MOVE(NetPacket* packet, uint16_t type, int64_t accountNo, uint16_t sectorX, uint16_t sectorY);

void MakePacket_en_PACKET_CS_CHAT_RES_MESSAGE(NetPacket* packet, uint16_t type, int64_t accountNo, WCHAR* id, WCHAR* nickName, uint16_t messageLen, WCHAR* message);
