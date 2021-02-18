#include "SeedComServer.h"
#include <string.h>

extern "C" void HAL_NVIC_SystemReset();

DaisySeed* seed = 0;

bool GetFlashFile(char*, FlashFile& FF)
	{
	return false;
	}
// Free up any SDRAM used by FlashFile
void FreeFlashFileRam(FlashFile& FF)
	{

	}

void RebootToBootloader()
	{
	// Initialize Boot Pin
	dsy_gpio_pin bootpin = { DSY_GPIOG, 3 };
	dsy_gpio pin;
	pin.mode = DSY_GPIO_MODE_OUTPUT_PP;
	pin.pin = bootpin;
	dsy_gpio_init(&pin);

	// Pull Pin HIGH
	dsy_gpio_write(&pin, 1);

	// wait a few ms for cap to charge
	seed->DelayMs(10);

	// Software Reset
	HAL_NVIC_SystemReset();

	seed->DelayMs(10);

	}

// echo hello > COM4 e.t.c.?
static uint64_t CommandShift = 0;
// connections
const uint64_t sHELO = 0x5345454448454c4f;
const uint64_t sGONE = 0x53454544474f4e45;
// acks
const uint64_t sGOOD = 0x53454544474f4f44;
const uint64_t sFAIL = 0x534545444641494c;
// commands
const uint64_t sBOOT = 0x53454544424f4f54;
const uint64_t sFILE = 0x5345454446494c45;
const uint64_t sDUMP = 0x5345454444554d50;
const uint64_t sMEMO = 0x534545444d454d4f;
const uint64_t sCSPD = 0x5345454443535044;

static bool Reboot = false;
// File transfer info
bool UartReceivingFile = false;
char* UartSentFileBuffer = 0;
char* UartSendFileName = 0;
int UartSentFileSize = 0;
int UartSentFileCRC32 = 0;
daisy::Switch Uartbutton;

#define ANSIBLACK "\033[0;30m"
#define ANSIRED "\033[0;31m"
#define ANSIGREEN "\033[0;32m"
#define ANSIYELLOW "\033[0;33m"
#define ANSIBLUE "\033[0;34m"
#define ANSIMAGENTA "\033[0;35m"
#define ANSICYAN "\033[0;36m"
#define ANSIWHITE "\033[0m"




void UartInitSentFile()
	{

	}

void UartSerialSend(const char* msg)
	{
	while (seed->usb_handle.TransmitInternal((uint8_t*)msg, strlen(msg)) == UsbHandle::Result::ERR)
		{
		System::Delay(1);
		}
	}

int BytesNeeded = 0;
#define BufSize (1024*64)
volatile int RXBuffer[BufSize];
volatile int RXRP = 0;
volatile int RXWP = 0;

bool RxDataAvailable()
	{
	return (RXRP != RXWP);
	}

uint8_t EatByte()
	{
	while (!RxDataAvailable())
		{
		System::Delay(1);
		}
	uint8_t res = RXBuffer[RXRP];
	RXRP = (RXRP + 1) & (BufSize - 1);
	return res;
	}

uint64_t UpdateCommandShift()
	{
	if (RxDataAvailable())
		{
		uint8_t data = EatByte();
		CommandShift = (CommandShift << 8) + data;
		while (seed->usb_handle.TransmitInternal((uint8_t*)&data, 1) == UsbHandle::Result::ERR)
			{
			System::Delay(1);
			}
		}
	return CommandShift;
	}

bool GetRxData(char* src, int count, int TimeOut)
	{
	return false;
	}



void UsbSerialServer(uint8_t* buf, uint32_t* len)
	{
	for (size_t i = 0; i < *len; i++)
		{
		RXBuffer[RXWP] = buf[i];
		RXWP = (RXWP + 1) & (BufSize - 1);
		}
	}
int BytesReceived = 0;
void UartShouldReboot()
	{

	uint64_t CS = 0;
	//if (RxDataAvailable())
	//	UartSerialSend((const char*)"\u001b[32m\n\r>>>Data Available\n\r\u001b[37m");


	while (BytesNeeded > 0)
		{
		EatByte();
		BytesNeeded--;
		BytesReceived++;
		if ((BytesReceived >= 0x4000))
			{
			UartSerialSend((const char*)"SEEDGOOD");
			BytesReceived = 0;
			}
		}



	while (RxDataAvailable() && BytesNeeded==0)
		{
		CS = UpdateCommandShift();

		if (CS == sHELO)
			UartSerialSend((const char*)"SEEDGOOD\u001b[32m\n\r>>>HELO PC from Seed!\n\r\u001b[37m");
		if ((CS == sBOOT))
			{
			UartSerialSend((const char*)"\u001b[32m\n\r>>>Seed rebooting to DFU loader!\n\r\u001b[37m");
			System::Delay(50);
			RebootToBootloader();
			}
		if (CommandShift == sFILE)
			{
			UartSerialSend((const char*)"\u001b[32m\n\r>>>Starting Test transfer\n\r\u001b[37mSEEDGOOD");
			BytesNeeded = 16 * 0x4000;
			BytesReceived = 0;
			}
		}


	//Uartbutton.Debounce();
	if (Uartbutton.Pressed())
		{
		//UartSerialSend((const char*)"\u001b[32m\n\r>>>Seed rebooting to DFU loader!\n\r\u001b[37m");
		System::Delay(50);
		RebootToBootloader();
		}
	}

extern "C" uint8_t line_coding_fs[7];
extern "C" uint8_t line_coding_hs[7];

void UartInitSerial(DaisySeed* hw, uint32_t BRate, int RebootButton)
	{
	//*(uint32_t*)line_coding_fs = BRate;
	//*(uint32_t*)line_coding_hs = BRate;

	SEED_ASSERT(hw, "Error passed in a null Daisyseed hardware!");
	seed = hw;
	if (seed)
		{
		Uartbutton.Init(seed->GetPin(RebootButton), 10);
		seed->usb_handle.Init(UsbHandle::FS_INTERNAL);
		const char* msg = ">>>>Init Seed Uart server";
		seed->usb_handle.TransmitInternal((uint8_t*)msg, strlen(msg));
		System::Delay(500);
		seed->usb_handle.SetReceiveCallback(UsbSerialServer, UsbHandle::FS_INTERNAL);

		}
	}