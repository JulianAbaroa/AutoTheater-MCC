#pragma once

#include <iostream>
#include <cstdint>
#include <cstddef>
#include <vector>

#pragma pack(push, 1)

struct SubData_Identity
{
	uint64_t idLow;
	uint64_t idHigh;
	char nameString[16];
	uint8_t isRegistered;
};

struct FrameStatus_Struct
{
	uint32_t sessionID;
	uint16_t regionCode;
	uint16_t statusCode;
	uint64_t networkToken;
};

struct EntityRecord_Struct
{
	int16_t updateType;
	int16_t networkID;
	uint32_t entityFlags;
	float position[3];
	float direction[3];
	uint32_t field0x20;
	uint64_t timestamp;
	uint8_t configData[32]; 
};

struct ConfigData_Struct
{
	std::byte reservedPadding[16];
	std::byte field0x10;
	std::byte primaryStat;
	std::byte secondaryStat;
	std::byte configType;
	short itemOrStatusId;
	std::byte variantIndex;
	std::byte teamOrOwnerId;
	std::byte extraFieldA;
	std::byte extraFieldB;
	std::byte defaultValue;
	std::byte finalPadding[5];
};

struct BitStreamContext_Struct
{
	std::byte* pCurrentBuffer;
	std::byte* pBufferEnd;
	uint32_t maxBufferSize;
	uint32_t initialState;
	std::byte _pad1[8];
	uint32_t totalBitsRead;
	uint64_t bitBuffer;
	uint32_t bitsInBuffer;
	std::byte _pad2[4];
	std::byte* pNextData;
};

struct ReplayFrame_Struct
{
	char versionOrType;
	std::byte _pad0[3];
	uint32_t frameIndex;
	long long timestamp;
	long long sessionToken;
	long long mapHash;
	long long serverGuid;
	char primaryState;
	std::byte secondaryState;
	std::byte tertiaryState;
	uint32_t statusFlags;
	std::byte regionId;
	std::byte _pad1[8];
	SubData_Identity primaryUser;
	std::byte _pad2[3];
	SubData_Identity secondaryUser;
	std::byte _pad3[203];
	uint32_t gameModeFlags;
	std::byte _pad4[12];
	short lobbyId;
	std::byte _pad5[14];
	std::byte weatherType;
	std::byte timeOfDay;
	std::byte cloudDensity;
	std::byte windStrength;
	uint32_t checksum;
	std::byte _pad6[8];
	short FrameType;
	short PlayerCount;
	uint32_t filmID;
	FrameStatus_Struct frameStatus;
	uint32_t CoordConfig[6];
	uint32_t syncFields[2];
	std::byte _pad7;
	std::byte isCompressed;
	std::byte hasExtraData;
	std::byte _pad8;
	uint32_t headerData[2];
	std::byte decompressWorkBuffer[4096];
	uint32_t dataBlockStatus;
	short entityIdMap[256];
	int activeDataBlocks;
	std::byte _pad9[4];
};

struct BLFChunkHeader
{
	char magic[4];		
	uint32_t size;		
	uint16_t unknown;
	uint16_t chunkType;
};

#pragma pack(pop)