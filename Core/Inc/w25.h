#ifndef __W25_H__
#define __W25_H__

#include "hdr.h"

#ifdef SET_W25FLASH

#include "main.h"

//#define W25QXX_DEBUG

#define W25_HARD_RESET

#define MAX_KNOWN_FLASH   15//14//13

#define W25QXX_DUMMY_BYTE 0xA5
#define FLASH_DUMMY_BYTE  W25QXX_DUMMY_BYTE
#define FLASH_ZERO_BYTE   0x00

#define W25qxx_Delay(delay) HAL_Delay(delay)
#define W25_SEL()   HAL_GPIO_WritePin(W25_CS_GPIO_Port, W25_CS_Pin, GPIO_PIN_RESET);//set to 0
#define W25_UNSEL() HAL_GPIO_WritePin(W25_CS_GPIO_Port, W25_CS_Pin, GPIO_PIN_SET);  //set to 1

#define W25_RST(x) { HAL_GPIO_WritePin(W25_RST_GPIO_Port, W25_RST_Pin, x); }


#define PAGE_BUF_SIZE   256
#define PAGE_HDR_BYTES  4


#define EN_RESET        0x66
#define CHIP_RESET      0x99

#define JEDEC_ID        0x9F
#define READ_UID        0x4B
#define READ_EID        0xAB


#define WRITE_ENABLE       6
#define WRITE_DISABLE      4
#define READ_STAT_REG1     5
#define READ_STAT_REG2  0x35
#define READ_STAT_REG3  0x15
#define WRITE_STAT_REG1    1
#define WRITE_STAT_REG2 0x31
#define WRITE_STAT_REG3 0x11
#define CHIP_ERASE      0xC7
#define SECTOR_ERASE    0x20
#define BLOCK32_ERASE   0x52
#define BLOCK64_ERASE   0xD8
#define FAST_READ       0x0B
#define DATA_READ          3
#define PAGE_PROG          2


#define HTONS(x) \
    ((uint16_t)((x >> 8) | ((x << 8) & 0xff00)))
#define HTONL(x) \
    ((uint32_t)((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | ((x << 24) & 0xff000000)))

typedef enum {
    W25Q10 = 1,
    W25Q20,
    W25Q40,
    W25Q80,
    W25Q16,
    W25Q32,
    W25Q64,
    W25Q128,
    W25Q256,
    W25Q512,
	MX25L8035,
	S25FL128,
	S25FL256,
	S25FL512
} W25QXX_ID_t;

#pragma pack(push,1)
typedef struct {
    W25QXX_ID_t ID;
    uint8_t UniqID[8];
    uint32_t Jedec;
    uint8_t EID;

    uint16_t PageSize;
    uint32_t PageCount;
    uint32_t SectorSize;
    uint32_t SectorCount;
    uint32_t BlockSize;
    uint32_t BlockCount;

    uint32_t CapacityInKiloByte;

    uint8_t StatusRegister1;
    uint8_t StatusRegister2;
    uint8_t StatusRegister3;

    uint8_t Lock;
} w25qxx_t;
#pragma pack(pop)


//------------------------------------------------------------------------------------------

extern w25qxx_t w25qxx;
//const char *all_chipID[];
//const uint32_t all_chipBLK[];
extern uint8_t w25_withDMA;
extern uint8_t *pageTmp;//[PAGE_BUF_SIZE + PAGE_HDR_BYTES + 1];
extern uint32_t size_pageTmp;
extern uint32_t page_buf_size;

extern const char *eol;

//------------------------------------------------------------------------------------------

void W25_SELECT();
void W25_UNSELECT();

void W25qxx_Reset();

bool W25qxx_Init(void);

uint32_t W25qxx_getChipID();
uint32_t W25qxx_getSectorCount();
uint32_t W25qxx_getSectorSize();
uint32_t W25qxx_getPageCount();
uint32_t W25qxx_getPageSize();
uint32_t W25qxx_getBlockCount();
uint32_t W25qxx_getBlockSize();
uint32_t W25qxx_getTotalMem();

void W25qxx_EraseChip(void);
void W25qxx_FillChip(uint8_t byte);
void W25qxx_EraseSector(uint32_t SectorAddr, uint8_t prn);
void W25qxx_EraseBlock(uint32_t BlockAddr);

uint32_t W25qxx_PageToSector(uint32_t PageAddress);
uint32_t W25qxx_PageToBlock(uint32_t PageAddress);
uint32_t W25qxx_SectorToBlock(uint32_t SectorAddress);
uint32_t W25qxx_SectorToPage(uint32_t SectorAddress);
uint32_t W25qxx_BlockToPage(uint32_t BlockAddress);

bool W25qxx_IsEmptyPage(uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_PageSize);
bool W25qxx_IsEmptySector(uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_SectorSize);
bool W25qxx_IsEmptyBlock(uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_BlockSize);

void W25qxx_WriteByte(uint8_t pBuffer, uint32_t Bytes_Address);
void W25qxx_ReadByte(uint8_t *pBuffer, uint32_t Bytes_Address);
void W25qxx_ReadBytes(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);

void W25qxx_WritePage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_PageSize);
void W25qxx_FillPage(uint8_t byte, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_PageSize);
void W25qxx_ReadPage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_PageSize);

void W25qxx_WriteSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_SectorSize);
void W25qxx_FillSector(uint8_t byte, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_SectorSize);
void W25qxx_ReadSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_SectorSize);

void W25qxx_WriteBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_BlockSize);
void W25qxx_ReadBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_BlockSize);

void W25qxx_RdSec(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_SectorSize);

void wpDisable();
void wpEnable();
bool wpGet();


#endif

#endif /* __W25_H__ */
