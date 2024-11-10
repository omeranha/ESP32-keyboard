#include <WiFi.h>
#include <esp_now.h>
#include "EspUsbHost.h"

uint8_t receiverAddr[] = { 0x68, 0xC6, 0x3A, 0x98, 0x56, 0xE1 };

class MyEspUsbHost : public EspUsbHost {
	void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
		Serial.println(keycode);
		esp_now_send(receiverAddr, &keycode, sizeof(ascii));
	}
};

MyEspUsbHost usbHost;

void setup() {
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	Serial.println(WiFi.macAddress());
	if (esp_now_init() != ESP_OK) {
		Serial.println("esp_now_init error");
		return;
	}

	esp_now_peer_info_t peerInfo;
	memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
	memcpy(peerInfo.peer_addr, receiverAddr, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	esp_err_t addPeerResult = esp_now_add_peer(&peerInfo);
	if (addPeerResult != ESP_OK) {
		Serial.print("esp_now_add_peer error: ");
		Serial.println(addPeerResult);
		return;
	}

	usbHost.begin();
	usbHost.setHIDLocal(HID_LOCAL_US);
}

void loop() {
	usbHost.task();
	delay(10);
}