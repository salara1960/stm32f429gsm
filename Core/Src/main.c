/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  arm-none-eabi-objcopy -O ihex "${BuildArtifactFileBaseName}.elf" "${BuildArtifactFileBaseName}.hex" && arm-none-eabi-objcopy -O binary "${BuildArtifactFileBaseName}.elf" "${BuildArtifactFileBaseName}.bin" && ls -la | grep "${BuildArtifactFileBaseName}.*"
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "usbd_cdc_if.h"
#include "mod.h"
//#include "usbd_storage_if.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio;

SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_tx;
DMA_HandleTypeDef hdma_spi4_rx;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_tx;

/* USER CODE BEGIN PV */
uint16_t devError = HAL_OK;

TIM_HandleTypeDef *timePort = &htim2;
UART_HandleTypeDef *portLOG = &huart3;//порт логов (uart)
#ifdef SET_W25FLASH
	SPI_HandleTypeDef *portFLASH = &hspi4;
	uint32_t spi_cnt = 0;
	uint8_t spiRdy = 1;
#endif
UART_HandleTypeDef *portMODEM = &huart2;//порт модема (uart)

char atCmd[256] = {0};
char atAck[256] = {0};
volatile uint8_t uartRdyMod = 1;
char RxBufMod[256] = {0};
volatile uint8_t rx_uk_mod;
volatile uint8_t uRxByteMod = 0;

uint8_t pageBuf[256];
char cmdBuf[MAX_UART_BUF + 24];
volatile uint8_t uartRdy = 1;
char RxBuf[MAX_UART_BUF] = {0};
volatile uint8_t rx_uk;
volatile uint8_t uRxByte = 0;

char oBuf[1024] = {0};
char iBuf[1024] = {0};
int np = 0;
uint32_t len_data = 0, len = 0;

volatile uint32_t vatra = 0;
volatile uint32_t seconda = 0;
uint32_t tmr = 0;
bool setDate = false;
static uint32_t epoch = 1722430459;//1720606603;//1714116555;//1705500950;//1705391321;
uint8_t tZone = 3;
volatile bool QuitLoop = false;

const char *eol = "\n";
uint32_t strob = 0;
int8_t cmd = sNone;
int8_t icmd = sNone;
const uint32_t period = 30;//sec

#ifdef SET_W25FLASH
	bool chipPresent = false;
	uint32_t cid = 0;
	//
	int adr_sector = 0, offset_sector = 0, list_sector = 0, len_write = 0;
	int cmd_sector = sNone, last_cmd_sector = sNone;
	uint8_t byte_fill = 0xff;
	bool flag_sector = false;
	uint32_t esector = 0, rpage = 0;
	uint8_t *pbuf = NULL;
	//
#endif

#ifdef SET_FAT_SD
	extern char SDPath[4];       // SD logical drive path
	extern FATFS SDFatFS;        // File system object for SD logical drive
	const char *cfg = "url.txt";
	bool mnt = false;
	const char *dirName = "/";
	unsigned char fs_work[_MAX_SS] = {0};
	char strf[512] = {0};
	char str_name[_MAX_LFN] = {0};
#endif

uint32_t lenVCP = 0;
char rxVCP[128] = {0};

s_recq_t gsmAck;
bool gsmAckFlag = false;
uint8_t adone = 0;

char egts_addr[MAX_ADDR_LEN] = {0};
char ctrl_addr[MAX_ADDR_LEN] = {0};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI4_Init(void);
static void MX_RTC_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SDIO_SD_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//----------------------------------------------------------------------------------------
bool initSRECQ(s_recq_t *q)//s_recq_t recq;
{
	q->put = q->get = 0;
	for (uint8_t i = 0; i < MAX_SQREC; i++) {
		q->rec[i].id = i;
		q->rec[i].adr = NULL;
	}
	return true;
}
//----------------------------------------------------------------------------------------
void clearSRECQ(s_recq_t *q)
{
	q->put = q->get = 0;
	for (uint8_t i = 0; i < MAX_SQREC; i++) {
		q->rec[i].id = i;
		free(q->rec[i].adr);
		q->rec[i].adr = NULL;
	}
}
//----------------------------------------------------------------------------------------
int8_t putSRECQ(char *adr, s_recq_t *q)
{
int8_t ret = -1;

	if (q->rec[q->put].adr == NULL) {
		q->rec[q->put].adr = adr;
		ret = q->rec[q->put].id;
		q->put++;   if (q->put >= MAX_SQREC) q->put = 0;
	}

	return ret;
}
//----------------------------------------------------------------------------------------
int8_t getSRECQ(char *dat, s_recq_t *q)
{
int8_t ret = -1;
int len = 0;

	if (q->rec[q->get].adr != NULL) {
		len = strlen(q->rec[q->get].adr);
		ret = q->rec[q->get].id;
		memcpy(dat, q->rec[q->get].adr, len);
		free(q->rec[q->get].adr);
		q->rec[q->get].adr = NULL;
	}

	if (ret >= 0) {
		*(dat + len) = '\0';
		q->get++;   if (q->get >= MAX_SQREC) q->get = 0;
	}

	return ret;
}

//------------------------------------------------------------------------------------------

//*****************************************************************************************
//
#ifdef SET_FAT_SD
	//------------------------------------------------------------------------------------
	static char *fsErrName(int fr)
	{
		switch (fr) {
		    case -1://FR_ERR:
		    	return "Error";
			case FR_OK:				// (0) Succeeded
				return "Succeeded";
			case FR_DISK_ERR://			(1) A hard error occurred in the low level disk I/O layer
				return "Error disk I/O";
			case FR_INT_ERR://			(2) Assertion failed
				return "Assertion failed";
			case FR_NOT_READY://		(3) The physical drive cannot work
				return "Drive not ready";
			case FR_NO_FILE://			(4) Could not find the file
				return "No file";
			case FR_NO_PATH://			(5) Could not find the path
				return "No path";
			case FR_INVALID_NAME://		(6) The path name format is invalid
				return "Path error";
			case FR_DENIED://			(7) Access denied due to prohibited access or directory full
			case FR_EXIST://			(8) Access denied due to prohibited access
				return "Access denied";
			case FR_INVALID_OBJECT://	(9) The file/directory object is invalid
				return "Invalid file/dir";
			case FR_WRITE_PROTECTED://	(10) The physical drive is write protected
				return "Write protected";
			case FR_INVALID_DRIVE://	(11) The logical drive number is invalid
				return "Invalid drive number";
			case FR_NOT_ENABLED://		(12) The volume has no work area
				return "Volume no area";
			case FR_NO_FILESYSTEM://	(13) There is no valid FAT volume
				return "Volume has't filesystem";
			case FR_MKFS_ABORTED://		(14) The f_mkfs() aborted due to any problem
				return "f_mkfs() aborted";
			case FR_TIMEOUT://			(15) Could not get a grant to access the volume within defined period
				return "Timeout access";
			case FR_LOCKED://			(16) The operation is rejected according to the file sharing policy
				return "File locked";
			case FR_NOT_ENOUGH_CORE://	(17) LFN working buffer could not be allocated
				return "Allocated buf error";
			case FR_TOO_MANY_OPEN_FILES://	(18) Number of open files > _FS_LOCK
				return "Open file limit";
			case FR_INVALID_PARAMETER://	(19) Given parameter is invalid
				return "Invalid parameter";
		}
		return "Unknown error";
	}
	//------------------------------------------------------------------------------------------
	static char *attrName(uint8_t attr)
	{
		switch (attr) {
			case AM_RDO://	0x01	/* Read only */
				return "Read only";
			case AM_HID://	0x02	/* Hidden */
				return "Hidden";
			case AM_SYS://	0x04	/* System */
				return "System";
			case AM_DIR://	0x10	/* Directory */
				return "Directory";
			case AM_ARC://	0x20	/* Archive */
				return "Archive";
			default : return "Unknown";
		}
	}
	//------------------------------------------------------------------------------------------
	static char *fsName(uint8_t type)
	{
		switch (type) {
			case FM_FAT://	 0x01
				return "FM_FAT";
			case FM_FAT32:// 0x02
				return "FM_FAT32";
			case FM_EXFAT:// 0x04
				return "FM_EXFAT";
			case FM_ANY://	 0x7
				return "FM_ANY";
			case FM_SFD://	 0x8
				return "FM_SFD";
		}
		return "Unknown type FS";
	}
	//------------------------------------------------------------------------------------------
	bool drvMount(const char *path)
	{
	int tp = FM_FAT32;

		FRESULT res = f_mount(&SDFatFS, (const TCHAR *)path, 1);
		if (res == FR_NO_FILESYSTEM) {
			Report(__func__, true, "Mount drive '%s' error #%u (%s)%s", path, res, fsErrName(res), eol);
			res = f_mkfs((const TCHAR *)path, tp, _MAX_SS, fs_work, sizeof(fs_work));
			if (!res) {
				Report(__func__, true, "Make %s fs on drive '%s' OK%s", fsName(tp), path, eol);
				res = f_mount(&SDFatFS, (const TCHAR *)path, 1);
	    	} else {
	    		Report(__func__, true, "Make %s fs error #%u (%s)%s", fsName(tp), res, fsErrName(res), eol);
	    	}
		}
		if (res == FR_OK) {
			Report(__func__, true, "Mount drive '%s' OK%s", path, eol);
		} else {
			Report(__func__, true, "Mount drive '%s' error #%u (%s)%s", path, res, fsErrName(res), eol);
		}

		return (res == FR_OK) ? true : false;
	}
	//--------------------------------------------------------------------------------------------------------
	FRESULT dirList(const char *name_dir)
	{
	DIR dir;

		FRESULT res = f_opendir(&dir, (const TCHAR *)name_dir);
		if (res == FR_OK) {
			FILINFO fno;
			int cnt = -1;
			Report(NULL, true, "Read folder '%s':%s", name_dir, eol);
			for (;;) {
				res = f_readdir(&dir, &fno);
				cnt++;
				if (res || fno.fname[0] == 0) {
					if (!cnt) Report(NULL, false, "\tFolder '%s' is empty%s", name_dir, eol);
					break;
				//} else if (fno.fattrib & AM_DIR) {// It is a directory
				//	Report(NULL, false, "\tIt is folder -> '%s'%s", fno.fname, eol);
				} else {// It is a file.
					Report(NULL, false, "\tname:%s, size:%u bytes, attr:%s%s",
										fno.fname,
										fno.fsize,
										attrName(fno.fattrib),
										eol);
				}
			}
			res = FR_OK;
			f_closedir(&dir);
		}

		return res;
	}
	//--------------------------------------------------------------------------------------------------------
	void wrFile(const char *name, char *text, bool update)
	{
	char tmp[128];
	FIL fp;
	FRESULT res = FR_NO_FILE;
	int8_t err = 0;

		sprintf(tmp, "%s", cfg);
		if (!update) {
			res = f_open(&fp, (const TCHAR *)tmp, FA_READ);
			if (res == FR_OK) {
				res = f_close(&fp);
				Report(NULL, true, "File '%s' allready present and update has't been ordered%s", tmp, eol);
				return;
			}
		}

		int wrt = 0, dl = strlen(text);
		res = f_open(&fp, (const TCHAR *)tmp, FA_CREATE_ALWAYS | FA_WRITE);
		if (res == FR_OK) {
			Report(NULL, true, "Create new file '%s' OK%s", tmp, eol);

			res = f_write(&fp, text, (UINT)dl, (UINT *)&wrt);
			if (res == FR_OK) {
				if (wrt != dl) err = 3;
			} else err = 2;

			res = f_close(&fp);
		} else err = 1;

		if (err) {
			devError |= devFS;
			switch (err) {
				case 1:
					Report(NULL, true, "Create new file '%s' error #%u (%s)%s", tmp, res, fsErrName(res), eol);
				break;
				case 2:
					Report(NULL, true, "Error while write file '%s'%s", tmp, eol);
				break;
				case 3:
					Report(NULL, true, "Error while write file '%s' -> write %d bytes but writen %d bytes%s", tmp, dl, wrt, eol);
				break;
			}
		}
	}
	//--------------------------------------------------------------------------------------------------------
	void rdFile(const char *name)
	{
	FIL fp;

		FRESULT res = f_open(&fp, (const TCHAR *)name, FA_READ);
		if (res == FR_OK) {
			char *tmp = (char *)calloc(1, 512);
			if (!tmp) {
				devError |= devMEM;
				f_close(&fp);
				return;
			}
			////Report(__func__, true, "File '%s' open for reading OK%s", name, eol);
			char *uks = NULL, *uke = NULL;
			while (f_gets((TCHAR *)tmp, sizeof(tmp) - 1, &fp) != NULL) {
				Report(NULL, false, "%s", tmp);
				if ((uke = strchr(uks, '\n'))) {
					*uke = '\0';
					if ((uke = strchr(uks, '\r'))) *uke = '\0';
				}
				if ((uks = strstr(tmp, "egts="))) {
					uks += 5;
					strncpy(egts_addr, uks, sizeof(egts_addr) - 1);
				} else if ((uks = strstr(tmp, "ctrl="))) {
					uks += 5;
					strncpy(ctrl_addr, uks, sizeof(ctrl_addr) - 1);
				}
			}

			free(tmp);

			/*FILINFO fno;
			res = f_stat((const TCHAR *)name, &fno);
			if (res == FR_OK) {
				UINT dl = fno.fsize;
				UINT wrt = 0;
				if (dl >= MAX_UART_BUF) dl = MAX_UART_BUF - 1;
				char *tmp = (char *)calloc(1, dl);
				if (tmp) {
					res = f_read (&fp, tmp, dl, &wrt);
					Report(NULL, false, "%s", tmp);
					free(tmp);
				} else devError |= devMEM;
			} else devError |= devFS;*/

			f_close(&fp);
		} else {
			Report(__func__, true, "Error '%s' while open for reading file '%s'%s", fsErrName(res), name, eol);
		}

	}
	//--------------------------------------------------------------------------------------------------------
#endif


#ifdef SET_W25FLASH
	//
	void printBuffer(const uint8_t *buf, uint32_t len, uint32_t lsize)
	{
	    if ((!len) || (len > PAGE_BUF_SIZE)) return;

	    char *stx = (char *)calloc(1, len << 2);
	    if (stx) {
	        int i = -1;
	        while (++i < len) {
	            if ( (!(i % lsize)) && (i > 0) ) strcat(stx, eol);
	            sprintf(stx+strlen(stx), " %02X", *(uint8_t *)(buf + i));
	        }
	        if (stx[strlen(stx) - 1] != '\n') strcat(stx, eol);
	        Report(NULL, false, "%s", stx);
	        free(stx);
	    } else {
	        devError |= devMEM;
	    }
	}
	//
#endif

//*****************************************************************************************
//------------------------------------------------------------------------------------------
uint32_t get_tmr(uint32_t sec)
{
	return (seconda + sec);
}
//------------------------------------------------------------------------------------------
bool check_tmr(uint32_t sec)
{
	return (seconda >= sec ? true : false);
}
//----------------------------------------------------------------------------------------
uint32_t get_ms250(uint32_t ms250)
{
	return (vatra + ms250);
}
//----------------------------------------------------------------------------------------
bool check_ms250(uint32_t ms250)
{
	return (vatra >= ms250 ? true : false);
}
//----------------------------------------------------------------------------------------
int date_to_str(char *stx)
{
int ret = 0;

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
		devError |= devRTC;
	} else {
		if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
			devError |= devRTC;
		} else {
			ret = sprintf(stx, "%02u.%02u %02u:%02u:%02u | ",
							sDate.Date, sDate.Month,
							sTime.Hours, sTime.Minutes, sTime.Seconds);
		}
	}

    return ret;
}
//-----------------------------------------------------------------------------------------
void set_Date(uint32_t usec)
{
struct tm ts;
time_t ep = (time_t)usec;
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;

	gmtime_r(&ep, &ts);

	sDate.WeekDay = ts.tm_wday;
	sDate.Month   = ts.tm_mon + 1;
	sDate.Date    = ts.tm_mday;
	sDate.Year    = ts.tm_year;
	sTime.Hours   = ts.tm_hour + tZone;
	sTime.Minutes = ts.tm_min;
	sTime.Seconds = ts.tm_sec;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
		devError |= devRTC;
	} else {
		if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
			devError |= devRTC;
		} else {
			setDate = true;
		}
	}
}
//------------------------------------------------------------------------------------------
//   Функция вывода символьной строки в локальный канал управления (portLOG)
//
void Report(const char *tag, bool addTime, const char *fmt, ...)
{
va_list args;
size_t len = MAX_UART_BUF;
int dl = 0;

	char *buff = (char *)calloc(1, len);
	if (buff) {
		if (addTime) dl = date_to_str(buff);

		if (tag) dl += sprintf(buff+strlen(buff), "[%s] ", tag);

		va_start(args, fmt);
		vsnprintf(buff + dl, len - dl, fmt, args);
		int dln = strlen(buff);
		uartRdy = 0;
		if (HAL_UART_Transmit_DMA(portLOG, (uint8_t *)buff, dln) != HAL_OK) devError |= devUART;
		while (!uartRdy) { /*HAL_Delay(1);*/ }
		/*while (HAL_UART_GetState(portLOG) != HAL_UART_STATE_READY) {
			if (HAL_UART_GetState(portLOG) == HAL_UART_STATE_BUSY_RX) break;
			HAL_Delay(1);
		}*/
		va_end(args);

		strcat(buff, "\r");
		if (CDC_Transmit_FS((uint8_t *)buff, (uint16_t)++dln) != USBD_OK) {
			devError |= devVCP;
		} else {
			devError &= ~devVCP;
		}

		free(buff);
	}
}
//---------------------------------------------------------------------------------------------
void putCmd(const char *at, int len)
{
int cnt = 300;

	if (!at || !len) return;

	while (!uartRdyMod) {
		HAL_Delay(1);
		cnt--;
		if (!cnt) {
			devError |= devUART;
			if (!uartRdyMod) return;
		}
	}

	uartRdyMod = 0;
	if (HAL_UART_Transmit_DMA(portMODEM, (uint8_t *)at, (uint16_t)len) != HAL_OK) devError |= devUART;
	//while (!uartRdyMod) { HAL_Delay(1); }
}
//----------------------------------------------------------------------------------------
uint8_t hexToBin(char *sc)
{
char st = 0, ml = 0;

    if ((sc[0] >= '0') && (sc[0] <= '9')) st = (sc[0] - 0x30);
    else
    if ((sc[0] >= 'A') && (sc[0] <= 'F')) st = (sc[0] - 0x37);
    else
    if ((sc[0] >= 'a') && (sc[0] <= 'f')) st = (sc[0] - 0x57);

    if ((sc[1] >= '0') && (sc[1] <= '9')) ml = (sc[1] - 0x30);
    else
    if ((sc[1] >= 'A') && (sc[1] <= 'F')) ml = (sc[1] - 0x37);
    else
    if ((sc[1] >= 'a') && (sc[1] <= 'f')) ml = (sc[1] - 0x57);

    return ((st << 4) | (ml & 0x0f));

}
//---------------------------------------------------------------------------------------------

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI4_Init();
  MX_RTC_Init();
  MX_USART3_UART_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  MX_SDIO_SD_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */


    //HAL_GPIO_WritePin(LED_MODE_GPIO_Port, LED_MODE_Pin, GPIO_PIN_SET);

    //HAL_Delay(1000);

    HAL_UART_Receive_IT(portLOG, (uint8_t *)&uRxByte, 1);
    HAL_UART_Receive_IT(portMODEM, (uint8_t *)&uRxByteMod, 1);

    HAL_TIM_Base_Start_IT(timePort);

    set_Date(epoch);

    gsmAckFlag = initSRECQ(&gsmAck);

    HAL_GPIO_WritePin(MOD_WAKE_GPIO_Port, MOD_WAKE_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(MOD_RST_GPIO_Port, MOD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(MOD_PWR_ON_GPIO_Port, MOD_PWR_ON_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);

#ifdef SET_W25FLASH

    chipPresent = W25qxx_Init();
    if (!chipPresent) {
    	Report(NULL, true, "Unknown chipID (err=%u)%s", devError, eol);
    } else {
    	cid = W25qxx_getChipID();

    	pbuf = (uint8_t *)calloc(1, page_buf_size);
    	if (!pbuf) devError |= devMEM;

    	list_sector = W25qxx_getPageSize() << 2;
	}
    int32_t esec = 0;
    uint8_t idx = MAX_SEC_PRN;

#endif

#ifdef SET_FAT_SD

    FRESULT rt = -1;

    //f_mount(NULL, (const TCHAR *)SDPath, 1);
    HAL_Delay(1000);

	if ((mnt = drvMount(SDPath))) {
		//
		DWORD chislo = 0;
		FATFS *fs = &SDFatFS;
		f_getfree((const TCHAR *)SDPath, &chislo, &fs);	/* Get number of free clusters on the drive */
		Report(NULL, true, "Free claster on drive '%s' is %u %s", SDPath, chislo, eol);
		//------------------------------------------------------------------------
		if ((rt = dirList(dirName)) == FR_OK) {
			//
			//sprintf(strf, "Project atb_f429 %s (atb-mini)%s\n", VERSION, eol);
			//wrFile(cfg, strf, true);
			//
			//HAL_Delay(500);
			rdFile(cfg);
			//
		} else {
			Report(__func__, true, "Error dirlist('%s%s')='%s'%s", SDPath, dirName, fsErrName(rt), eol);
		}
		//------------------------------------------------------------------------
		//if (f_mount(NULL, (const TCHAR *)SDPath, 1) == FR_OK) mnt = false;
		//Report(__func__, true, "Umount drive '%.*s'%s", sizeof(SDPath), SDPath, eol);
	} else {
		Report(__func__, true, "Mount disk Error '%s'%s", fsErrName(mnt), eol);
	}

	//
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


	int32_t snext = -1;
    //uint32_t tmr = get_tmr(2);
    const uint32_t waits = 180;
    int pval = -1;
    uint32_t rst_tmr = 0;//get_ms250(_1s);

    strob = get_tmr(waits);//set timer to 3 minutes

    cmd = sGetMsg;//sNone;//sModRst;

    while (!QuitLoop) {

      if (gsmAckFlag) {//command to GSM module queue is ready
    	  atAck[0] = '\0';
    	  if (getSRECQ(atAck, &gsmAck) >= 0) {
    		  if (strlen(atAck)) Report(NULL, false, "%s\n", atAck);
    	  }
      }

      /*if (lenVCP) {
    	  if (strchr(rxVCP, '\n')) {
    		  Report(__func__, true, "%s", rxVCP);
    		  lenVCP = 0;
    		  memset(rxVCP, 0, sizeof(rxVCP));
    	  }
      }*/

	  /*if (check_tmr(tmr)) {
		  tmr = get_tmr(30);
		  sprintf(oBuf, "msec=%lu Packet number=%d (err=%u)%s", HAL_GetTick(), np++, devError, eol);
		  if (snext) Report(NULL, true, "%s", oBuf);
	  }*/

	  switch (cmd) {
	  	  case sRestart:
	  		  QuitLoop = true;
	  		  Report(NULL, true, "Go go restart STM32F429...%s", eol);
	  		  if (mnt) {
	  			  f_mount(NULL, (const TCHAR *)SDPath, 1);
	  			  mnt = false;
	  		  }
	  		  NVIC_SystemReset();
	  		  break;
		  break;
	  	  case sGetMsg:
	  		  //tmr = get_tmr(0);
	  		  sprintf(oBuf, "msec=%lu Packet number=%d (err=%u)%s", HAL_GetTick(), np++, devError, eol);
	  		  if (snext) Report(NULL, true, "%s", oBuf);
	  		  cmd = sNone;
		  break;
	  	  case sErase:
#ifdef SET_W25FLASH
	  		  if (chipPresent) {
	  			  if (esector == W25qxx_getSectorCount()) {
	  				  if (snext == -1) {
	  					  snext = 0;
	  					  idx = MAX_SEC_PRN;
	  					  //wt = 1;
	  					  Report(NULL, true, "Start erase all sector's (%u)...%s", W25qxx_getSectorCount(), eol);
	  				  }
	  			  } else {
	  				  W25qxx_EraseSector(esector, 1);
	  				  cmd = sRead;
	  				  strob = get_tmr(waits);
	  				  break;
	  			  }
	  			  //tmr = get_tmr(period);
	  		  } else Report(NULL, true, "Unknown chipID (err=%u)%s", devError, eol);
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sRead:
#ifdef SET_W25FLASH
	  		  if (chipPresent) {
	  			  if (pbuf) {
	  				  W25qxx_ReadPage(pbuf, rpage, 0, page_buf_size);
	  				  Report(NULL, 1, "Read page #%lu sector #%lu:%s", rpage, W25qxx_PageToSector(rpage), eol);
	  				  printBuffer(pbuf, page_buf_size, 32);
	  			  }
	  			  //tmr = get_tmr(period);
	  	  	  } else Report(NULL, true, "Unknown chipID (err=%u)%s", devError, eol);
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sFill:
#ifdef SET_W25FLASH
	  		  if (chipPresent) {
	  			  if (pbuf) {
	  				  W25qxx_FillPage(byte_fill, rpage, 0, W25qxx_getPageSize());
	  				  Report(NULL, 1, "Fill page #%lu sector #%lu to 0x%X%s", rpage, W25qxx_PageToSector(rpage), byte_fill, eol);
	  				  cmd = sRead;
	  				  strob = get_tmr(waits);
	  				  break;
	  			  }
	  			  //tmr = get_tmr(period);
	  		  } else Report(NULL, true, "Unknown chipID (err=%u)%s", devError, eol);
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sRdDir:
#ifdef SET_FAT_SD
	  		  if (strlen(str_name)) {
	  			  if (!mnt) mnt = drvMount(SDPath);
	  			  if (mnt) dirList(str_name);
	  			  memset(str_name, 0, sizeof(str_name));
	  			  //tmr = get_tmr(period);
	  		  }
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sMkDir:
#ifdef SET_FAT_SD
	  		  if (strlen(str_name)) {
	  			  if (!mnt) mnt = drvMount(SDPath);
	  			  if (mnt) {
	  				  f_mkdir(str_name);
	  				  dirList(dirName);
	  				  memset(str_name, 0, sizeof(str_name));
	  				  //tmr = get_tmr(period);
	  			  }
	  		  }
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sRdFile:
#ifdef SET_FAT_SD
	  		  if (strlen(str_name)) {
	  			  if (!mnt) mnt = drvMount(SDPath);
	  			  if (mnt) rdFile(str_name);
	  			  memset(str_name, 0, sizeof(str_name));
	  			  //tmr = get_tmr(period);
	  		  }
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sRemove:
#ifdef SET_FAT_SD
	  		  if (strlen(str_name)) {
	  			  if (!mnt) mnt = drvMount(SDPath);
	  			  if (mnt) {
	  				  f_unlink(str_name);
	  				  memset(str_name, 0, sizeof(str_name));
	  				  dirList(dirName);
	  				  //tmr = get_tmr(period);
	  			  }
	  		  }
#endif
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  break;
	  	  case sModCmd:
	  	  {
	  		  int dln = strlen(atCmd);
	  		  if (dln) {
	  			  strcat(atCmd, eol);
	  			  dln += 2;
	  			  putCmd(atCmd, dln);
	  			  Report(NULL, false, "%d:%.*s%s", icmd, dln - 4, atCmd + 2, eol);
	  			  //tmr = get_tmr(period);
	  			  HAL_Delay(50);
	  		  }
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  	  }
	  	  break;
	  	  case sModAck:
	  		  if (strlen(atAck)) Report(NULL, false, "%s", atAck);
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  		  //tmr = get_tmr(period);
	  	  break;
	  	  case sModPwrOn:
	  	  case sModPwrOff:
	  	  {
	  		  if (cmd == sModPwrOff) pval = GPIO_PIN_RESET; else pval = GPIO_PIN_SET;
	  		  HAL_GPIO_WritePin(MOD_PWR_ON_GPIO_Port, MOD_PWR_ON_Pin, pval);
	  		  strob = get_tmr(waits);
	  		  cmd = sNone;
	  		  //tmr = get_tmr(period);
	  	  }
	  	  break;
	  	  case sModRst:
	  		  pval = GPIO_PIN_SET;
	  		  HAL_GPIO_WritePin(MOD_RST_GPIO_Port, MOD_RST_Pin, pval);
	  		  Report(NULL, true, "MOD_RST_PIN=%d%s", pval, eol);
	  		  strob = get_tmr(waits);
	  		  rst_tmr = get_tmr(2);
	  		  cmd = sNone;
	  		  //tmr = get_tmr(period);
	  	  break;
	  }

	  if (QuitLoop) break;

	  if (rst_tmr) {
		  if (check_tmr(rst_tmr)) {
			  rst_tmr = 0;
			  pval = GPIO_PIN_RESET;
			  HAL_GPIO_WritePin(MOD_RST_GPIO_Port, MOD_RST_Pin, pval);
			  Report(NULL, true, "MOD_RST_PIN=%d%s", pval, eol);
		  }
	  }
#ifdef SET_W25FLASH
	  if (!snext) {
		  W25qxx_EraseSector(esec++, 0);
		  idx--;
		  if (!idx) idx = MAX_SEC_PRN;
		  if (esec == W25qxx_getSectorCount()) {
			  Report(NULL, true, "Done erase all sector's (%u)%s", W25qxx_getSectorCount(), eol);
			  snext = -1;
			  //wt = 100;
		  }
		  strob = get_tmr(waits);
	  }
#endif


	  //HAL_Delay(wt);

	  if (strob) {
		  if (check_tmr(strob)) {
			  QuitLoop = 0;
			  strob = 0;
		  }
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    }

#ifdef SET_W25FLASH
    if (pbuf) free(pbuf);
#endif

    Report(NULL, true, "Quit command received.%s", eol);

    f_mount(NULL, (const TCHAR *)SDPath, 1);

    LOOP_FOREVER();

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 288;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x14;
  sDate.Year = 0x24;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_4B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 48;
  /* USER CODE BEGIN SDIO_Init 2 */
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 35999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 499;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

	//       MODEM

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, SPI4_RST_Pin|SPI4_HOLD_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MOD_WAKE_GPIO_Port, MOD_WAKE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MOD_RST_GPIO_Port, MOD_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, MOD_PWR_ON_Pin|LED_MODE_Pin|SPI4_WP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI4_CS1_GPIO_Port, SPI4_CS1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : SPI4_RST_Pin */
  GPIO_InitStruct.Pin = SPI4_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI4_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MOD_WAKE_Pin */
  GPIO_InitStruct.Pin = MOD_WAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MOD_WAKE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MOD_RST_Pin */
  GPIO_InitStruct.Pin = MOD_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MOD_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MOD_PWR_ON_Pin */
  GPIO_InitStruct.Pin = MOD_PWR_ON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MOD_PWR_ON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_MODE_Pin SPI4_HOLD_Pin SPI4_WP_Pin */
  GPIO_InitStruct.Pin = LED_MODE_Pin|SPI4_HOLD_Pin|SPI4_WP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI4_CS1_Pin */
  GPIO_InitStruct.Pin = SPI4_CS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI4_CS1_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

	if (htim->Instance == TIM2) {
		vatra++;
		if ((vatra & 3) == 3) {
			seconda++;
			HAL_GPIO_TogglePin(LED_MODE_GPIO_Port, LED_MODE_Pin);
		}
		//
		CDC_Recv((uint8_t *)rxVCP, &lenVCP);
	}

  /* USER CODE END Callback 0 */
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}
//------------------------------------------------------------------------------------------
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART3) uartRdy = 1;
	else
	if (huart->Instance == USART2) uartRdyMod = 1;
}
//------------------------------------------------------------------------------------------
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2) {
		//
		if ((uRxByteMod > 0x0d) && (uRxByteMod < 0x80)) {
			if (uRxByteMod >= 0x20) adone = 1;
			if (adone) RxBufMod[rx_uk_mod] = (char)uRxByteMod;
		}
		//
		if (adone) {
			if ( (uRxByteMod == 0x0a) || (uRxByteMod == 0x3e) ) {// '\n' || '>'
				//if (uRxByteMod != 0x3e) strcat(RxBufMod, "\r\n");//0x0D 0x0A
				//if (strstr(RxBufMod, "OK")) {
					int len = strlen(RxBufMod);
					if (gsmAckFlag) {
						// Блок помещает в очередь ответов на команду очередное сообщение от модуля GSM
						char *from = (char *)calloc(1, len + 1);
						if (from) {
							memcpy(from, RxBufMod, len);
							if (putSRECQ(from, &gsmAck) < 0) {
								devError |= devQue;
								free(from);
							} else {
								if (devError & devQue) devError &= ~devQue;
							}
						} else {
							devError |= devMEM;
						}
						//-----------------------------------------------------------------------------
					}
				//}
				//strcpy(atAck, RxBufMod);
				rx_uk_mod = 0;
				memset(RxBufMod, 0, sizeof(RxBufMod));
				//cmd = sModAck;
			} else rx_uk_mod++;
		}

		HAL_UART_Receive_IT(huart, (uint8_t *)&uRxByteMod, 1);
	}
	else
	if (huart->Instance == USART3) {
		RxBuf[rx_uk & 0xff] = (char)uRxByte;
		if ((uRxByte == 0x0a) && (rx_uk >= 2)) {//end of line
			char *uk = NULL;
			icmd = sNone;
			//
			uk = strchr(RxBuf, '\r');
			if (uk) *uk = '\0';
			else {
				if ((uk = strchr(RxBuf, '\n'))) *uk = '\0';
			}
			//
			if ((uk = strstr(RxBuf, "epoch="))) {
				uint32_t ep = atol(uk + 6);
				if (ep > epoch) {
					set_Date(ep);
					cmd = sGetMsg;
				}
			} else if ((uk = strstr(RxBuf, "quit"))) {
				HAL_GPIO_WritePin(MOD_PWR_ON_GPIO_Port, MOD_PWR_ON_Pin, GPIO_PIN_RESET);
				QuitLoop = true;
			} else if ((uk = strstr(RxBuf, "restart"))) {
				if (!QuitLoop) {
					cmd = sRestart;
				} else {
					NVIC_SystemReset();
				}
			} else if ((uk = strstr(RxBuf, "get"))) {
				cmd = sGetMsg;
			} else if ( ((uk = strstr(RxBuf, "m:"))) || ((uk = strstr(RxBuf, "M:"))) ) {
				uk += 2;
				if (strstr(uk, "AT")) {
					sprintf(atCmd, "\r\n%s", uk);
					//
					icmd = getCmdInd(atCmd);
					//
					cmd = sModCmd;
				} else if (strstr(uk, "PWR_ON")) {//MOD PWR = 1
					cmd = sModPwrOn;
				} else if (strstr(uk, "PWR_OFF")) {//MOD PWR = 0
					cmd = sModPwrOff;
				} else if (strstr(uk, "RST")) {//MOD RESET = 0
					cmd = sModRst;
				} else if (strstr(uk, "Z")) {//Ctrl+Z
					sprintf(atCmd, "%c\r\n", 0x1a);
					cmd = sModCmd;
				} else {
					sprintf(atCmd, "%s%c", uk, 0x1a);
					cmd = sModCmd;
				}
			}
#ifdef SET_FAT_SD
			else if ((uk = strstr(RxBuf, "cat="))) {
				uk += 4;
				strcpy(str_name, uk);
				cmd = sRdFile;
			} else if ((uk = strstr(RxBuf, "mkdir="))) {
				uk += 6;
				strcpy(str_name, uk);
				cmd = sMkDir;
			} else if ((uk = strstr(RxBuf, "dir="))) {
				uk += 4;
				strcpy(str_name, uk);
				cmd = sRdDir;
			} else if ((uk = strstr(RxBuf, "rm="))) {
				uk += 3;
				strcpy(str_name, uk);
				cmd = sRemove;
			}
#endif
#ifdef SET_W25FLASH
			else if ((uk = strstr(RxBuf, "erase="))) {//erase:0 //erase:max = erase:4096
				if (cmd == sNone) {
					uint32_t item;
					if (strstr(uk + 6, "max")) item = w25qxx.SectorCount;//erase all sectors
										  else item = atol(uk + 6);      //erase sector
					if ((item >= 0) && (item <= w25qxx.SectorCount)) {
						esector = item;
						cmd = sErase;
					}
				}
			} else if ((uk = strstr(RxBuf, "read="))) {
				if (cmd == sNone) {
					uint32_t page = atol(uk + 5);
					if ((page >= 0) && (page < w25qxx.PageCount)) {
						rpage = page;
						cmd = sRead;
					}
				}
			} else if ((uk = strstr(RxBuf, "fill="))) {//fill=0:127
				uk += 5;//pointer to page number
				char *uki = strchr(uk, ':');
				if (uki) {
					char *ukp = uki;
					uki++;
					if (strstr(uki, "0x")) {
						uki += 2;
						byte_fill = hexToBin(uki);
					} else byte_fill = atol(uki + 1);
					*ukp = '\0';
					uint32_t page = atol(uk);
					if ((page >= 0) && (page < W25qxx_getPageCount())) {
						rpage = page;
						cmd = sFill;
					}
				}
			}
#endif
			rx_uk = 0;
			memset(RxBuf, 0, sizeof(RxBuf));
		} else rx_uk++;

		HAL_UART_Receive_IT(huart, (uint8_t *)&uRxByte, 1);
	}
}
//------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
#ifdef SET_W25FLASH
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI4) {//FLASH
		spi_cnt++;
		spiRdy = 1;
	#ifdef SET_W25FLASH_DMA
		if (w25_withDMA) {
			W25_UNSELECT();
			//putMsg(msg_spiTxDone);
		}
	#endif
	}
}
//-------------------------------------------------------------------------------------------
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI4) {//FLASH
		spi_cnt++;
		spiRdy = 1;
	#ifdef SET_W25FLASH_DMA
		if (w25_withDMA) {
			W25_UNSELECT();
			//putMsg(msg_spiRxDone);
		}
	#endif
	}
}
#endif
//-------------------------------------------------------------------------------------------
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
