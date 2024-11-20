#include "ESP8266WiFi.h"
#include "espnow.h"

struct keypress {
	uint8_t modifier;
	uint8_t reserved;
	uint8_t keycode[6];
};

void onReceiveData(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
	char str[15];
	uint8_t pos = 0;
	for (uint8_t i = 0; i < len; i++) {
		pos += sprintf(&str[pos], "%d", data[i]);
		if (i < len - 1) {
			pos += sprintf(&str[pos], ",");
		}
	}
	Serial.println(str);
}

void setup() {
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	Serial.println(WiFi.macAddress());
	if (esp_now_init() != 0) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}

	esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
	esp_now_register_recv_cb(onReceiveData);
}

void loop() {
}