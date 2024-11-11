#include "util.h"

void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report) {
	keypress press;
	press.keycode = keycodes[report.keycode[0]];
	press.modifier = report.modifier;
	esp_now_send(receiverAddr, (uint8_t*)&press, sizeof(press));
	Serial.print(press.keycode);
	Serial.print(" ");
	Serial.println(press.modifier);
}

EspUsbHost usbhost;

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

	usbhost.begin();
	usbhost.setHIDLocal(HID_LOCAL_Portuguese);
	usbhost.setKeyboardCallback(onKeyboard);
}

void loop() {
	usbhost.task();
}
