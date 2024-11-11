#include "ESP8266WiFi.h"
#include "espnow.h"

struct keypress {
	uint8_t key;
	uint8_t modifier;
};

void onReceiveData(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
	keypress *press = (keypress*)data;
	Serial.print(press->key);
	Serial.print(",");
	Serial.println(press->modifier);
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