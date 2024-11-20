#include "util.h"

void pressKey();
void autoRepeatThread();

#define PORT L"COM4"

uint8_t keys[6] = { 0 };
uint8_t modifier = 0;
uint8_t lastmodifier = 0;
bool keypressed = false;
bool autoRepeat = true;

int main() {
	HANDLE hSerial = CreateFile(PORT, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);        
	if (hSerial == INVALID_HANDLE_VALUE) {
		std::cerr << "Error opening serial port" << std::endl;
		return 1;
	}

	DCB dcbSerialParams = { 0 };
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	GetCommState(hSerial, &dcbSerialParams);
	dcbSerialParams.BaudRate = CBR_115200; 
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	SetCommState(hSerial, &dcbSerialParams);

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	SetCommTimeouts(hSerial, &timeouts);

	std::string serial;
	while (true) {
		uint8_t buffer = 0;
		if (ReadFile(hSerial, &buffer, sizeof(buffer), nullptr, nullptr) && buffer != 0) {
			serial += buffer;
			if (buffer == '\n') {
				std::cout << "Read string: " << serial;
				std::stringstream ss(serial);
				std::string token;
				uint8_t i = 0;
				while (std::getline(ss, token, ',')) {
					try {
						uint8_t value = std::stoi(token);
						if (i == 0) {
							modifier = value;
						} else if (i > 1 && i < 8) {
							keys[i - 2] = value;
						}
						i++;
					} catch (const std::invalid_argument& e) {
						serial.clear();
						break;
					}
				}
				keypressed = true;
				pressKey();
				serial.clear();
			}
		}
	}

	CloseHandle(hSerial);
	return 0;
}

void pressKey() {
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	if (modifier && modifier != lastmodifier) {
		input.ki.wVk = modifiercodes[modifier];
		SendInput(1, &input, sizeof(INPUT));
	}

	for (uint8_t key : keys) {
		if (key != 0) {
			input.ki.wVk = keycodes[key];
			SendInput(1, &input, sizeof(INPUT));
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));
		}
	}

	if (lastmodifier && lastmodifier != modifier) {
		input.ki.wVk = modifiercodes[lastmodifier];
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}

	lastmodifier = modifier;
}

void autoRepeatThread() {
	while (autoRepeat) {
		if (keypressed) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500)); // auto-repeat delay
			while (keypressed && autoRepeat) {
				pressKey();
				std::this_thread::sleep_for(std::chrono::milliseconds(100)); // auto-repeat rate
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
