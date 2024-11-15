#include "util.h"


void pressKey(uint8_t* keys, uint8_t modifier);
void pressThread();
void autoRepeatThread();

#define PORT L"COM4"
bool keyPressed = false;
bool autoRepeatActive = false;
std::vector<uint8_t> keys(6);
uint8_t lastKeys[6] = { 0 };
uint8_t modifier = 0;
uint8_t lastModifier = 0;

int main() {
	HANDLE hSerial = CreateFile(PORT, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hSerial == INVALID_HANDLE_VALUE) {
		std::cout << "Error opening serial port\n";
		return 1;
	}

	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hSerial, &dcbSerialParams)) {
		std::cout << "Error getting state\n";
		CloseHandle(hSerial);
		return 1;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (!SetCommState(hSerial, &dcbSerialParams)) {
		std::cout << "Error setting state\n";
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
		std::cout << "Error setting timeouts\n";
		CloseHandle(hSerial);
		return 1;
	}

	std::string serial;
	std::thread(pressThread).detach();
	std::thread(autoRepeatThread).detach();
	while (true) {
		uint8_t buffer = 0;
		DWORD bytesRead;
		if (ReadFile(hSerial, &buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
			serial.push_back(buffer);
			if (buffer == '\n') {
				auto comma = serial.find(',');
				auto start = 0;
				if (comma != std::string::npos) {
					while (comma != std::string::npos) {
						keys.push_back(static_cast<uint8_t>(std::stoi(serial.substr(start, comma - start))));
						start = comma + 1;
						comma = serial.find(',', start);
					}
					keys.push_back(static_cast<uint8_t>(std::stoi(serial.substr(start))));
					modifier = keys.back();
					keys.pop_back();
				}
				serial.clear();
				keys.clear();
				keyPressed = true;
			} else {
				keyPressed = false;
			}
		}
	}
	CloseHandle(hSerial);
	return 0;
}

void pressKey(uint8_t* keys, uint8_t modifier) {
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	if (modifier && modifier != lastModifier) {
		input.ki.wVk = modifiers[modifier];
		SendInput(1, &input, sizeof(INPUT));
	}


	for (uint8_t i = 0; i < 6; i++) {
		if (keys[i] != 0) {
			input.ki.wVk = keycodes[keys[i]];
			input.ki.dwFlags = 0;
			SendInput(1, &input, sizeof(INPUT));
		}
	}

	for (uint8_t i = 0; i < 6; i++) {
		if (lastKeys[i] != 0 && std::find(keys, keys + 6, lastKeys[i]) == keys + 6) {
			input.ki.wVk = keycodes[lastKeys[i]];
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
		}
	}

	if (lastModifier && lastModifier != modifier) {
		input.ki.wVk = modifiers[lastModifier];
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}

	memcpy(lastKeys, keys, 6);
	lastModifier = modifier;
}

void autoRepeatThread() {
	while (true) {
		if (keyPressed) {
			Sleep(500); // auto-repeat delay
			autoRepeatActive = true;
			while (keyPressed) {
				pressKey(keys.data(), modifier);
				Sleep(100); // auto-repeat speed
			}
			autoRepeatActive = false;
		}
		Sleep(1);
	}
}

void pressThread() {
	while (true) {
		if (!autoRepeatActive) {
			pressKey(keys.data(), modifier);
		}
		Sleep(1);
	}
}
