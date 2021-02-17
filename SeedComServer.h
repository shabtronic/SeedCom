#pragma once
#ifndef SEEDCOMSERVER_H
#define SEEDCOMSERVER_H
#ifdef __cplusplus

#include "daisysp.h"
#include "daisy_seed.h"

// Changing usb uart speed:
// \DaisyExamples\libdaisy\src\usbd\usbd_cdc_if.c line 204
//static uint8_t line_coding_fs[7] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08 };

using namespace daisysp;
using namespace daisy;

#define SEED_ASSERT(x,y)

// Simple SDRAM struct
struct FlashFile
	{
	char* Data;
	int DataSize;
	};

// Copies a file from the QFlashData.zip in external flash into SDRAM - if it exists
// .Data and .DataSize will be zero if the file doesn't exist
// returns true or false if the file exists or not
// This will automatically decompress from the zip file for you

bool GetFlashFile(char*, FlashFile& FF);

// Free up any SDRAM used by FlashFile
void FreeFlashFileRam(FlashFile& FF);

// If you want to reboot yourself in code
void RebootToBootloader();

// Put in main while loop - checks to see if we should reboot
void UartShouldReboot();

// Initialize the comms, also sets up a GPIO button for rebooting
void UartInitSerial(DaisySeed *hw,int RebootButton);


#endif
#endif