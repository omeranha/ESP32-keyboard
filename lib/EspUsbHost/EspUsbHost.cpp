#include "EspUsbHost.h"

void EspUsbHost::_printPcapText(const char *title, uint16_t function, uint8_t direction, uint8_t endpoint, uint8_t type, uint8_t size, uint8_t stage, const uint8_t *data) {
	uint8_t urbsize = 0x1c;
	if (stage == 0xff) {
		urbsize = 0x1b;
	}

	String data_str = "";
	for (int i = 0; i < size; i++) {
		if (data[i] < 16) {
			data_str += "0";
		}
		data_str += String(data[i], HEX) + " ";
	}

	printf("\n");
	printf("[PCAP TEXT]%s\n", title);
	printf("0000  %02x 00 00 00 00 00 00 00 00 00 00 00 00 00 %02x %02x\n", urbsize, (function & 0xff), ((function >> 8) & 0xff));
	printf("0010  %02x 01 00 01 00 %02x %02x %02x 00 00 00", direction, endpoint, type, size);
	if (stage != 0xff) {
		printf(" %02x\n", stage);
	} else {
		printf("\n");
	}
	printf("00%02x  %s\n", urbsize, data_str.c_str());
	printf("\n");
}

void EspUsbHost::begin(void) {
	usbTransferSize = 0;
	const usb_host_config_t config = {
		.skip_phy_setup = false,
		.intr_flags = ESP_INTR_FLAG_LEVEL1,
	};

	esp_err_t result = usb_host_install(&config);
	if (result != ESP_OK) {
		Serial.println("usb_host_install() error" + String(result));
		return;
	}

	const usb_host_client_config_t client_config = {
		.is_synchronous = true,
		.max_num_event_msg = 10,
		.async = {
			.client_event_callback = this->_clientEventCallback,
			.callback_arg = this,
		}
	};
	usb_host_client_register(&client_config, &this->clientHandle);
}

void EspUsbHost::_clientEventCallback(const usb_host_client_event_msg_t *eventMsg, void *arg) {
	EspUsbHost *usbHost = (EspUsbHost *)arg;
	esp_err_t result;
	switch (eventMsg->event) {
		case USB_HOST_CLIENT_EVENT_NEW_DEV: {
			Serial.println("USB_HOST_CLIENT_EVENT_NEW_DEV new_dev.address = " + String(eventMsg->new_dev.address));
			result = usb_host_device_open(usbHost->clientHandle, eventMsg->new_dev.address, &usbHost->deviceHandle);
			if (result != ESP_OK) {
				Serial.println("usb_host_device_open() error = " + String(result));
				return;
			}

			usb_device_info_t dev_info;
			result = usb_host_device_info(usbHost->deviceHandle, &dev_info);
			if (result != ESP_OK) {
				Serial.println("usb_host_device_info() error = " + String(result));
				return;
			}

			const usb_device_desc_t *dev_desc;
			result = usb_host_get_device_descriptor(usbHost->deviceHandle, &dev_desc);
			if (result != ESP_OK) {
				Serial.println("usb_host_get_device_descriptor() error = " + String(result));
				return;
			}

			const uint8_t requestdevice[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00 };
			_printPcapText("GET DESCRIPTOR Request DEVICE", 0x000b, 0x00, 0x80, 0x02, sizeof(requestdevice), 0x00, requestdevice);
			_printPcapText("GET DESCRIPTOR Response DEVICE", 0x0008, 0x01, 0x80, 0x02, sizeof(usb_device_desc_t), 0x03, (const uint8_t *)dev_desc);

			const usb_config_desc_t *config_desc;
			result = usb_host_get_active_config_descriptor(usbHost->deviceHandle, &config_desc);
			if (result != ESP_OK) {
				Serial.println("usb_host_get_active_config_descriptor() error = " + String(result));
				return;
			}

			const uint8_t requestconfig[8] = { 0x80, 0x06, 0x00, 0x02, 0x00, 0x00, 0x09, 0x00 };
			_printPcapText("GET DESCRIPTOR Request CONFIGURATION", 0x000b, 0x00, 0x80, 0x02, sizeof(requestconfig), 0x00, requestconfig);
			_printPcapText("GET DESCRIPTOR Response CONFIGURATION", 0x0008, 0x01, 0x80, 0x02, sizeof(usb_config_desc_t), 0x03, (const uint8_t *)config_desc);
			usbHost->_configCallback(config_desc);
			break;
		}
		case USB_HOST_CLIENT_EVENT_DEV_GONE: {
			Serial.println("USB_HOST_CLIENT_EVENT_DEV_GONE");
			for (uint8_t i = 0; i < usbHost->usbTransferSize; i++) {
				if (!usbHost->usbTransfer[i]) {
					continue;
				}

				usb_host_endpoint_clear(eventMsg->dev_gone.dev_hdl, usbHost->usbTransfer[i]->bEndpointAddress);
				usb_host_transfer_free(usbHost->usbTransfer[i]);
				usbHost->usbTransfer[i] = NULL;
			}

			usbHost->usbTransferSize = 0;
			for (uint8_t i = 0; i < usbHost->usbInterfaceSize; i++) {
				usb_host_interface_release(usbHost->clientHandle, usbHost->deviceHandle, usbHost->usbInterface[i]);
				usbHost->usbInterface[i] = 0;
			}

			usbHost->usbInterfaceSize = 0;
			usb_host_device_close(usbHost->clientHandle, usbHost->deviceHandle);
			usbHost->onGone(eventMsg);
			break;
		}
	}
}

void EspUsbHost::_configCallback(const usb_config_desc_t *config_desc) {
	const uint8_t *p = &config_desc->val[0];
	uint8_t bLength;
	const uint8_t setup[8] = { 0x80, 0x06, 0x00, 0x02, 0x00, 0x00, (uint8_t)config_desc->wTotalLength, 0x00 };
	_printPcapText("GET DESCRIPTOR Request CONFIGURATION", 0x000b, 0x00, 0x80, 0x02, sizeof(setup), 0x00, setup);
	_printPcapText("GET DESCRIPTOR Response CONFIGURATION", 0x0008, 0x01, 0x80, 0x02, config_desc->wTotalLength, 0x03, (const uint8_t *)config_desc);

	for (uint16_t i = 0; i < config_desc->wTotalLength; i += bLength, p += bLength) {
		bLength = *p;
		if ((i + bLength) >= config_desc->wTotalLength) {
			return;
		}

		const uint8_t bDescriptorType = *(p + 1);
		this->onConfig(bDescriptorType, p);
	}
}

void EspUsbHost::task(void) {
	esp_err_t result = usb_host_lib_handle_events(1, &this->eventFlags);
	if (result != ESP_OK) {
		Serial.println("usb_host_lib_handle_events() error" + String(result));
		return;
	}

	result = usb_host_client_handle_events(this->clientHandle, 1);
	if (result != ESP_OK) {
		Serial.println("usb_host_client_handle_events() error" + String(result));
		return;
	}

	if (!this->isReady) {
		return;
	}

	unsigned long now = millis();
	if ((now - this->lastCheck) > this->interval) {
		this->lastCheck = now;

		for (uint8_t i = 0; i < this->usbTransferSize; i++) {
			if (!this->usbTransfer[i]) {
				continue;
			}

			usb_host_transfer_submit(this->usbTransfer[i]);
		}
	}
}

String EspUsbHost::getUsbDescString(const usb_str_desc_t *str_desc) {
	String str = "";
	if (!str_desc) {
		return str;
	}

	for (int i = 0; i < str_desc->bLength / 2; i++) {
		if (str_desc->wData[i] > 0xFF) {
			continue;
		}
		str += char(str_desc->wData[i]);
	}
	return str;
}

void EspUsbHost::onConfig(const uint8_t bDescriptorType, const uint8_t *p) {
	switch (bDescriptorType) {
		case USB_INTERFACE_DESC: {
			const usb_intf_desc_t *intf = (const usb_intf_desc_t *)p;
			this->claim_err = usb_host_interface_claim(this->clientHandle, this->deviceHandle, intf->bInterfaceNumber, intf->bAlternateSetting);
			if (this->claim_err != ESP_OK) {
				Serial.println("usb_host_interface_claim() error" + String(claim_err));
				return;
			}

			this->usbInterface[this->usbInterfaceSize] = intf->bInterfaceNumber;
			this->usbInterfaceSize++;
			_bInterfaceClass = intf->bInterfaceClass;
			_bInterfaceSubClass = intf->bInterfaceSubClass;
			_bInterfaceProtocol = intf->bInterfaceProtocol;
			break;
		}
		case USB_ENDPOINT_DESC: {
			const usb_ep_desc_t *ep_desc = (const usb_ep_desc_t *)p;
			if (this->claim_err != ESP_OK) {
				Serial.println("usb_host_interface_claim() error" + String(claim_err));
				return;
			}

			this->endpoint_data_list[USB_EP_DESC_GET_EP_NUM(ep_desc)].bInterfaceClass = _bInterfaceClass;
			this->endpoint_data_list[USB_EP_DESC_GET_EP_NUM(ep_desc)].bInterfaceSubClass = _bInterfaceSubClass;
			this->endpoint_data_list[USB_EP_DESC_GET_EP_NUM(ep_desc)].bInterfaceProtocol = _bInterfaceProtocol;
			this->endpoint_data_list[USB_EP_DESC_GET_EP_NUM(ep_desc)].bCountryCode = _bCountryCode;

			if ((ep_desc->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) != USB_BM_ATTRIBUTES_XFER_INT) {
				Serial.println("err ep_desc->bmAttributes=" + String(ep_desc->bmAttributes));
				return;
			}

			if (ep_desc->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) {
				esp_err_t err = usb_host_transfer_alloc(ep_desc->wMaxPacketSize + 1, 0, &this->usbTransfer[this->usbTransferSize]);
				if (err != ESP_OK) {
					this->usbTransfer[this->usbTransferSize] = NULL;
					Serial.println("usb_host_transfer_alloc() error" + String(err));
					return;
				}

				this->usbTransfer[this->usbTransferSize]->device_handle = this->deviceHandle;
				this->usbTransfer[this->usbTransferSize]->bEndpointAddress = ep_desc->bEndpointAddress;
				this->usbTransfer[this->usbTransferSize]->callback = this->_onReceive;
				this->usbTransfer[this->usbTransferSize]->context = this;
				this->usbTransfer[this->usbTransferSize]->num_bytes = ep_desc->wMaxPacketSize;
				interval = ep_desc->bInterval;
				isReady = true;
				this->usbTransferSize++;
			}
			break;
		}
		case USB_HID_DESC: {
			const tusb_hid_descriptor_hid_t *hid_desc = (const tusb_hid_descriptor_hid_t *)p;
			_bCountryCode = hid_desc->bCountryCode;
			submitControl(0x81, 0x00, 0x22, 0x0000, 136);
			break;
		}
	}
}

void EspUsbHost::_onReceive(usb_transfer_t *transfer) {
	EspUsbHost *usbHost = static_cast<EspUsbHost *>(transfer->context);
	endpoint_data_t *endpoint_data = &usbHost->endpoint_data_list[(transfer->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_NUM_MASK)];
	if (!(endpoint_data->bInterfaceClass == USB_CLASS_HID) && !(endpoint_data->bInterfaceSubClass == HID_SUBCLASS_BOOT)) {
		return;
	}

	if (endpoint_data->bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD) {
		static hid_keyboard_report_t last_report = {};
		if (memcmp(&last_report, transfer->data_buffer, sizeof(last_report))) {
			hid_keyboard_report_t report = {};
			memcpy(&report, transfer->data_buffer, sizeof(report));
			usbHost->keyboardCallback(report, last_report);
			memcpy(&last_report, &report, sizeof(last_report));

			uint8_t keycode = report.keycode[0];
			usbHost->keyboard_leds = 0; // todo: test
			switch (report.keycode[0]) {
				case HID_KEY_NUM_LOCK:
					usbHost->keyboard_leds |= KEYBOARD_LED_NUMLOCK;
					break;
				case HID_KEY_CAPS_LOCK:
					usbHost->keyboard_leds |= KEYBOARD_LED_CAPSLOCK;
					break;
				case HID_KEY_SCROLL_LOCK:
					usbHost->keyboard_leds |= KEYBOARD_LED_SCROLLLOCK;
					break;
			}

			if (usbHost->keyboard_leds) {
				usbHost->sendKeyboardLeds();
			}
		}
	} else if (endpoint_data->bInterfaceProtocol == HID_ITF_PROTOCOL_MOUSE) {
		static uint8_t last_buttons = 0;
		hid_mouse_report_t report = {};
		report.buttons = transfer->data_buffer[1];
		report.x = static_cast<uint8_t>(transfer->data_buffer[2]);
		report.y = static_cast<uint8_t>(transfer->data_buffer[4]);
		report.wheel = static_cast<uint8_t>(transfer->data_buffer[6]);

		usbHost->mouseCallback(report, last_buttons);
	}

	usbHost->onReceive(transfer);
}

void EspUsbHost::setHIDLocal(hid_local_enum_t code) {
	hidLocal = code;
}

esp_err_t EspUsbHost::submitControl(const uint8_t bmRequestType, const uint8_t bDescriptorIndex, const uint8_t bDescriptorType, const uint16_t wInterfaceNumber, const uint16_t wDescriptorLength) {
	usb_transfer_t *transfer;
	usb_host_transfer_alloc(wDescriptorLength + 9, 0, &transfer);
	transfer->num_bytes = wDescriptorLength + 8;
	transfer->data_buffer[0] = bmRequestType;
	transfer->data_buffer[1] = 0x06;
	transfer->data_buffer[2] = bDescriptorIndex;
	transfer->data_buffer[3] = bDescriptorType;
	transfer->data_buffer[4] = wInterfaceNumber & 0xff;
	transfer->data_buffer[5] = wInterfaceNumber >> 8;
	transfer->data_buffer[6] = wDescriptorLength & 0xff;
	transfer->data_buffer[7] = wDescriptorLength >> 8;
	transfer->device_handle = deviceHandle;
	transfer->bEndpointAddress = 0x00;
	transfer->callback = _onReceiveControl;
	transfer->context = this;
	if (bmRequestType == 0x81 && bDescriptorIndex == 0x00 && bDescriptorType == 0x22) {
		_printPcapText("GET DESCRIPTOR Request HID Report", 0x0028, 0x00, 0x80, 0x02, 8, 0, transfer->data_buffer);
	}

	esp_err_t result = usb_host_transfer_submit_control(clientHandle, transfer);
	return result;
}

void EspUsbHost::_onReceiveControl(usb_transfer_t *transfer) {
	EspUsbHost *usbHost = (EspUsbHost *)transfer->context;
	_printPcapText("GET DESCRIPTOR Response HID Report", 0x0008, 0x01, 0x80, 0x02, transfer->actual_num_bytes - 8, 0x03, &transfer->data_buffer[8]);
	usb_host_transfer_free(transfer);
}

void EspUsbHost::setKeyboardCallback(keyboard_callback callback) {
	this->keyboardCallback = callback;
}

void EspUsbHost::setMouseCallback(mouse_callback callback) {
	this->mouseCallback = callback;
}

void EspUsbHost::sendKeyboardLeds() {
	usb_transfer_t *transfer;
	esp_err_t transferResult = usb_host_transfer_alloc(9, 0, &transfer);
	if (transferResult != ESP_OK) {
		Serial.println("usb_host_transfer_alloc() error " + String(transferResult));
		return;
	}

	transfer->num_bytes = 9;
	transfer->data_buffer[0] = 0x21; // bmRequestType
	transfer->data_buffer[1] = 0x09; // bRequest (SetReport)
	transfer->data_buffer[2] = 0x00; // wValue (Report ID)
	transfer->data_buffer[3] = 0x02; // wValue (Report Type)
	transfer->data_buffer[4] = 0x00; // wIndex (Interface number, low byte)
	transfer->data_buffer[5] = 0x00; // wIndex (Interface number, high byte)
	transfer->data_buffer[6] = 0x01; // wLength (Data length, low byte)
	transfer->data_buffer[7] = 0x00; // wLength (Data length, high byte)
	transfer->data_buffer[8] = keyboard_leds; // Data stage (LED state bitfield)
	transfer->device_handle = deviceHandle;
	transfer->bEndpointAddress = 0x00; // Control endpoint
	transfer->callback = _onReceiveControl;
	transfer->context = this;
	usb_host_transfer_submit_control(clientHandle, transfer);
	//keyboard_leds = 0; todo: test
}
