
#include "hdr.h"

#ifdef SET_W25FLASH

#include "main.h"
#include "w25.h"

//------------------------------------------------------------------------------------------

w25qxx_t w25qxx;

const uint32_t all_chipBLK[] = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 16, 256, 256, 256 };

uint8_t *pageTmp = NULL;//[PAGE_BUF_SIZE + PAGE_HDR_BYTES + 1] = {0};
uint32_t size_pageTmp = 0;

uint8_t w25_withDMA = 0;

uint32_t min_wait_ms = 250;
uint32_t max_wait_ms = 1000;

uint8_t dummy_byte = FLASH_ZERO_BYTE;
uint32_t page_hdr_bytes = PAGE_HDR_BYTES;
uint32_t page_buf_size = PAGE_BUF_SIZE;

//char stz[MAX_UART_BUF] = {0};

bool wp_status = false;
//------------------------------------------------------------------------------------------

void W25_SELECT()   { W25_SEL(); }
void W25_UNSELECT() { W25_UNSEL(); }


//------------------------------------------------------------------------------------------
void wpDisable()
{
	HAL_GPIO_WritePin(SPI4_WP_GPIO_Port, SPI4_WP_Pin, GPIO_PIN_SET);
	wp_status = false;
}
//------------------------------------------------------------------------------------------
void wpEnable()
{
	HAL_GPIO_WritePin(SPI4_WP_GPIO_Port, SPI4_WP_Pin, GPIO_PIN_RESET);
	wp_status = true;
}
//------------------------------------------------------------------------------------------
bool wpGet()
{
	return wp_status;
}
//------------------------------------------------------------------------------------------
const char *all_chipID(uint32_t idx)
{
	switch (idx) {
		case 1:
			return "W25Q10";
		case 2:
			return "W25Q20";
		case 3:
			return "W25Q40";
		case 4:
			return "W25Q80";
		case 5:
			return "W25Q16";
		case 6:
			return "W25Q32";
		case 7:
			return "W25Q64";
		case 8:
			return "W25Q128";
		case 9:
			return "W25Q256";
		case 10:
			return "W25Q512";
		case 11:
			return "MX25L8035";
		case 12:
			return "S25FL128";
		case 13:
			return "S25FL256";
		case 14:
			return "S25FL512";
	}
	return "???";
}
//------------------------------------------------------------------------------------------
uint8_t W25qxx_Spi(uint8_t Data)
{
uint8_t ret;

    HAL_SPI_TransmitReceive(portFLASH, &Data, &ret, 1, min_wait_ms);//HAL_MAX_DELAY);

    return ret;
}
//------------------------------------------------------------------------------------------
void W25qxx_Reset(void)
{
	W25qxx_Delay(1);

#ifdef W25_HARD_RESET

	W25_RST(GPIO_PIN_SET);
	W25qxx_Delay(1);
	W25_RST(GPIO_PIN_RESET);
	W25qxx_Delay(2);
	W25_RST(GPIO_PIN_SET);
	W25qxx_Delay(2);

#else

	W25_SEL();
		W25qxx_Spi(EN_RESET);
		W25qxx_Spi(CHIP_RESET);
	W25_UNSEL();

#endif

	W25qxx_Delay(100);
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_ReadID(void)
{
uint32_t Temp[3] = {0};

    W25_SEL();//set to 0

    W25qxx_Spi(JEDEC_ID);
    Temp[0] = W25qxx_Spi(dummy_byte);
    Temp[1] = W25qxx_Spi(dummy_byte);
    Temp[2] = W25qxx_Spi(dummy_byte);

    W25_UNSEL();//set to 1

    return ((Temp[0] << 16) | (Temp[1] << 8) | Temp[2]);
}
//------------------------------------------------------------------------------------------
void W25qxx_ReadUniqID(void)
{
	uint8_t dat[] = {READ_UID, dummy_byte, dummy_byte, dummy_byte, dummy_byte};
    W25_SEL();

    HAL_SPI_Transmit(portFLASH, dat, sizeof(dat), min_wait_ms);
    HAL_SPI_Receive(portFLASH, w25qxx.UniqID, 8, min_wait_ms);

    W25_UNSEL();
}
//------------------------------------------------------------------------------------------
void W25qxx_ReadEID(void)
{
	uint8_t dat[] = {READ_EID, dummy_byte, dummy_byte};
	uint8_t in[2];
    W25_SEL();

    HAL_SPI_Transmit(portFLASH, dat, sizeof(dat), min_wait_ms);
    HAL_SPI_Receive(portFLASH, in, 2, min_wait_ms);

    w25qxx.EID = in[1];

    W25_UNSEL();
}
//------------------------------------------------------------------------------------------
void W25qxx_WriteEnable(void)
{
    W25_SEL();

    W25qxx_Spi(WRITE_ENABLE);

    W25_UNSEL();

    W25qxx_Delay(1);
}
//------------------------------------------------------------------------------------------
void W25qxx_WriteDisable(void)
{
    W25_SEL();

    W25qxx_Spi(WRITE_DISABLE);

    W25_UNSEL();

    W25qxx_Delay(1);
}
//------------------------------------------------------------------------------------------
uint8_t W25qxx_ReadStatusRegister(uint8_t SelectStatusReg)
{
uint8_t status = 0;

    W25_SEL();

    switch (SelectStatusReg) {
        case 1:
            W25qxx_Spi(READ_STAT_REG1);
            status = W25qxx_Spi(dummy_byte);
            w25qxx.StatusRegister1 = status;
        break;
        case 2:
            W25qxx_Spi(READ_STAT_REG2);
            status = W25qxx_Spi(dummy_byte);
            w25qxx.StatusRegister2 = status;
        break;
        default : {
            W25qxx_Spi(READ_STAT_REG3);
            status = W25qxx_Spi(dummy_byte);
            w25qxx.StatusRegister3 = status;
        }
    }

    W25_UNSEL();

    return status;
}
//------------------------------------------------------------------------------------------
void W25qxx_WriteStatusRegister(uint8_t SelectStatusReg, uint8_t Data)
{
    W25_SEL();

    switch (SelectStatusReg) {
        case 1 :
            W25qxx_Spi(WRITE_STAT_REG1);
            w25qxx.StatusRegister1 = Data;
        break;
        case 2 :
            W25qxx_Spi(WRITE_STAT_REG2);
            w25qxx.StatusRegister2 = Data;
        break;
        default : {
            W25qxx_Spi(WRITE_STAT_REG3);
            w25qxx.StatusRegister3 = Data;
        }
    }

    W25qxx_Spi(Data);

    W25_UNSEL();
}
//------------------------------------------------------------------------------------------
void W25qxx_WaitForWriteEnd(void)
{
    W25qxx_Delay(1);

    W25_SEL();

    W25qxx_Spi(READ_STAT_REG1);
    do
    {
        w25qxx.StatusRegister1 = W25qxx_Spi(dummy_byte);
        W25qxx_Delay(1);
    } while ((w25qxx.StatusRegister1 & 0x01) == 0x01);

    W25_UNSEL();
}
//------------------------------------------------------------------------------------------
bool W25qxx_Init(void)
{

	wpDisable();
	HAL_GPIO_WritePin(SPI4_HOLD_GPIO_Port, SPI4_HOLD_Pin, GPIO_PIN_SET);

	W25qxx_Reset();

    w25qxx.Lock = 1;
    bool ret = false;

    W25_UNSEL();

    uint32_t id = W25qxx_ReadID();
    w25qxx.Jedec = id;

//#ifdef W25QXX_DEBUG
        Report(NULL, true, "chipID:0x%06X%s", id, eol);
//#endif
        uint8_t man = (id >> 16);
        id &= 0xffff;
        if (man == 0xef) {// ef 401X -> w25.....
            if ((id >> 8) == 0x40) {
                id &= 0xff;
                if ((id > 0x10) && (id <= 0x20)) {
                    id -= 0x10;//0x4010..0x4018
                    if (id > 0x0a) id = 0;
                }
            } else {
                id = 0;
            }
        } else if (man == 0xc2) {//0xc2 2014
            dummy_byte = FLASH_ZERO_BYTE;
            if ((id >> 8) == 0x20) {
                id &= 0xff;
                if (id == 0x14)
                    id -= 9;//0x14 - 0x0b
                else
                    id = 0;
            } else {
                id = 0;
            }
        } else if (man == 0x01) {//0x01 0x20 0x18  ...  0x01 0x02 0x20
            dummy_byte = FLASH_ZERO_BYTE;
            if ((id >> 8) == 0x20) {
                id &= 0xff;
                if (id == 0x18) id = S25FL128;//MAX_KNOWN_FLASH - 2;//0x14 - 0x0b
                           else id = 0;
            } else if ((id >> 8) == 0x02) {
            	id &= 0xff;
            	if (id == 0x20) id = S25FL512;//MAX_KNOWN_FLASH - 1;//s25fl512
            		       else id = 0;
            } else id = 0;
        } else id = 0;

        if ((id >= MAX_KNOWN_FLASH) || !id) goto out_label;

        w25qxx.ID         = id;              //W25Q10...W25Q512, MX25..
        w25qxx.BlockCount = all_chipBLK[id]; //0..1024, 256

        if (id >= S25FL128) W25qxx_ReadEID();

        char *stz = (char *)calloc(1, MAX_UART_BUF);
        if (stz) {
            sprintf(stz, "Chip idx=%lu %s", id, all_chipID(id));
            if (id >= S25FL128) sprintf(stz+strlen(stz), "%c", ((w25qxx.EID >= 0x17) && (w25qxx.EID <= 0x19)) ? 'S' : '?');
            if (id) {
            	w25qxx.PageSize = 256;
            	w25qxx.SectorSize = 4096;
            	switch (w25qxx.ID) {
            		case S25FL512:
            			w25qxx.PageSize = 512;
            			w25qxx.CapacityInKiloByte = 64 * 1024;
            			w25qxx.SectorCount = (w25qxx.CapacityInKiloByte * 1024) / w25qxx.SectorSize;
            			w25qxx.PageCount = (w25qxx.CapacityInKiloByte * 1024) / w25qxx.PageSize;
            			w25qxx.BlockSize = 256 * 1024;
            	    break;
            		case S25FL128:
            			w25qxx.CapacityInKiloByte = ((128 * 1024 * 1024) / 8) / 1024;
            			w25qxx.SectorCount = w25qxx.CapacityInKiloByte / (w25qxx.SectorSize / 1024);
            			w25qxx.PageCount = (w25qxx.CapacityInKiloByte * 1024) / w25qxx.PageSize;
            			w25qxx.BlockSize = w25qxx.SectorSize * 16;
            		break;
            		case S25FL256:
            			w25qxx.CapacityInKiloByte = ((256 * 1024 * 1024) / 8) / 1024;
            			w25qxx.SectorCount = w25qxx.CapacityInKiloByte / (w25qxx.SectorSize / 1024);
            			w25qxx.PageCount = (w25qxx.CapacityInKiloByte * 1024) / w25qxx.PageSize;
            			w25qxx.BlockSize = w25qxx.SectorSize * 16;
                	break;
            			default: {
            				w25qxx.SectorCount = w25qxx.BlockCount * 16;
            				w25qxx.PageCount = (w25qxx.SectorCount * w25qxx.SectorSize) / w25qxx.PageSize;
            				w25qxx.CapacityInKiloByte = (w25qxx.SectorCount * w25qxx.SectorSize) / 1024;
            				w25qxx.BlockSize = w25qxx.SectorSize * 16;
            			}
                }
                sprintf(stz+strlen(stz),
"%s\tPage Size:\t%u bytes%s\tPage Count:\t%lu%s\tSector Size:\t%lu bytes%s\
\tSector Count:\t%lu%s\tBlock Size:\t%lu bytes%s\tBlock Count:\t%lu%s\tCapacity:\t%lu KBytes%s",
                    eol,
					w25qxx.PageSize, eol,
					w25qxx.PageCount, eol,
					w25qxx.SectorSize, eol,
					w25qxx.SectorCount, eol,
					w25qxx.BlockSize, eol,
					w25qxx.BlockCount, eol,
					w25qxx.CapacityInKiloByte, eol );
                ret = true;
            }

            if (id >= S25FL128)
            	sprintf(stz+strlen(stz), "EID: 0x%02X -> %s%c%s",
            			                 w25qxx.EID,
										 all_chipID(id),
										 ((w25qxx.EID >= 0x17) && (w25qxx.EID <= 0x19)) ? 'S' : '?',
										 eol);
//#ifdef W25QXX_DEBUG
            Report(NULL, true, "%s", stz);
//#endif
            free(stz);
        }

        if (w25qxx.CapacityInKiloByte >= 32768) page_hdr_bytes++;
        page_buf_size = w25qxx.PageSize;
        size_pageTmp = w25qxx.PageSize + page_hdr_bytes + 2;
        pageTmp = (uint8_t *)calloc(1, size_pageTmp);
        if (!pageTmp) {
        	devError |= devMEM;
        	ret = false;
        }


out_label:

    w25qxx.Lock = 0;

    return ret;
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_getChipID()
{
	return (uint32_t)w25qxx.ID;
}
uint32_t W25qxx_getSectorCount()
{
	return w25qxx.SectorCount;
}
uint32_t W25qxx_getSectorSize()
{
	return w25qxx.SectorSize;
}
uint32_t W25qxx_getPageCount()
{
	return w25qxx.PageCount;
}
uint32_t W25qxx_getPageSize()
{
	return w25qxx.PageSize;
}
uint32_t W25qxx_getBlockCount()
{
	return w25qxx.BlockCount;
}
uint32_t W25qxx_getBlockSize()
{
	return w25qxx.BlockSize;
}
uint32_t W25qxx_getTotalMem()
{
	return w25qxx.CapacityInKiloByte;
}
//------------------------------------------------------------------------------------------
void W25qxx_EraseChip(void)
{
    while (w25qxx.Lock) W25qxx_Delay(1);//wait unlock device

    w25qxx.Lock = 1;//lock device

//#ifdef W25QXX_DEBUG
    uint32_t StartTime = HAL_GetTick();
    Report(__func__, true, "Begin...");
//#endif
    W25qxx_WriteEnable();

    W25_SEL();

    W25qxx_Spi(CHIP_ERASE);

    W25_UNSEL();

    W25qxx_WaitForWriteEnd();
//#ifdef W25QXX_DEBUG
    uint32_t dur = HAL_GetTick() - StartTime;
    Report(NULL, false, " done after %u ms (%u sec)%s", dur, dur / 1000, eol);
//#endif
    W25qxx_Delay(10);

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_FillChip(uint8_t byte)
{
    for (uint32_t s = 0; s < W25qxx_getSectorCount(); s++)
        W25qxx_FillSector(byte, s, 0, W25qxx_getSectorSize());
}
//------------------------------------------------------------------------------------------
void W25qxx_EraseSector(uint32_t SectorAddr, uint8_t prn)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    uint32_t StartTime;
//#ifdef W25QXX_DEBUG
    if (prn) {
    	StartTime = HAL_GetTick();
    	Report(__func__, true, "%u Begin...", SectorAddr);
    }
//#endif

    W25qxx_WaitForWriteEnd();
    SectorAddr = SectorAddr * w25qxx.SectorSize;
    W25qxx_WriteEnable();

    W25_SEL();
    W25qxx_Spi(SECTOR_ERASE);
    //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((SectorAddr & 0xFF000000) >> 24);
    W25qxx_Spi((SectorAddr & 0xFF0000) >> 16);
    W25qxx_Spi((SectorAddr & 0xFF00) >> 8);
    W25qxx_Spi(SectorAddr & 0xFF);
    W25_UNSEL();

    W25qxx_WaitForWriteEnd();

//#ifdef W25QXX_DEBUG
    if (prn) {
    	uint32_t dur = HAL_GetTick() - StartTime;
    	Report(NULL, false, " done after %u ms (%u sec)%s", dur, dur / 1000, eol);
    }
//#endif
    W25qxx_Delay(1);

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_EraseBlock(uint32_t BlockAddr)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

//#ifdef W25QXX_DEBUG
    Report(__func__, true, "%u Begin...", BlockAddr);
    //W25qxx_Delay(100);
    uint32_t StartTime = HAL_GetTick();
//#endif

    W25qxx_WaitForWriteEnd();
    BlockAddr = BlockAddr * w25qxx.SectorSize * 16;
    W25qxx_WriteEnable();

    W25_SEL();
    W25qxx_Spi(BLOCK64_ERASE);
    //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((BlockAddr & 0xFF000000) >> 24);
    W25qxx_Spi((BlockAddr & 0xFF0000) >> 16);
    W25qxx_Spi((BlockAddr & 0xFF00) >> 8);
    W25qxx_Spi(BlockAddr & 0xFF);
    W25_UNSEL();

    W25qxx_WaitForWriteEnd();

//#ifdef W25QXX_DEBUG
    uint32_t dur = HAL_GetTick() - StartTime;
    Report(NULL, false, " done after %u ms (%u sec)%s", dur, dur / 1000, eol);
//#endif
    W25qxx_Delay(1);

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_PageToSector(uint32_t PageAddress)
{
    return ((PageAddress * w25qxx.PageSize) / w25qxx.SectorSize);
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_PageToBlock(uint32_t PageAddress)
{
    return ((PageAddress * w25qxx.PageSize) / w25qxx.BlockSize);
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_SectorToBlock(uint32_t SectorAddress)
{
    return ((SectorAddress * w25qxx.SectorSize) / w25qxx.BlockSize);
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_SectorToPage(uint32_t SectorAddress)
{
    return (SectorAddress * w25qxx.SectorSize) / w25qxx.PageSize;
}
//------------------------------------------------------------------------------------------
uint32_t W25qxx_BlockToPage(uint32_t BlockAddress)
{
    return (BlockAddress * w25qxx.BlockSize) / w25qxx.PageSize;
}
//------------------------------------------------------------------------------------------
bool W25qxx_IsEmptyPage(uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_PageSize)
{

    while(w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    if ( ((NumByteToCheck_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) ||
            (!NumByteToCheck_up_to_PageSize) )
                        NumByteToCheck_up_to_PageSize = w25qxx.PageSize - OffsetInByte;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckPage:0x%X(%u), Offset:%u, Bytes:%u begin...%s",
                 Page_Address, Page_Address, OffsetInByte, NumByteToCheck_up_to_PageSize, eol);
    //W25qxx_Delay(100);
    uint32_t StartTime = HAL_GetTick();
#endif

    uint8_t pBuffer[32];
    uint32_t i, WorkAddress;
    for (i = OffsetInByte; i < w25qxx.PageSize; i += sizeof(pBuffer)) {

        W25_SEL();
        WorkAddress = (i + Page_Address * w25qxx.PageSize);
        W25qxx_Spi(DATA_READ);//FAST_READ);
        //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
        W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
        W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
        W25qxx_Spi(WorkAddress & 0xFF);
        //W25qxx_Spi(0);
        HAL_SPI_Receive(portFLASH, pBuffer, sizeof(pBuffer), min_wait_ms);
        W25_UNSEL();

        for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
            if (pBuffer[x] != 0xFF) goto NOT_EMPTY;
        }
    }
    if ((w25qxx.PageSize + OffsetInByte) % sizeof(pBuffer) != 0) {
        i -= sizeof(pBuffer);
        for ( ; i < w25qxx.PageSize; i++) {

        	W25_SEL();
            WorkAddress = (i + Page_Address * w25qxx.PageSize);
            W25qxx_Spi(DATA_READ);//FAST_READ);
            //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
            W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
            W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
            W25qxx_Spi(WorkAddress & 0xFF);
            //W25qxx_Spi(0);
            HAL_SPI_Receive(portFLASH, pBuffer, 1, min_wait_ms);
            W25_UNSEL();

            if (pBuffer[0] != 0xFF) goto NOT_EMPTY;
        }
    }

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckPage is Empty in %u ms%s", HAL_GetTick() - StartTime, eol);
    //W25qxx_Delay(100);
#endif

    w25qxx.Lock = 0;

    return true;

NOT_EMPTY:

#ifdef W25QXX_DEBUG
	Report(NULL, true, "w25qxx CheckPage is Not Empty in %u ms%s", HAL_GetTick() - StartTime, eol);
	W25qxx_Delay(100);
#endif

    w25qxx.Lock = 0;

    return false;
}
//------------------------------------------------------------------------------------------
bool W25qxx_IsEmptySector(uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_SectorSize)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    if ( (NumByteToCheck_up_to_SectorSize > w25qxx.SectorSize) || (!NumByteToCheck_up_to_SectorSize) )
                NumByteToCheck_up_to_SectorSize = w25qxx.SectorSize;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckSector:0x%X(%u), Offset:%u, Bytes:%u begin...%s",
                 Sector_Address, Sector_Address, OffsetInByte, NumByteToCheck_up_to_SectorSize, eol);
    //W25qxx_Delay(100);
    uint32_t StartTime = HAL_GetTick();
#endif

    uint8_t pBuffer[32];
    uint32_t i, WorkAddress;
    for ( i = OffsetInByte; i < w25qxx.SectorSize; i += sizeof(pBuffer)) {

    	W25_SEL();
        WorkAddress = (i + Sector_Address * w25qxx.SectorSize);
        W25qxx_Spi(DATA_READ);//FAST_READ);
        //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
        W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
        W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
        W25qxx_Spi(WorkAddress & 0xFF);
        //W25qxx_Spi(0);
        HAL_SPI_Receive(portFLASH, pBuffer, sizeof(pBuffer), min_wait_ms);
        W25_UNSEL();

        for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
            if (pBuffer[x] != 0xFF) goto NOT_EMPTY;
        }
    }
    if ((w25qxx.SectorSize + OffsetInByte) % sizeof(pBuffer) != 0) {
        i -= sizeof(pBuffer);
        for( ; i < w25qxx.SectorSize; i++) {

            W25_SEL();
            WorkAddress = (i + Sector_Address * w25qxx.SectorSize);
            W25qxx_Spi(DATA_READ);//FAST_READ);
            //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
            W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
            W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
            W25qxx_Spi(WorkAddress & 0xFF);
            //W25qxx_Spi(0);
            HAL_SPI_Receive(portFLASH, pBuffer, 1, min_wait_ms);
            W25_UNSEL();

            if (pBuffer[0] != 0xFF) goto NOT_EMPTY;
        }
    }

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckSector is Empty in %u ms%s", HAL_GetTick() - StartTime, eol);
    //W25qxx_Delay(100);
#endif

    w25qxx.Lock = 0;

    return true;

NOT_EMPTY:

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckSector is Not Empty in %u ms%s", HAL_GetTick() - StartTime, eol);
    //W25qxx_Delay(100);
#endif

    w25qxx.Lock = 0;

    return false;
}
//------------------------------------------------------------------------------------------
bool W25qxx_IsEmptyBlock(uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToCheck_up_to_BlockSize)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    if ( (NumByteToCheck_up_to_BlockSize > w25qxx.BlockSize) || !NumByteToCheck_up_to_BlockSize )
                          NumByteToCheck_up_to_BlockSize = w25qxx.BlockSize;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckBlock:0x%X(%u), Offset:%u, Bytes:%u begin...%s",
                 Block_Address, Block_Address, OffsetInByte, NumByteToCheck_up_to_BlockSize, eol);
    //W25qxx_Delay(100);
    uint32_t StartTime = HAL_GetTick();
#endif

    uint8_t pBuffer[32];
    uint32_t i, WorkAddress;
    for (i = OffsetInByte; i < w25qxx.BlockSize; i += sizeof(pBuffer)) {

        W25_SEL();
        WorkAddress = (i + Block_Address * w25qxx.BlockSize);
        W25qxx_Spi(DATA_READ);//FAST_READ);
        //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
        W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
        W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
        W25qxx_Spi(WorkAddress & 0xFF);
        //W25qxx_Spi(0);
        HAL_SPI_Receive(portFLASH, pBuffer, sizeof(pBuffer), min_wait_ms);
        W25_UNSEL();

        for (uint8_t x = 0; x < sizeof(pBuffer); x++) {
            if(pBuffer[x] != 0xFF) goto NOT_EMPTY;
        }
    }
    if ((w25qxx.BlockSize + OffsetInByte) % sizeof(pBuffer) != 0) {
        i -= sizeof(pBuffer);
        for ( ; i < w25qxx.BlockSize; i++) {

            W25_SEL();
            WorkAddress = (i + Block_Address * w25qxx.BlockSize);
            W25qxx_Spi(DATA_READ);//FAST_READ);
            //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WorkAddress & 0xFF000000) >> 24);
            W25qxx_Spi((WorkAddress & 0xFF0000) >> 16);
            W25qxx_Spi((WorkAddress & 0xFF00) >> 8);
            W25qxx_Spi(WorkAddress & 0xFF);
            //W25qxx_Spi(0);
            HAL_SPI_Receive(portFLASH, pBuffer, 1, min_wait_ms);
            W25_UNSEL();

            if (pBuffer[0] != 0xFF) goto NOT_EMPTY;
        }
    }

#ifdef W25QXX_DEBUG
    Report(NULL, true, "w25qxx CheckBlock is Empty in %u ms%s", HAL_GetTick() - StartTime, eol);
    //W25qxx_Delay(100);
#endif

    w25qxx.Lock = 0;

    return true;

NOT_EMPTY:

#ifdef W25QXX_DEBUG
	Report(NULL, true, "w25qxx CheckBlock is Not Empty in %u ms%s", HAL_GetTick() - StartTime, eol);
	//W25qxx_Delay(100);
#endif

    w25qxx.Lock = 0;

    return false;
}
//------------------------------------------------------------------------------------------
void W25qxx_WriteByte(uint8_t pBuffer, uint32_t WriteAddr_inBytes)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

#ifdef W25QXX_DEBUG
    uint32_t StartTime = HAL_GetTick();
    Report(NULL, true, "%s 0x%02X at address %d begin...", __func__, pBuffer, WriteAddr_inBytes);
#endif

    W25qxx_WaitForWriteEnd();
    W25qxx_WriteEnable();

    uint8_t *uk = NULL;
    uint16_t len = 6;
    if (w25qxx.CapacityInKiloByte >= 32768) {
    	uint8_t dat[] = {PAGE_PROG,
    			(uint8_t)((WriteAddr_inBytes & 0xFF000000) >> 24),
				(uint8_t)((WriteAddr_inBytes & 0xFF0000) >> 16),
				(uint8_t)((WriteAddr_inBytes & 0xFF00) >> 8),
				(uint8_t)(WriteAddr_inBytes & 0xFF),
				pBuffer};
        uk = &dat[0];
    } else {
    	uint8_t dat[] = {PAGE_PROG,
    			(uint8_t)((WriteAddr_inBytes & 0xFF0000) >> 16),
				(uint8_t)((WriteAddr_inBytes & 0xFF00) >> 8),
				(uint8_t)(WriteAddr_inBytes & 0xFF),
				pBuffer};
    	uk = &dat[0];
        len--;
    }
    W25_SEL();
    //W25qxx_Spi(PAGE_PROG);
    ////if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((WriteAddr_inBytes & 0xFF000000) >> 24);
    //W25qxx_Spi((WriteAddr_inBytes & 0xFF0000) >> 16);
    //W25qxx_Spi((WriteAddr_inBytes & 0xFF00) >> 8);
    //W25qxx_Spi(WriteAddr_inBytes & 0xFF);
    //W25qxx_Spi(pBuffer);
    HAL_SPI_Transmit(portFLASH, uk, len, min_wait_ms);//100);
    W25_UNSEL();

    W25qxx_WaitForWriteEnd();

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s done after %d ms%s", __func__, HAL_GetTick() - StartTime, eol);
#endif

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_WritePage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_PageSize)
{
	if (!pageTmp || !size_pageTmp) return;

    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    if ( ((NumByteToWrite_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) || !NumByteToWrite_up_to_PageSize )
                NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
    if ( (OffsetInByte + NumByteToWrite_up_to_PageSize) > w25qxx.PageSize )
                NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s WritePage:0x%X(%u), Offset:%u ,Writes %u Bytes, begin...%s",
                 __func__, Page_Address, Page_Address, OffsetInByte, NumByteToWrite_up_to_PageSize, eol);
    uint32_t StartTime = HAL_GetTick();
#endif

    W25qxx_WaitForWriteEnd();
    W25qxx_WriteEnable();

    Page_Address = (Page_Address * w25qxx.PageSize) + OffsetInByte;

    /*W25_SELECT();
    W25qxx_Spi(PAGE_PROG);
    //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((Page_Address & 0xFF000000) >> 24);
    W25qxx_Spi((Page_Address & 0xFF0000) >> 16);
    W25qxx_Spi((Page_Address & 0xFF00) >> 8);
    W25qxx_Spi(Page_Address&0xFF);
    HAL_SPI_Transmit(portFLASH, pBuffer, NumByteToWrite_up_to_PageSize, min_wait_ms);
    W25_UNSELECT();*/

    uint16_t lens = NumByteToWrite_up_to_PageSize + page_hdr_bytes;//PAGE_HDR_BYTES;
    int idx = 0;
    pageTmp[idx++] = PAGE_PROG;
    //if (w25qxx.CapacityInKiloByte >= 32768) pageTmp[idx++] = (Page_Address & 0xFF000000) >> 24;
    pageTmp[idx++] = (Page_Address & 0xFF0000) >> 16;
    pageTmp[idx++] = (Page_Address& 0xFF00) >> 8;
    pageTmp[idx++] = Page_Address & 0xFF;
    memcpy(&pageTmp[PAGE_HDR_BYTES], pBuffer, NumByteToWrite_up_to_PageSize);//w25qxx.PageSize);

    spiRdy = 0;

w25_withDMA = 1;
    W25_SEL();
    if (w25_withDMA) {
    	HAL_SPI_Transmit_DMA(portFLASH, pageTmp, lens);
    	while (!spiRdy) {
    		W25qxx_Delay(1);
    	}
w25_withDMA = 0;
    } else {
    	HAL_SPI_Transmit(portFLASH, pageTmp, lens, min_wait_ms);

    	W25_UNSEL();

    	W25qxx_WaitForWriteEnd();

    	spiRdy = 1;

#ifdef W25QXX_DEBUG
    	StartTime = HAL_GetTick() - StartTime;
    	for (uint32_t i = 0; i < NumByteToWrite_up_to_PageSize ; i++) {
    		if ( (i % 16 == 0) && (i > 2) ) Report(NULL, false, "%s", eol);
    		Report(NULL, false, "0x%02X,", pBuffer[i]);
    	}
    	Report(NULL, false, "%s", eol);
    	Report(NULL, true, "%s done after %u ms%s", __func__, StartTime, eol);
#endif
    }

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_FillPage(uint8_t byte, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_PageSize)
{
	if (!pageTmp || !size_pageTmp) return;

    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    if ( ((NumByteToWrite_up_to_PageSize + OffsetInByte) > w25qxx.PageSize) || !NumByteToWrite_up_to_PageSize )
                NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;
    if ( (OffsetInByte + NumByteToWrite_up_to_PageSize) > w25qxx.PageSize )
                NumByteToWrite_up_to_PageSize = w25qxx.PageSize - OffsetInByte;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s WritePage:0x%X(%u), Offset:%u ,Writes %u Bytes, begin...\r\n",
                 __func__, Page_Address, Page_Address, OffsetInByte, NumByteToWrite_up_to_PageSize);
    uint32_t StartTime = HAL_GetTick();
#endif

    //W25qxx_WaitForWriteEnd();
    W25qxx_WriteEnable();

    Page_Address = (Page_Address * w25qxx.PageSize) + OffsetInByte;

    uint16_t lens = NumByteToWrite_up_to_PageSize + page_hdr_bytes;//PAGE_HDR_BYTES;
    int idx = 0;
    pageTmp[idx++] = PAGE_PROG;
//    if (w25qxx.CapacityInKiloByte >= 32768) pageTmp[idx++] = (Page_Address & 0xFF000000) >> 24;
    pageTmp[idx++] = (Page_Address & 0xFF0000) >> 16;
    pageTmp[idx++] = (Page_Address& 0xFF00) >> 8;
    pageTmp[idx++] = Page_Address & 0xFF;
    memset(&pageTmp[page_hdr_bytes], byte, NumByteToWrite_up_to_PageSize);
    //memcpy(&pageTmp[PAGE_HDR_BYTES], pBuffer, NumByteToWrite_up_to_PageSize);//w25qxx.PageSize);

    spiRdy = 0;

//w25_withDMA = 1;
    W25_SEL();
    if (w25_withDMA) {
    	HAL_SPI_Transmit_DMA(portFLASH, pageTmp, lens);
    	while (!spiRdy) { W25qxx_Delay(1); }
    	//W25_UNSEL();
//w25_withDMA = 0;
    } else {
    	HAL_SPI_Transmit(portFLASH, pageTmp, lens, min_wait_ms);

    	W25_UNSEL();

    	W25qxx_WaitForWriteEnd();

    	spiRdy = 1;

#ifdef W25QXX_DEBUG
    	StartTime = HAL_GetTick() - StartTime;
    	for (uint32_t i = 0; i < NumByteToWrite_up_to_PageSize ; i++) {
    		if ( (i % 16 == 0) && (i > 2) ) Report(NULL, false, "\r\n");
    		Report(NULL, false, "0x%02X,", pBuffer[i]);
    	}
    	Report(NULL, false, "\r\n");
    	Report(NULL, true, "%s done after %u ms\r\n", __func__, StartTime);
#endif
    }

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_WriteSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_SectorSize)
{
    if ((NumByteToWrite_up_to_SectorSize > w25qxx.SectorSize) || !NumByteToWrite_up_to_SectorSize)
                NumByteToWrite_up_to_SectorSize = w25qxx.SectorSize;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s WriteSector:0x%X(%u), Offset:%u ,Write %u Bytes, begin...%s",
                 __func__, Sector_Address, Sector_Address, OffsetInByte, NumByteToWrite_up_to_SectorSize, eol);
    //W25qxx_Delay(100);
#endif

    if (OffsetInByte >= w25qxx.SectorSize) {
#ifdef W25QXX_DEBUG
    	Report(NULL, true, "---w25qxx WriteSector Faild!%s", eol);
    	//W25qxx_Delay(100);
#endif
        return;
    }

    int32_t BytesToWrite;
    uint32_t LocalOffset, StartPage;
    if ((OffsetInByte + NumByteToWrite_up_to_SectorSize) > w25qxx.SectorSize)
        BytesToWrite = w25qxx.SectorSize - OffsetInByte;
    else
        BytesToWrite = NumByteToWrite_up_to_SectorSize;
    StartPage = W25qxx_SectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;

    do
    {
        W25qxx_WritePage(pBuffer, StartPage, LocalOffset, BytesToWrite);
        StartPage++;
        BytesToWrite -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize;
        LocalOffset = 0;
    } while(BytesToWrite > 0);

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s Done%s", __func__, eol);
    //W25qxx_Delay(100);
#endif

}
//------------------------------------------------------------------------------------------
void W25qxx_FillSector(uint8_t byte, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_SectorSize)
{
    if ((NumByteToWrite_up_to_SectorSize > w25qxx.SectorSize) || !NumByteToWrite_up_to_SectorSize)
                NumByteToWrite_up_to_SectorSize = w25qxx.SectorSize;

#ifdef W25QXX_DEBUG
    Report(__func__, true, "WriteSector:%u, Offset:%u ,Write %u Bytes, begin...\r\n",
                           Sector_Address, OffsetInByte, NumByteToWrite_up_to_SectorSize);
    //W25qxx_Delay(100);
#endif

    if (OffsetInByte >= w25qxx.SectorSize) {
#ifdef W25QXX_DEBUG
    	Report(NULL, true, "---w25qxx WriteSector Faild!\r\n");
    	//W25qxx_Delay(100);
#endif
        return;
    }

    int32_t BytesToWrite;
    uint32_t LocalOffset, StartPage;
    if ((OffsetInByte + NumByteToWrite_up_to_SectorSize) > w25qxx.SectorSize)
        BytesToWrite = w25qxx.SectorSize - OffsetInByte;
    else
        BytesToWrite = NumByteToWrite_up_to_SectorSize;
    StartPage = W25qxx_SectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;

    do
    {
        W25qxx_FillPage(byte, StartPage++, LocalOffset, w25qxx.PageSize);//BytesToWrite);
        BytesToWrite -= w25qxx.PageSize - LocalOffset;
        LocalOffset = 0;
    } while(BytesToWrite > 0);

#ifdef W25QXX_DEBUG
    Report(__func__, true, "Done\r\n");
    W25qxx_Delay(100);
#endif

}
//------------------------------------------------------------------------------------------
void W25qxx_WriteBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToWrite_up_to_BlockSize)
{
    if ((NumByteToWrite_up_to_BlockSize > w25qxx.BlockSize) || !NumByteToWrite_up_to_BlockSize)
            NumByteToWrite_up_to_BlockSize = w25qxx.BlockSize;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s WriteBlock:0x%X(%u), Offset:%u ,Write %u Bytes, begin...%s",
                 __func__, Block_Address, Block_Address, OffsetInByte, NumByteToWrite_up_to_BlockSize, eol);
    //W25qxx_Delay(100);
#endif

    if (OffsetInByte >= w25qxx.BlockSize) {
#ifdef W25QXX_DEBUG
    	Report(NULL, true, "%s Faild!%s", __func__, eol);
    	//W25qxx_Delay(100);
#endif
        return;
    }

    int32_t BytesToWrite;
    uint32_t LocalOffset, StartPage;
    if ((OffsetInByte + NumByteToWrite_up_to_BlockSize) > w25qxx.BlockSize)
        BytesToWrite = w25qxx.BlockSize - OffsetInByte;
    else
        BytesToWrite = NumByteToWrite_up_to_BlockSize;
    StartPage = W25qxx_BlockToPage(Block_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do
    {
        W25qxx_WritePage(pBuffer, StartPage, LocalOffset, BytesToWrite);
        StartPage++;
        BytesToWrite -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize;
        LocalOffset = 0;
    } while(BytesToWrite > 0);

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s done%s", __func__, eol);
    //W25qxx_Delay(100);
#endif

}
//------------------------------------------------------------------------------------------
void W25qxx_ReadByte(uint8_t *pBuffer, uint32_t Bytes_Address)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

#ifdef W25QXX_DEBUG
    uint32_t StartTime = HAL_GetTick();
    Report(NULL, true, "%s at address %u begin...%s", __func__, Bytes_Address, eol);
#endif

    W25_SEL();
    W25qxx_Spi(DATA_READ);//FAST_READ);
    //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((Bytes_Address & 0xFF000000) >> 24);
    W25qxx_Spi((Bytes_Address & 0xFF0000) >> 16);
    W25qxx_Spi((Bytes_Address& 0xFF00) >> 8);
    W25qxx_Spi(Bytes_Address & 0xFF);
    //W25qxx_Spi(0);
    *pBuffer = W25qxx_Spi(W25QXX_DUMMY_BYTE);
    W25_UNSEL();

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s 0x%02X done after %u ms%s", __func__, *pBuffer, HAL_GetTick() - StartTime, eol);
#endif

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_ReadBytes(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead)
{
    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

#ifdef W25QXX_DEBUG
    uint32_t StartTime = HAL_GetTick();
    Report(NULL, true, "%s at Address:0x%X(%u), %u Bytes  begin...%s",
    			__func__, ReadAddr, ReadAddr, NumByteToRead, eol);
#endif

    W25_SEL();
    W25qxx_Spi(DATA_READ);//FAST_READ);
//    if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((ReadAddr & 0xFF000000) >> 24);
    W25qxx_Spi((ReadAddr & 0xFF0000) >> 16);
    W25qxx_Spi((ReadAddr& 0xFF00) >> 8);
    W25qxx_Spi(ReadAddr & 0xFF);
    //W25qxx_Spi(0);
    HAL_SPI_Receive(portFLASH, pBuffer, NumByteToRead, max_wait_ms);
    W25_UNSEL();

#ifdef W25QXX_DEBUG
    StartTime = HAL_GetTick() - StartTime;
    for (uint32_t i = 0; i < NumByteToRead ; i++) {
    	if ((i % 8 == 0) && (i > 2)) {
    		Report(NULL, false, "%s", eol);
    		//W25qxx_Delay(10);
    	}
    	Report(NULL, false, "0x%02X,", pBuffer[i]);
    }
    Report(NULL, false, "%s", eol);
    Report(NULL, true, "%s done after %u ms%s", __func__, StartTime, eol);
    //W25qxx_Delay(100);
#else
    //W25qxx_Delay(1);
#endif

    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_ReadPage(uint8_t *pBuffer, uint32_t Page_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_PageSize)
{
	if (!pageTmp || !size_pageTmp) return;

    while (w25qxx.Lock) W25qxx_Delay(1);

    w25qxx.Lock = 1;

    if ((NumByteToRead_up_to_PageSize > w25qxx.PageSize) || !NumByteToRead_up_to_PageSize)
        NumByteToRead_up_to_PageSize = w25qxx.PageSize;
    if ((OffsetInByte + NumByteToRead_up_to_PageSize) > w25qxx.PageSize)
        NumByteToRead_up_to_PageSize = w25qxx.PageSize - OffsetInByte;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s:0x%X(%u), Offset:%u ,Read %u Bytes, begin...%s",
                 __func__, Page_Address, Page_Address, OffsetInByte, NumByteToRead_up_to_PageSize, eol);
    //W25qxx_Delay(100);
    uint32_t StartTime = HAL_GetTick();
#endif

    Page_Address = Page_Address * w25qxx.PageSize + OffsetInByte;
    /*
    W25_SEL();
    W25qxx_Spi(DATA_READ);//FAST_READ);
    //if (w25qxx.CapacityInKiloByte >= 32768) W25qxx_Spi((Page_Address & 0xFF000000) >> 24);
    W25qxx_Spi((Page_Address & 0xFF0000) >> 16);
    W25qxx_Spi((Page_Address& 0xFF00) >> 8);
    W25qxx_Spi(Page_Address & 0xFF);
    //W25qxx_Spi(0);
    HAL_SPI_Receive(portFLASH, pBuffer, NumByteToRead_up_to_PageSize, min_wait_ms);
    W25_UNSEL();
    */
    memset(pageTmp, 0, size_pageTmp);
    uint16_t lens = NumByteToRead_up_to_PageSize + page_hdr_bytes;//PAGE_HDR_BYTES;// + 1;
    int idx = 0;
    pageTmp[idx++] = DATA_READ;//FAST_READ;
//    if (w25qxx.CapacityInKiloByte >= 32768) pageTmp[idx++] = (Page_Address & 0xFF000000) >> 24;
    pageTmp[idx++] = (Page_Address & 0xFF0000) >> 16;
    pageTmp[idx++] = (Page_Address& 0xFF00) >> 8;
    pageTmp[idx++] = Page_Address & 0xFF;
    //pageTmp[idx++] = 0;
    spiRdy = 0;

//w25_withDMA = 1;
    W25_SEL();
    if (w25_withDMA) {
    	HAL_SPI_TransmitReceive_DMA(portFLASH, pageTmp, pageTmp, lens);
    	while (!spiRdy) {
    		W25qxx_Delay(1);
    	}
//w25_withDMA = 0;
    } else {
    	if (HAL_SPI_TransmitReceive(portFLASH, pageTmp, pageTmp, lens, min_wait_ms) != HAL_OK) devError |= devSPI;
    	W25_UNSEL();

    	spiRdy = 1;
    }
	memcpy(pBuffer, &pageTmp[page_hdr_bytes], NumByteToRead_up_to_PageSize);//w25qxx.PageSize);
#ifdef W25QXX_DEBUG
    	StartTime = HAL_GetTick() - StartTime;
    	for (uint32_t i = 0; i < NumByteToRead_up_to_PageSize ; i++) {
    		if ((i % 16 == 0) && (i > 2)) Report(NULL, false, "%s", eol);
    		Report(NULL, false, "0x%02X,", pBuffer[i]);
    	}
    	Report(NULL, false, "%s", eol);
    	Report(NULL, true, "%s done after %u ms%s", __func__, StartTime, eol);
#endif


    w25qxx.Lock = 0;
}
//------------------------------------------------------------------------------------------
void W25qxx_ReadSector(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_SectorSize)
{
    if ((NumByteToRead_up_to_SectorSize > w25qxx.SectorSize) || !NumByteToRead_up_to_SectorSize)
                NumByteToRead_up_to_SectorSize = w25qxx.SectorSize;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s:0x%X(%u), Offset:%u ,Read %u Bytes, begin...%s",
                 __func__, Sector_Address, Sector_Address, OffsetInByte, NumByteToRead_up_to_SectorSize, eol);
    //W25qxx_Delay(100);
#endif

    if (OffsetInByte >= w25qxx.SectorSize) {
#ifdef W25QXX_DEBUG
    	Report(NULL, true, "---w25qxx ReadSector Faild!%s", eol);
    	//W25qxx_Delay(100);
#endif
        return;
    }

    int32_t BytesToRead;
    uint32_t LocalOffset, StartPage;
    if ((OffsetInByte + NumByteToRead_up_to_SectorSize) > w25qxx.SectorSize)
        BytesToRead = w25qxx.SectorSize - OffsetInByte;
    else
        BytesToRead = NumByteToRead_up_to_SectorSize;
    StartPage = W25qxx_SectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_ReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
        StartPage++;
        BytesToRead -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize;
        LocalOffset = 0;
    } while(BytesToRead > 0);

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s done%s", __func__, eol);
    //W25qxx_Delay(100);
#endif

}
//------------------------------------------------------------------------------------------
void W25qxx_ReadBlock(uint8_t *pBuffer, uint32_t Block_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_BlockSize)
{
    if ((NumByteToRead_up_to_BlockSize > w25qxx.BlockSize) || !NumByteToRead_up_to_BlockSize)
        NumByteToRead_up_to_BlockSize = w25qxx.BlockSize;

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s:0x%X(%u), Offset:%u ,Read %u Bytes, begin...%s",
                 __func__, Block_Address, Block_Address, OffsetInByte, NumByteToRead_up_to_BlockSize, eol);
    //W25qxx_Delay(100);
#endif

    if (OffsetInByte >= w25qxx.BlockSize) {
#ifdef W25QXX_DEBUG
    	Report(NULL, true, "%s Faild!%s", __func__, eol);
    	//W25qxx_Delay(100);
#endif
        return;
    }

    int32_t BytesToRead;
    uint32_t LocalOffset, StartPage;
    if ((OffsetInByte + NumByteToRead_up_to_BlockSize) > w25qxx.BlockSize)
        BytesToRead = w25qxx.BlockSize - OffsetInByte;
    else
        BytesToRead = NumByteToRead_up_to_BlockSize;
    StartPage = W25qxx_BlockToPage(Block_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_ReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
        StartPage++;
        BytesToRead -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize;
        LocalOffset = 0;
    } while(BytesToRead > 0);

#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s done%s", __func__, eol);
    //W25qxx_Delay(100);
#endif

}
//------------------------------------------------------------------------------------------
//
void W25qxx_RdSec(uint8_t *pBuffer, uint32_t Sector_Address, uint32_t OffsetInByte, uint32_t NumByteToRead_up_to_SectorSize)
{
    if ((NumByteToRead_up_to_SectorSize > w25qxx.SectorSize) || !NumByteToRead_up_to_SectorSize)
                NumByteToRead_up_to_SectorSize = w25qxx.SectorSize;

/*#ifdef W25QXX_DEBUG
    Report(NULL, true, "%s:0x%X(%u), Offset:%u ,Read %u Bytes, begin...",
                 __func__, Sector_Address, Sector_Address, OffsetInByte, NumByteToRead_up_to_SectorSize);
    //W25qxx_Delay(100);
#endif*/

    if (OffsetInByte >= w25qxx.SectorSize) {
/*#ifdef W25QXX_DEBUG
    	Report(NULL, true, "---w25qxx ReadSector Faild!\r\n");
    	//W25qxx_Delay(100);
#endif*/
        return;
    }

    int32_t BytesToRead;
    uint32_t LocalOffset, StartPage;
    if ((OffsetInByte + NumByteToRead_up_to_SectorSize) > w25qxx.SectorSize)
        BytesToRead = w25qxx.SectorSize - OffsetInByte;
    else
        BytesToRead = NumByteToRead_up_to_SectorSize;
    StartPage = W25qxx_SectorToPage(Sector_Address) + (OffsetInByte / w25qxx.PageSize);
    LocalOffset = OffsetInByte % w25qxx.PageSize;
    do {
        W25qxx_ReadPage(pBuffer, StartPage, LocalOffset, BytesToRead);
        StartPage++;
        BytesToRead -= w25qxx.PageSize - LocalOffset;
        pBuffer += w25qxx.PageSize;
        LocalOffset = 0;
    } while(BytesToRead > 0);

/*#ifdef W25QXX_DEBUG
    Report(NULL, false, " done\r\n");
    W25qxx_Delay(1);//100);
#endif*/

}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------

#endif
