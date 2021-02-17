# SeedCom
Serial comms utility for the Electrosmith Daisy Seed

This is a windows command line system.

Seedcom allows you to do a number of things:

1) Upload files to the seeds external flash memory

2) reboot the seed into DFU mode

3) View the files in flash memory

4) View general debug output

## How to install?

1) Make seedcom.exe

type seedcom.bat on the commandline - if you have gcc setup this will make the .exe for you

2) copy the libdaisy/coremakefile into libdaisy/core
this adds new make commands
REBOOT
file

3) 
add SeedComServer.cpp to your local makefile
```
# Sources
CPP_SOURCES = MyMain.cpp SeedComServer.cpp
```
add SeedComServer.h to your main cpp file
```
#include "SeedComServer.h"
int main(void)
{
    UartInitSerial(&seed,14); // 14 is a gpio pin with a switch that will also reboot for you!
    while(1) 
        {
        UartShouldReboot();
        }
}
```
## How do I change the transfer speed?

currently it's hardcoded!

\DaisyExamples\libdaisy\src\usbd\usbd_cdc_if.c line 204

static uint8_t line_coding_fs[7] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08 };

change to

static uint8_t line_coding_fs[7] = {0x00, 0x10, 0x0E, 0x00, 0x00, 0x00, 0x08};

rebuild your libdaisy.a

max speed so far is 921600bps (100KB/s) - tested and works fine


## How do I upload files to flash

create a directory called QFLashZip in your main project folder, put some files in there - images,txt whatever

"make zip" - will zip those files up into a file QFlashData.zip in your build directory

"seedcom build/qflashdata.zip reboot" - will upload the QFlashData.zip to your seed external flash and reboot the seed into DFU mode

## How does this work?

we send txt commands from the PC to seed via serial uart:

- SEEDHELO - seed will return SEEDGOOD
- SEEDBOOT - seed will reboot into DFU mode
- SEEDFILE - seed will expect file info and transfer raw data into external flash (see protocol below)
- SEEDDUMP - seed will list all the files currently in external flash file "QFlashData.zip"
- SEEDMEMO - seed will dump memory usage

The file transfer protocol is:
-	PC sends csv data:
-	FileName,FileSize,FileSize2,CRC32,Type,.....raw file data in 1024 byte chunks....
-	FileName is a standard string - without the null char
- 	FileSize,FileSize2,CRC32 and Type are sent as raw DWORD's

Seed will only receive the file if the CRC is different - this is handy - no need to touch the files and
it saves on time and flash write wear.

Seed will send back SEEDGOOD after every 1024 byte chunk and the final chunk has been received

Seed will send back SEEDFAIL is anything goes wrong or it refuses the file

For the seed - the filename will currently always be "QFlashData.zip"

Currently only supports 1 file in flash, but since it's a .zip file - it contains a whole file system structure
and SEEDDUMP will list all those files inside.

## TODO

Flash writing
Zip Loading/Decoding e.t.c.

