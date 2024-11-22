#include<windows.h>
#include<stdio.h>

#define MAX_BYTES_READ 1024

#define SECRET_MSG "Secret message no one supposed to see :)"
#define LSECRET_MSG TEXT(SECRET_MSG)
#define LERR_MSG TEXT("Error, pls insert the key")

void CHECK_ERR() {
	DWORD error = GetLastError();
	printf("failed with error code: %d\n", error);
	MessageBoxW(NULL, LERR_MSG, L"Error", MB_ICONERROR);
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
	return Status;
}

int isDeviceConnected(HANDLE serial) {
	char* data = malloc(MAX_BYTES_READ + 1);
	if (data == NULL) {
		exit(1);
	}
	memset(data, '\0', MAX_BYTES_READ);

	int n = Write(serial, "get_data\n");
	if (n < 0) {
		return 0;
	}

	BOOL ok = Read(serial, data, MAX_BYTES_READ);
	if (!ok) {
		return 0;
	}
	int ret = (strncmp(data, key, strlen(key)) == 0);
	free(data);
	return ret;
}

void WriteMsg(HWND hwnd, LPCWSTR text) {
	RECT rect;
	HDC wdc = GetDC(hwnd);
	GetClientRect(hwnd, &rect);
	FillRect(wdc, &rect, WHITE_BRUSH);
	SetTextColor(wdc, 0x00000000);
	SetBkMode(wdc, TRANSPARENT);
	rect.left = 0;
	rect.top = 0;
	DrawTextW(wdc, text, -1, &rect, DT_SINGLELINE | DT_NOCLIP);
	ReleaseDC(hwnd, wdc);
}

void ShowSecret(HANDLE serial, HWND hwnd, char* data) {
	if (strncmp(data, key, strlen(key)) == 0) {
		WriteMsg(hwnd, LSECRET_MSG);
	}
	while (isDeviceConnected(serial)) {
		Sleep(1000);
	}
	WriteMsg(hwnd, TEXT("No key"));
	MessageBoxW(NULL, L"Key has been disconnected, connect key and restart program", L"Error", MB_ICONERROR);
}

int main() {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (hInstance == NULL) {
		CHECK_ERR();
		return 0;
	}

	HWND hwnd = CreateWindowEx(
		0, L"STATIC", L" ", WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
		NULL, NULL, hInstance, NULL
	);

	ShowWindow(hwnd, 1);

	WriteMsg(hwnd, TEXT("No key"));

	if (hwnd == NULL)
	{
		CHECK_ERR();
		return 0;
	}

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

	ok = Read(serial, data, MAX_BYTES_READ);
	if (!ok) {
		printf("cannot read, exiting...");
		CloseHandle(serial);
		return 1;
	}
	printf("%s\n", data);

	ShowSecret(serial, hwnd, data);

	CloseWindow(hwnd);

	free(data);
	CloseHandle(serial);
}
