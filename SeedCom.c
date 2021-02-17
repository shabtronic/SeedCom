#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
const int UartSpeed = 115200;
const int UartBits = 8;
const int UartStopBits = 1;
DCB dcbSerialParams = { 0 };
#define ConsumeTimeout 5

#define ORIGIN "PC: "
#define ANSIBLACK "\033[0;30m"
#define ANSIRED "\033[0;31m"
#define ANSIGREEN "\033[0;32m"
#define ANSIYELLOW "\033[0;33m"
#define ANSIBLUE "\033[0;34m"
#define ANSIMAGENTA "\033[0;35m"
#define ANSICYAN "\033[0;36m"
#define ANSIWHITE "\033[0m"

// SEEDHELO - seed will return SEEDGOOD
// SEEDBOOT - seed will reboot into DFU mode
// SEEDFILE - seed will expect file info and transfer raw data into external flash (see protocol below)
// SEEDDUMP - seed will list all the files currently in external flash file "QFlashData.zip"
// SEEDMEMO - seed will dump memory usage

// The file transfer protocol is:
//		PC sends this csv data:
//			FileName,FileSize,FileSize2,CRC32,Type,.....raw file data in 1024 byte chunks....
//
//		FileName is a standard string - without the null char
//		FileSize,FileSize2,CRC32 and Type are sent as raw DWORD's

//		Seed will send back SEEDGOOD after every 1024 byte chunk and the final chunk has been received

// For the seed - the filename will currently always be "QFlashData.zip"

// Currently only supports 1 file in flash, but since it's a .zip file - it contains a whole file system structure
// and SEEDDUMP will list all those files inside.


// printf doesn't redirect in mingw64 - so this:
HANDLE screen;
void WriteScreen(char* format, ...)
	{
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 255, format, args);
	DWORD written;
	WriteFile(screen, buffer, strlen(buffer), &written, NULL);
	va_end(args);
	}

char cornerset[] = "│─┐└┘┌";
char conerset2[] = "║═╗╚╝╔";

unsigned int crc32(const void* data, unsigned int n_bytes)
	{
	unsigned int crc = 0;
	static unsigned int table[0x100] = { 0 };
	if (!*table)
		for (unsigned int i = 0; i < 0x100; i++)
			{
			unsigned int xi = i;
			for (int j = 0; j < 8; j++)
				xi = (xi & 1 ? 0 : (unsigned int)0xEDB88320L) ^ xi >> 1;
			table[i] = (xi ^ (unsigned int)0xFF000000L);
			}
	while (n_bytes--)
		crc = table[(unsigned char)crc ^ (*(unsigned char*)data++)] ^ crc >> 8;
	return crc;
	}



HANDLE OpenPort(char* Name)
	{
	HANDLE hSerial = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hSerial == INVALID_HANDLE_VALUE)
		{
		WriteScreen(ANSIRED"Error opening: %s\n"ANSIWHITE, Name);
		return 0;
		}
	return hSerial;
	}

int FileExists(char* filename)
	{
	FILE* F = fopen(filename, "rb");
	if (F)
		{
		fclose(F);
		return 1;
		}
	return 0;
	}

void PortConfig(HANDLE port, int Speed)
	{

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	GetCommState(port, &dcbSerialParams);

	WriteScreen("ComPort Baud %d data %d stop %d \n", dcbSerialParams.BaudRate, dcbSerialParams.ByteSize, (int)dcbSerialParams.StopBits);
	char Temp[256];
	sprintf(Temp, "baud=%d parity=n data=8 stop=1", dcbSerialParams.BaudRate);
	if (!BuildCommDCB(Temp, &port))
		WriteScreen("error building comm DCB");
	if (!SetCommState(port, &dcbSerialParams))
		WriteScreen("error Setting commstate\n");

	GetCommState(port, &dcbSerialParams);

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	if (!SetCommTimeouts(port, &timeouts))
		WriteScreen("error setCommTimeouts!\n");

	if (!EscapeCommFunction(port, CLRDTR))
		WriteScreen("clearing DTR\n");
	Sleep(200);
	if (!EscapeCommFunction(port, SETDTR))
		WriteScreen("error setting DTR\n");
	}


double GetTime()
	{
	__int64 time1 = 0, time2 = 0, freq = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&time1);
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	return (double)time1 / freq;
	}

int WriteString(HANDLE cp, char* m)
	{
	DWORD BytesWritten;
	for (int a = 0; a < strlen(m); a++)
		WriteFile(cp, &m[a], 1, &BytesWritten, NULL);
	return BytesWritten;
	}

int WriteBytes(HANDLE cp, char* buf, int size)
	{
	DWORD BytesWritten;
	for (int a = 0; a < size; a++)
		WriteFile(cp, &buf[a], 1, &BytesWritten, NULL);
	return BytesWritten;
	}
int WriteDWORD(HANDLE cp, DWORD data)
	{
	DWORD BytesWritten;
	char* dat = (char*)&data;
	for (int a = 0; a < 4; a++)
		WriteFile(cp, &dat[a], 1, &BytesWritten, NULL);
	return BytesWritten;
	}

int Consume(HANDLE ComPort, char* EatMe, int TimeOut)
	{
	char* teatme = EatMe;
	while (*teatme && TimeOut)
		{
		char Byte;
		DWORD dwBytesRead;
		ReadFile(ComPort, &Byte, 1, &dwBytesRead, NULL);

		if (dwBytesRead == 1)
			{
			if (Byte == *teatme)
				teatme++;
			else
				teatme = EatMe;
			}
		else
			{
			WriteScreen("Timedout!\n");
			TimeOut--;
			}
		}
	if (*teatme)
		return 0;
	return 1;
	}
void SendFile(HANDLE comport, char* filename)
	{
	WriteScreen("PC: Sending File %s to Seed\n", filename);
	FILE* F = fopen(filename, "rb");
	if (F)
		{
		double st = GetTime();
		fseek(F, 0, SEEK_END);
		DWORD fsize = ftell(F);
		DWORD ofsize = fsize;
		fseek(F, 0, SEEK_SET);
		char* Data = (char*)malloc(fsize);
		fread(Data, 1, fsize, F);
		fclose(F);
		WriteScreen("PC: file size is %2.1fKb\n", (float)fsize / (1024));
		DWORD crc = crc32(Data, fsize);

		WriteString(comport, "SEEDFILE");
		// We Send 1024 bytes of data 

		WriteDWORD(comport, strlen(filename));
		WriteDWORD(comport, fsize);
		WriteDWORD(comport, fsize);
		WriteDWORD(comport, crc);
		WriteDWORD(comport, (DWORD)'RAW ');
		WriteString(comport, filename);
		char temp[1024];
		memset(temp, 0, 1024);
		WriteBytes(comport, temp, 1024 - (20 + strlen(filename)));

		char* tdata = Data;

		WriteScreen(ANSIYELLOW"PC: File Transfer: "ANSIWHITE);
		while (fsize)
			{
			float percent = 100 - 100 * ((float)fsize / ofsize);
			WriteScreen(ANSIGREEN"%5.1f%%\b\b\b\b\b\b"ANSIGREEN, percent);
			if (fsize >= 1024)
				{
				if (WriteBytes(comport, tdata, 1024) != 1024 || !Consume(comport, "SEEDGOOD", 50))
					{
					WriteScreen(ANSIRED"\nPC: Error Sending File Chunk\n"ANSIWHITE);
					goto cleanup;
					}
				tdata += 1024;
				fsize -= 1024;
				}
			else
				{
				if (WriteBytes(comport, tdata, fsize) != fsize || !Consume(comport, "SEEDGOOD", 500))
					{
					WriteScreen(ANSIRED"\nPC: Error Sending File Chunk\n"ANSIWHITE);
					goto cleanup;
					}
				fsize = 0;
				st = GetTime() - st;
				WriteScreen(ANSIGREEN"100%%  in %2.1f secs\n\n"ANSIWHITE, st);
				}
			}
	cleanup:
		free(Data);
		}
	else
		WriteScreen("PC: error file %s not found\n", filename);
	}
HANDLE EnumComPorts()
	{
	char Res[1024];
	static char ComString[8];
	for (int a = 0; a < 50; a++)
		{
		sprintf(ComString, "COM%d", a);
		if (QueryDosDevice(ComString, Res, 1024) > 0)
			{
			HANDLE ComPort = OpenPort(ComString);
			PortConfig(ComPort, UartSpeed);
			WriteString(ComPort, "SEEDHELO");
			if (Consume(ComPort, "SEEDGOOD", 15))
				{
				WriteScreen("PC: Found Seed on %s %s\n", ComString, Res);
				return ComPort;
				}
			CloseHandle(ComPort);
			}
		}
	return 0;
	}





int main(int nArgC, char** ppArgV)
	{
	screen = GetStdHandle(STD_OUTPUT_HANDLE);
	char* pArg0 = *ppArgV++;
	nArgC--;
	system("cls");
	WriteScreen(ANSIGREEN"SeedCom V1.00 Serial Daisy Seed Comms Utility (C) 2021 S.D.Smith\n\n"ANSIWHITE);

	HANDLE ComPort = EnumComPorts();
	if (!ComPort)
		{
		WriteScreen(ANSIRED"\nPC: Daisy Seed not found :(\n"ANSIWHITE);
		return 0;
		}
	WriteScreen("PC: Uart speed is %d = %2.1fKb/s\n", dcbSerialParams.BaudRate, (float)dcbSerialParams.BaudRate / (9 * 1024));

	char* filesend = 0;
	int Reboot = 0;
	for (int a = 0; a < nArgC; a++)
		{
		if (FileExists(ppArgV[a]))
			filesend = ppArgV[a];
		if (strstr(ppArgV[a], "reboot") > 0)
			Reboot = 1;
		}

	if (filesend)
		SendFile(ComPort, filesend);
	char Buf[256] = { 0 };
	while (1)
		{
		DWORD written;
		DWORD dwBytesRead = 0;
		char ch;
		ReadFile(ComPort, Buf, 1, &dwBytesRead, NULL);
		Buf[dwBytesRead] = 0;
		
		WriteScreen("%s", Buf);

		while (kbhit())
			{
			ch = getch();
			if (ch == 13) WriteScreen("\n");
			WriteFile(ComPort, &ch, 1, &written, NULL);
			}
		if (Reboot)
			{
			WriteScreen("PC: Rebooting Seed\n");
			WriteString(ComPort, "SEEDBOOT");
			Reboot = 0;
			}
		}
	CloseHandle(ComPort);
	return 0;
	}
