#ifndef __EspUsbHost_H__
#define __EspUsbHost_H__

#include <Arduino.h>
#include <usb/usb_host.h>
#include <class/hid/hid.h>
#include <rom/usb/usb_common.h>

typedef void (*keyboard_callback)(hid_keyboard_report_t report, hid_keyboard_report_t last_report);
typedef void (*mouse_callback)(hid_mouse_report_t report, uint8_t last_buttons);

class EspUsbHost {
public:
	bool isReady = false;
	uint8_t interval;
	unsigned long lastCheck;
	uint8_t keyboard_leds = 0;
	
	struct endpoint_data_t {
		uint8_t bInterfaceClass;
		uint8_t bInterfaceSubClass;
		uint8_t bInterfaceProtocol;
		uint8_t bCountryCode;
	};

	endpoint_data_t endpoint_data_list[17];
	uint8_t _bInterfaceClass;
	uint8_t _bInterfaceSubClass;
	uint8_t _bInterfaceProtocol;
	uint8_t _bCountryCode;
	esp_err_t claim_err;

	usb_host_client_handle_t clientHandle;
	usb_device_handle_t deviceHandle;
	uint32_t eventFlags;
	usb_transfer_t *usbTransfer[16];
	uint8_t usbTransferSize;
	uint8_t usbInterface[16];
	uint8_t usbInterfaceSize;

	hid_local_enum_t hidLocal;

	keyboard_callback keyboardCallback;
	mouse_callback mouseCallback;

	void begin(void);
	void task(void);

	static void _clientEventCallback(const usb_host_client_event_msg_t *eventMsg, void *arg);
	void _configCallback(const usb_config_desc_t *config_desc);
	void onConfig(const uint8_t bDescriptorType, const uint8_t *p);
	static String getUsbDescString(const usb_str_desc_t *str_desc);
	static void _onReceive(usb_transfer_t *transfer);

	esp_err_t submitControl(const uint8_t bmRequestType, const uint8_t bDescriptorIndex, const uint8_t bDescriptorType, const uint16_t wInterfaceNumber, const uint16_t wDescriptorLength);
	static void _onReceiveControl(usb_transfer_t *transfer);

	virtual void onReceive(const usb_transfer_t *transfer){};
	virtual void onGone(const usb_host_client_event_msg_t *eventMsg){};

	void setHIDLocal(hid_local_enum_t code);

	void setKeyboardCallback(keyboard_callback callback);
	void setMouseCallback(mouse_callback callback);

	void sendKeyboardLeds(uint8_t led);
};

#endif
