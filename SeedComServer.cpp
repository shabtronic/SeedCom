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
	seed->usb_handle.TransmitInternal((uint8_t*)msg, strlen(msg));
	}
int BytesNeeded = 0;
void UsbSerialServer(uint8_t* buf, uint32_t* len)
	{
	for (size_t i = 0; i < *len; i++)
		{
		if (BytesNeeded)
			{

			}
		else
			{
			CommandShift = (CommandShift << 8) + buf[i];
			// Last char in command doesnt get echoed for some reason
			// TransmitInternal doesn't block until the previous data sent?
			// or some kind of race condition
			// so I add the last command char on the front of the
			// return resonse string
			// probably better to concat everything into one string and send that


			if (CommandShift == sHELO)
				UartSerialSend((const char*)"O SEEDGOOD\u001b[32m\n\r>>>HELO PC from Seed!\n\r\u001b[37m");
			if (CommandShift == sBOOT)
				{
				Reboot = true; // cant reboot inside this callback!
				}

			seed->usb_handle.TransmitInternal((uint8_t*)&buf[i], 1);
			}
		}
	}
void UartShouldReboot()
	{
	Uartbutton.Debounce();
	if (Uartbutton.Pressed() || Reboot)
		{
		UartSerialSend((const char*)"\u001b[32m\n\r>>>Seed rebooting to DFU loader!\n\r\u001b[37m");

		System::Delay(50);
		RebootToBootloader();
		}
	}

extern "C" uint8_t line_coding_fs[7];


void UartInitSerial(DaisySeed* hw,uint32_t BRate, int RebootButton)
	{
	uint32_t* BaudRate = (uint32_t*)line_coding_fs;
	*BaudRate = BRate;
	SEED_ASSERT(hw, "Error passed in a null Daisyseed hardware!");
	seed = hw;
	if (seed)
		{
		seed->usb_handle.Init(UsbHandle::FS_INTERNAL);
		const char* msg = ">>>>Init Seed Uart server";

		seed->usb_handle.TransmitInternal((uint8_t*)msg, strlen(msg));
		System::Delay(500);
		seed->usb_handle.SetReceiveCallback(UsbSerialServer, UsbHandle::FS_INTERNAL);
		Uartbutton.Init(seed->GetPin(RebootButton), 10);
		}
	}