#define CFG_USB
#include "usb.h"
#include "usb-c.h"
#include "conf.h"
#include "usb-pck.h"
#include "assert-c.h"
#include "cmd.h"
#include <string.h>

static void setup_send(struct pck_setup *r, void *pck, size_t s)
{
	usb_send(USB_EP_CONTROL, pck, s < r->wLength ? s : r->wLength);
}

static void req_get_descriptor(struct pck_setup *r)
{
	enum str_indx {
		STR_LANGID,
		STR_MANUF,
		STR_PRODUCT,
		STR_SERIAL
	};
	struct pck_device_descriptor desc = {
		.bLength = sizeof(struct pck_device_descriptor),
		.bDescriptorType = PCK_DESCRIPTOR_DEVICE,
		.bcdUSB = 0x00,
		.bDeviceClass = 0xFF,
		.bDeviceSubClass = 0xFF,
		.bDeviceProtocol = 0xFF,
		.bMaxPacketSize = usb_get_max_packet_size(USB_EP_CONTROL),
		.idVendor = 0xDEAF,
		.idProduct = 0xCAFE,
		.bcdDevice = 0xFACE,
		.iManufacturer = STR_MANUF,
		.iProduct = STR_PRODUCT,
		.iSerialNumber = STR_SERIAL,
		.bNumConfigurations = 1,
	};

	struct pck_endpoint_descriptor ep_htd = {
		.bLength = sizeof(ep_htd),
		.bDescriptorType = PCK_DESCRIPTOR_ENDPOINT,
		.bEndpointAddress = {
			.endpoint_number = 1,
			.direction = PCK_DIRECTION_HOST_TO_DEV,
		},
		.bmAttributes = {
			.transfer_type = PCK_TRANSFER_TYPE_BULK,
		},
		.wMaxPacketSize = cfg_usb_ep[1].rx_size,
		//.bInterval = 0 // Ignored
	};

	struct pck_endpoint_descriptor ep_dth = {
		.bLength = sizeof(ep_dth),
		.bDescriptorType = PCK_DESCRIPTOR_ENDPOINT,
		.bEndpointAddress = {
			.endpoint_number = 2,
			.direction = PCK_DIRECTION_DEV_TO_HOST,
		},
		.bmAttributes = {
			.transfer_type = PCK_TRANSFER_TYPE_BULK,
		},
		.wMaxPacketSize = cfg_usb_ep[2].tx_size,
		//.bInterval = 0 // Ignored
	};

	struct {
		struct pck_configuration_descriptor cfg;
		struct pck_interface_descriptor face;
		struct pck_endpoint_descriptor ep_htd;
		struct pck_endpoint_descriptor ep_dth;
	} __attribute__ ((__packed__)) configdesc = {
		.cfg = {
			.bLength = sizeof(struct pck_configuration_descriptor),
			.bDescriptorType = PCK_DESCRIPTOR_CONFIGURATION,
			.wTotalLength = sizeof(configdesc),
			.bNumInterfaces = 1,
			.bConfigurationValue = 1,
			.iConfiguration = 0,
			.bmAttributes = {
				.self_powered = 1,
				.reserved_set_to_1 = 1,
			},
			.bMaxPower = 100 / 2,
		},
		.face = {
			.bLength = sizeof(struct pck_interface_descriptor),
			.bDescriptorType = PCK_DESCRIPTOR_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = 0,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		.ep_htd = ep_htd,
		.ep_dth = ep_dth,
	};
	_Static_assert(sizeof(configdesc) == 18 + sizeof(ep_htd)*2, "configdesc mismatch");

	struct {
		struct pck_string_descriptor d;
		short arr[2];
	} langid = {
		{ sizeof(langid), PCK_DESCRIPTOR_STRING },
		{ 0x09, 0x04 }
	};

	struct {
		struct pck_string_descriptor d;
		uint16_t arr[23];
	} vendor = {
		{ sizeof(vendor), PCK_DESCRIPTOR_STRING },
		u"Omniscient Vendor Group"
	};

	struct {
		struct pck_string_descriptor d;
		uint16_t arr[21];
	} product = {
		{ sizeof(product), PCK_DESCRIPTOR_STRING },
		u"Uneven Hissing Device"
	};

	struct {
		struct pck_string_descriptor d;
		uint16_t arr[14];
	} serial = {
		{ sizeof(serial), PCK_DESCRIPTOR_STRING },
		u"Vague Quotient"
	};

	switch (r->wValue >> 8) {
	case PCK_DESCRIPTOR_DEVICE:
		setup_send(r, &desc, sizeof(desc));
		break;
	case PCK_DESCRIPTOR_CONFIGURATION:
		setup_send(r, &configdesc, sizeof(configdesc));
		break;
	case PCK_DESCRIPTOR_STRING:
		switch (r->wValue & 0xff) {
		case STR_LANGID:
			setup_send(r, &langid, sizeof(langid));
			break;
		case STR_MANUF:
			setup_send(r, &vendor, sizeof(vendor));
			break;
		case STR_PRODUCT:
			setup_send(r, &product, sizeof(product));
			break;
		case STR_SERIAL:
			setup_send(r, &serial, sizeof(serial));
			break;
		}
		break;
	case PCK_DESCRIPTOR_ENDPOINT:
		switch (r->wValue & 0xff) {
		case 1:
			setup_send(r, &ep_htd, sizeof(ep_htd));
			break;
		case 2:
			setup_send(r, &ep_dth, sizeof(ep_dth));
			break;
		};
		break;
	default:
		usb_send(USB_EP_CONTROL, NULL, 0);
	}
}

void handle_setup_requests(struct pck_setup *r)
{
	switch (r->bRequest) {
	case PCK_REQUEST_GET_DESCRIPTOR:
		req_get_descriptor(r);
		break;
	case PCK_REQUEST_SET_ADDRESS:
		usb_set_address(r->wValue);
		usb_respond(USB_EP_CONTROL, USB_RESPONSE_ACK);
		break;
	case PCK_REQUEST_SET_CONFIGURATION:
		usb_respond(USB_EP_CONTROL, USB_RESPONSE_ACK);
		break;
	default:
		usb_respond(USB_EP_CONTROL, USB_RESPONSE_STALL);
	}
}

void usb_htd_handle(enum usb_endp ep, void *dat, int len)
{
	assert(ep == USB_EP1);
	cmd_received_cpy(CMD_DEVICE_USB, dat, len);
}

void usb_reply(char *n)
{
	int l = strlen(n);
	usb_send(USB_EP2, n, l);
}


