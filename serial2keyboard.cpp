#include "util.h"

static void pressKey(uint8_t key, uint8_t modifier) {
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	if (modifier) {
		input.ki.wVk = modifiers[modifier];
		SendInput(1, &input, sizeof(INPUT));
	}
	input.ki.wVk = key;
	SendInput(1, &input, sizeof(INPUT));
	input.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
	if (modifier) {
		input.ki.wVk = modifiers[modifier];
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}
}

int main() {
	HANDLE hSerial = CreateFile(PORT, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
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

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
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

	std::string serial;
	while (true) {
		uint8_t buffer = 0;
		if (ReadFile(hSerial, &buffer, sizeof(buffer), nullptr, nullptr) && buffer > 0) {
			serial.push_back(buffer);
			if (buffer == '\n') {
				auto comma = serial.find(',');
				if (comma != std::string::npos) {
					uint8_t key = static_cast<uint8_t>(std::stoi(serial.substr(0, comma)));
					uint8_t modifier = static_cast<uint8_t>(std::stoi(serial.substr(comma + 1)));
					pressKey(key,  modifier);
				}
				serial.clear();
			}
		}
	}

	CloseHandle(hSerial);
	return 0;
}
