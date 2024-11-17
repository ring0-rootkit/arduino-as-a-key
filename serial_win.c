#include<windows.h>
#include<stdio.h>

#define MAX_BYTES_READ 1024

#define SECRET_MSG "Secret message no one supposed to see :)"
#define LSECRET_MSG TEXT(SECRET_MSG)
#define LERR_MSG TEXT("Error, pls insert the key")

const char* COM_PORT_NAME = "\\\\.\\COM3";

#define CHECK_ERR() {\
	DWORD error = GetLastError();\
	printf("failed with error code: %d\n", error);\
	MessageBoxW(NULL, LERR_MSG, L"Error", MB_ICONERROR);\
}

const char* key = "approved";

void SetDCBParams(HANDLE serial) {
	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(serial, &dcbSerialParams)) {
		printf("error while getting com port state, exiting...");
		exit(1);
	}
	dcbSerialParams.BaudRate = CBR_9600;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (!SetCommState(serial, &dcbSerialParams)) {
		CHECK_ERR();
		printf("error while setting com port state, exiting...");
		exit(1);
	}
}

void SetTimeoutParams(HANDLE serial) {
	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (!SetCommTimeouts(serial, &timeouts)) {
		CHECK_ERR();
		printf("error while setting timeouts, exiting...");
		exit(1);
	}
}

void SetParams(HANDLE serial) {
	SetDCBParams(serial);
	SetTimeoutParams(serial);
}

HANDLE OpenSerial(int idx) {
	TCHAR comname[100];
	wsprintf(comname, TEXT("\\\\.\\COM%d"), idx);
	HANDLE serial = CreateFile(comname,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (serial == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			CHECK_ERR();
			printf("no serial port found, exiting...");
			exit(1);
		}
		CHECK_ERR();
		printf("some error occured, exiting...");
		exit(1);
	}
	SetParams(serial);

	return serial;
}
int Write(HANDLE com_port, const char* data)
{
	DWORD  noOFBytestoWrite = strlen(data);
	DWORD  noOfBytesWritten;
	BOOL Status = WriteFile(com_port,
		data,
		noOFBytestoWrite,
		&noOfBytesWritten,
		NULL);
	if (Status == FALSE)
		return -1;
	return noOfBytesWritten;
}

int Read(HANDLE com_port, char* data, int len)
{
	DWORD noBytesRead = { 0 };
	
	BOOL Status = ReadFile(com_port, data, len, &noBytesRead, NULL);
	data[noBytesRead] = '\0';
	printf("read:%d bytes\n", noBytesRead);
	return Status;
}

int main() {
	HANDLE serial = OpenSerial(3);

	char* data = malloc(MAX_BYTES_READ + 1);
	if (data == NULL) {
		exit(1);
	}
	memset(data, '\0', MAX_BYTES_READ);

	int ok = Read(serial, data, MAX_BYTES_READ);
	if (!ok) {
		printf("cannot read, exiting...\n");
		CloseHandle(serial);
		return 1;
	}
	printf("%s\n", data);

	int n = Write(serial, "get_data\n");
	if (n < 0) {
		printf("cannot write, exiting...");
		CloseHandle(serial);
		return 1;
	}
	printf("written:%d bytes\n", n);

	ok = Read(serial, data, MAX_BYTES_READ);
	if (!ok) {
		printf("cannot read, exiting...");
		CloseHandle(serial);
		return 1;
	}
	printf("%s\n", data);

	if (strncmp(data, key, strlen(key)) == 0) {
		printf("%s\n", SECRET_MSG);
		MessageBoxW(NULL, LSECRET_MSG, L"Nice ;)", MB_ICONEXCLAMATION);
	}

	free(data);
	CloseHandle(serial);
}
