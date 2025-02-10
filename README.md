#################################################################################################
#
# Board with stm32f429 + microSD adapter(SDIO) + gsm modem ПН6280(UART) + flash chip S25FL128S(SPI)
#
#################################################################################################


## Состав рабочего оборудования:

```
* stm32f429 - микроконтроллер
* microSD adapter (интерфейс SDIO)
* flash memory chip (интерфейс SPI)
* gsm modem (интерфейс UART)
```


# Средства разработки:

```
* STM32CubeMX - графический пакет для создание проектов (на языке Си) под микроконтроллеры семейства STM32
  (https://www.st.com/en/development-tools/stm32cubemx.html).
* System Workbench for STM32 - IDE среда разработки ПО для микроконтроллеров семейства STM32
  (https://www.st.com/en/development-tools/sw4stm32.html).
* stm32flash - утилита для записи firmware в flash-память микроконтроллеров семейства STM32
  через встроенный порт USART1 (https://sourceforge.net/projects/stm32flash/files/)
* STM32CubeProgrammer - утилита для записи firmware в flash-память микроконтроллеров семейства STM32
  (https://www.st.com/en/development-tools/stm32cubeprog.html).
* ST-LINK V2 - usb отладчик для микроконтроллеров семейства STM8/STM32.
* Saleae USB Logic 8ch - логический анализатор сигналов, 8 каналов , макс. частота дискретизации 24МГц
  (https://www.saleae.com/ru/downloads/)
```


# Функционал:

* ПО построено по модели BARE METAL (без использования ОС) с использованием буфера событий,
  построенного по принципу fifo. События обслуживаются в основном цикле программы. Формируются события в callBack-функциях
  по завершении прерываний от используемых модулей микроконтроллера.
* Устройство инициализирует некоторые интерфейсы микроконтроллера :
  - USART2 : параметры порта 115200 8N1 - порт для взвимодействия с gsm модемом посредством AT команд.
  - USART3 : параметры порта 115200 8N1 - порт для взвимодействия с внешним устройством (например комп.).
  - TIM2 : таймер-счетчик временных интервалов в 1 мс. и 1 сек., реализован в callback-функции.
  - RTC : часы реального времени, могут быть установлены с помощью команды epoch=XXXXXXXXXX
  - SPI4 : обслуживает flash memmory chip.
  - SDIO : порт для облуживания microSD карты с файловой системой FAT32.
  - USB FS: здесь реализован виртуальный ком-порт для выдачи логов.
* Прием данных по последовательному порту (USART3) выполняется в callback-функции обработчика прерывания.
* Устройство передает АТ-команды на модем (USART2) , принятые с порта USART3 от внешнего устройства.
  в USART1, например :


```
m:ATI
ATI
Manufacturer:JSC NIIMA PROGRESS and NAVIA 
Model:PN6280
Revision:PN6280_SPv01.03b02
IMEI:35860100008758

    команда для модема : ATI
```


* Через USART2 можно отправлять команды на устройство, вот пример некоторых из них :


```
epoch=1608307455

    установить текущее время и временную зону
    Unix epoch time - 1608307455
```


```
restart

    рестарт устройства
```


```
read=0
Read page #0 sector #0:
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

    чтение страницы с номером 0 из chip flash memory
```


```
dir=<filder name>

    листинг файлов в указанной папке
````


```
cat=<полное имя файла>

    печать файла
```


```
mkdir=<имя папки>

    создать папку
```


```
rm=<полное имя файла>

    удалить файл
```

