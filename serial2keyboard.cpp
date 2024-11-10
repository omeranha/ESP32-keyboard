#include <iostream>
#include <Windows.h>


void pressKey(uint8_t key, uint16_t delay) {

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = key;
    SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    if (delay) {
        Sleep(delay);
    }
}

int main() {
	HANDLE hSerial = CreateFile(L"COM5", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port" << std::endl;
        return 1;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting state" << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_115200;  // Set Baud rate to 9600
    dcbSerialParams.ByteSize = 8;         // Data bits = 8
    dcbSerialParams.StopBits = ONESTOPBIT;// One stop bit
    dcbSerialParams.Parity = NOPARITY;  // No parity

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting state" << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        std::cerr << "Error setting timeouts" << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    while (true) {
        // Reading from the serial port
        uint8_t buffer = 0x00;
        DWORD bytesRead;
        if (ReadFile(hSerial, &buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
            std::cout << "Data read: " << std::hex << buffer << std::endl;
			pressKey(buffer, 0);
        }
    }

    CloseHandle(hSerial);
    return 0;
}
