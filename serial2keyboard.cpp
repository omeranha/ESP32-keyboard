#include "util.h"
#include <vector>
#include <thread>

std::vector<uint8_t> keys;
uint8_t lastKeys[6] = { 0, 0, 0, 0, 0, 0 };
uint8_t modifier;
uint8_t lastModifier;

static void pressKey(uint8_t* keys, uint8_t modifier) {
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;

	// Handle modifier key press
	if (modifier && modifier != lastModifier) {
		input.ki.wVk = modifiers[modifier];
		SendInput(1, &input, sizeof(INPUT));
	}

	// Handle key presses and releases
	for (size_t i = 0; i < 6; i++) {
		if (keys[i] != 0) {
			input.ki.wVk = keycodes[keys[i]];
			input.ki.dwFlags = 0; // Key pressssssssssssssss (down)
			SendInput(1, &input, sizeof(INPUT));
		}
	}

	// Handle key releases
	for (size_t i = 0; i < 6; i++) {
		if (lastKeys[i] != 0 && std::find(keys, keys + 6, lastKeys[i]) == keys + 6) {
			input.ki.wVk = keycodes[lastKeys[i]];
			input.ki.dwFlags = KEYEVENTF_KEYUP; // Key release
			SendInput(1, &input, sizeof(INPUT));
		}
	}

	// Handle modifier key release
	if (lastModifier && lastModifier != modifier) {
		input.ki.wVk = modifiers[lastModifier];
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}

	memcpy(lastKeys, keys, 6);
	lastModifier = modifier; // Update the last modifier state
}

void pressThread() {
	while (true) {
		pressKey(keys.data(), modifier);
		Sleep(100);
	}
}

int main() {
	keys.reserve(6);
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
	std::thread(pressThread).detach();
	while (true) {
		uint8_t buffer = 0;
		DWORD bytesRead;
		if (ReadFile(hSerial, &buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
			serial.push_back(buffer);
			if (buffer == '\n') {
				auto comma = serial.find(',');
				if (comma != std::string::npos) {
					size_t start = 0;
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
			}
		}
	}
	CloseHandle(hSerial);
	return 0;
}
