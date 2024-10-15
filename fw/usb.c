#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include "usb.h"
#include "uart.h"

#define USB_VERSION 0x0200

#define ENDPOINT_DIRECTION_IN				0x01
#define ENDPOINT_DIRECTION_OUT				0x00
#define ENDPOINT_DIRECTION_SHIFT			0
#define ENDPOINT_DIRECTION_MASK				0x01
#define ENDPOINT_TYPE_CONTROL				0x00
#define ENDPOINT_TYPE_ISOCHRONOUS			0x01
#define ENDPOINT_TYPE_BULK					0x10
#define ENDPOINT_TYPE_INTERRUPT				0x11
#define ENDPOINT_TYPE_SHIFT					6
#define ENDPOINT_TYPE_MASK					0xC0
#define ENDPOINT_CONFIG_1(ENDPOINT_TYPE, ENDPOINT_DIRECTION)	(ENDPOINT_TYPE_MASK & (ENDPOINT_TYPE << ENDPOINT_TYPE_SHIFT)) | \
																(ENDPOINT_DIRECTION_MASK & (ENDPOINT_DIRECTION << ENDPOINT_DIRECTION_SHIFT))

#define ENDPOINT_ALLOCATION_CLEAR			0x00
#define ENDPOINT_ALLOCATION_SET				0x01
#define ENDPOINT_ALLOCATION_SHIFT			1
#define ENDPOINT_ALLOCATION_MASK			0x02
#define ENDPOINT_BANK_SINGLE				0x00
#define ENDPOINT_BANK_DOUBLE				0x01
#define ENDPOINT_BANK_SHIFT					2
#define ENDPOINT_BANK_MASK					0x0C
#define ENDPOINT_SIZE_8						0x00
#define ENDPOINT_SIZE_16					0x01
#define ENDPOINT_SIZE_32					0x02
#define ENDPOINT_SIZE_64					0x03
#define ENDPOINT_SIZE_128					0x04
#define ENDPOINT_SIZE_256					0x05
#define ENDPOINT_SIZE_512					0x06
#define ENDPOINT_SIZE_SHIFT					4
#define ENDPOINT_SIZE_MASK					0x70
#define ENDPOINT_CONFIG_2(ENDPOINT_SIZE, ENDPOINT_BANK, ENDPOINT_ALLOCATION)	(ENDPOINT_SIZE_MASK & (ENDPOINT_SIZE << ENDPOINT_SIZE_SHIFT)) | \
																				(ENDPOINT_BANK_MASK & (ENDPOINT_BANK_SINGLE << ENDPOINT_BANK_SHIFT)) | \
																				(ENDPOINT_ALLOCATION_MASK & (ENDPOINT_ALLOCATION_SET << ENDPOINT_ALLOCATION_SHIFT))

#define ENDPOINT_0_CONTROL_TRANSFER		0x00
#define ENDPOINT_1
#define ENDPOINT_2
#define ENDPOINT_3_KEYBOARD				0x03

#define ENDPOINT0_SIZE       			32 // Size has to be equal to what was programmed at the endpoint configuration register 

#define DESCRIPTOR_TYPE_DEVICE  		0x01
#define DESCRIPTOR_TYPE_CONFIGURATION  	0x02
#define DESCRIPTOR_TYPE_INTERFACE  		0x04

#define USB_CLASS_NONE      			0x00
#define USB_SUBCLASS_NONE   			0x00
#define USB_PROTOCOL_NONE   			0x00
#define USB_CONFIG_SELF_POWERED		 	0xC0 // Bitmap configuration, See in USB 2.0 Specification Table 9-10
#define USB_CONFIG_CURRENT_100mA		50 // Expressed in 2 mA units
#define USB_DEVICE_CLASS_CODE_HID 		0x03 // See Defined Class Codes
#define USB_DEVICE_SUBCLASS_BOOT   		0x01 // See Device Class Definition for Human Interface Devices (HID) Section 4.2 Subclass
#define USB_DEVICE_PROTOCOL_KEYBOARD   	0x01 // See Device Class Definition for Human Interface Devices (HID) Section 4.2 Subclass

#define VENDOR_ID   		0x03EB // Atmel Corporation
#define PRODUCT_ID  		0x2FF4 // Repurposing Product ID that corresponds to the 'atmega32u4 DFU bootloader'
#define DEVICE_VERSION 		0x0100

#define GET_STATUS 			0x00
#define CLEAR_FEATURE 		0x01
#define SET_FEATURE 		0x03
#define SET_ADDRESS 		0x05
#define GET_DESCRIPTOR 		0x06
#define GET_CONFIGURATION 	0x08
#define SET_CONFIGURATION 	0x09
#define GET_INTERFACE 		0x0A
#define SET_INTERFACE 		0x0B

volatile uint8_t usbConfigurationValue = 0; // When non-zero device is is configured and respective stored value holds selected configuration

// See USB 2.0 Specification Table 9-8
typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations; 
} __attribute__((packed)) deviceDescriptor_t;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __attribute__((packed)) configurationDescriptor_t;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __attribute__((packed)) interfaceDescriptor_t;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;	
    uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} __attribute__((packed)) endpointDescriptor_t;

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType1;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDescriptors;
	uint8_t bDescriptorType2;
	uint16_t wDescriptorLength;
} __attribute__((packed)) hidDescriptor_t;

typedef struct {
    deviceDescriptor_t deviceDescriptor;
    configurationDescriptor_t configurationDescriptor;
    interfaceDescriptor_t interfaceDescriptor;
    endpointDescriptor_t endpointDescriptor;
	hidDescriptor_t hidDescriptor;
} usbDescriptors_t;

const usbDescriptors_t usbDescriptors PROGMEM = {
	.deviceDescriptor = {
        .bLength = sizeof(deviceDescriptor_t), // 18 bytes
        .bDescriptorType = DESCRIPTOR_TYPE_DEVICE,
        .bcdUSB = USB_VERSION,
        .bDeviceClass = USB_CLASS_NONE, // Set to none to indicate that the HID interface
        .bDeviceSubClass = USB_SUBCLASS_NONE,
        .bDeviceProtocol = USB_PROTOCOL_NONE,
        .bMaxPacketSize0 = ENDPOINT0_SIZE, // Must match endpoint size. TODO: Confirm this note
        .idVendor = VENDOR_ID,
        .idProduct = PRODUCT_ID,
        .bcdDevice = DEVICE_VERSION,
        .iManufacturer = 0x00, // TBD
        .iProduct = 0x00, // TBD
        .iSerialNumber = 0x00, // TBD
        .bNumConfigurations	= 1
    },
    .configurationDescriptor = {
		.bLength = sizeof(configurationDescriptor_t),
		.bDescriptorType = DESCRIPTOR_TYPE_CONFIGURATION,
		.wTotalLength = (sizeof(configurationDescriptor_t) + sizeof(interfaceDescriptor_t) + sizeof(hidDescriptor_t) + sizeof(endpointDescriptor_t)), // TODO: Need to update this once HID descriptor is added
		.bNumInterfaces = 0x01,
		.bConfigurationValue = 0x01,
		.iConfiguration = 0x00,
		.bmAttributes = USB_CONFIG_SELF_POWERED,
		.bMaxPower = USB_CONFIG_CURRENT_100mA,
	},
	.interfaceDescriptor = {
		.bLength = sizeof(interfaceDescriptor_t),
		.bDescriptorType = DESCRIPTOR_TYPE_INTERFACE,
		.bInterfaceNumber = 0x00,
		.bAlternateSetting = 0x00,
		.bNumEndpoints = 0x01,
		.bInterfaceClass = USB_DEVICE_CLASS_CODE_HID,
		.bInterfaceSubClass = USB_DEVICE_SUBCLASS_BOOT,
		.bInterfaceProtocol = USB_DEVICE_PROTOCOL_KEYBOARD,
		.iInterface = 0x00,
	},
	.endpointDescriptor = {
		.bLength = sizeof(endpointDescriptor_t),
		.bDescriptorType = 0x5,	
    	.bEndpointAddress = 0x81,
		.bmAttributes = 0x03,
		.wMaxPacketSize = 0x0008,
		.bInterval = 0x0A,
	},
	.hidDescriptor = {
		.bLength = sizeof(hidDescriptor_t),
		.bDescriptorType1 = 0x21, // HID_DESCRIPTOR_TYPE
		.bcdHID = 0x0101, //HID Class Specification release number
		.bCountryCode = 0x00,
		.bNumDescriptors = 0x01,
		.bDescriptorType2 = 0x22, // Report Descriptor Type
		.wDescriptorLength = 0x003F,
	}
};

static const uint8_t hidReportDescriptor[] PROGMEM = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection (Application)
    0x05, 0x07,  // Usage Page (Key Codes)
    0x19, 0xE0,  // Usage Minimum (224)
    0x29, 0xE7,  // Usage Maximum (231)
    0x15, 0x00,  // Logical Minimum (0)
    0x25, 0x01,  // Logical Maximum (1)
    0x75, 0x01,  // Report Size (1)
    0x95, 0x08,  // Report Count (8)
    0x81, 0x02,  // Input (Data, Variables, Absolute)
    0x95, 0x01,  // Report Count (1)
    0x75, 0x08,  // Report Size (8)
    0x81, 0x01,  // Input (Constant)
    0x95, 0x05,  // Report Count (5)
    0x75, 0x01,  // Report Size (1)
    0x05, 0x08,  // Usage Page (Page# for LEDs)
    0x19, 0x01,  // Usage Minimum (1)
    0x29, 0x05,  // Usage Maximum (5)
    0x91, 0x02,  // Output (Data, Variables, Absolute)
    0x95, 0x01,  // Report Count (1)
    0x75, 0x03,  // Report Size (3)
    0x91, 0x01,  // Output (Constant)
	0x95, 0x06,  // Report Count (6)
    0x75, 0x08,  // Report Size (8)
    0x15, 0x00,  // Logical Minimum (0)
    0x25, 0x65,  // Logical Maximum (101)
    0x05, 0x07,  // Usage Page (Key Codes)
    0x19, 0x00,  // Usage Minimum (0)
    0x29, 0x65,  // Usage Maximum (101)
    0x81, 0x00,  // Input (Data, Array)
    0xC0   		 // End collection
};

volatile uint8_t usbAddressConfig = 0;

void usb_init() {
    UHWCON = (1<<UVREGE); // Power on the USB pads regulator
    USBCON = (1<<FRZCLK); // Freeze the clock prior configuration
    PLLCSR = (1<<PINDIV) | (1 << PLLE); // Set PLL input prescaler 1:2  and enable PLL and start calibration
    while (!(PLLCSR & (1 << PLOCK))) { 
        // Wait until PLL is locked to the reference clock
    }
    USBCON |= ((1 << USBE) | (1 << OTGPADE)); // Enable the USB controller and enable the VBUS pad
    USBCON &= ~(1 << FRZCLK); // Unfreeze the USB controller clock
    UDCON = 0; // Set full speed mode and attach USB device
    UDIEN = (1 << EORSTE) | (1 << SOFE); // Enable the USB interrupt flags for end of reset and start of frame
	usbConfigurationValue = 0; // Device is unconfigured
	sei();

	usbAddressConfig |= (1 << 0);
}

// USB General Interrupt Service Routine
ISR(USB_GEN_vect) {
	uint8_t udint_bits = UDINT;
	UDINT = 0; // Clear interrupt flag register

    // Check if end of reset interrupt flag as occured to begin configuration of control transfer endpoint
    if (udint_bits & (1 << EORSTI)) {
        UENUM = ENDPOINT_0_CONTROL_TRANSFER; // Select the endpoint
        UECONX = (1 << EPEN); // Activate the endpoint
        UECFG0X = ENDPOINT_CONFIG_1(ENDPOINT_TYPE_CONTROL, ENDPOINT_DIRECTION_OUT); // Configure the endpoint type and direction
		UECFG1X = ENDPOINT_CONFIG_2(ENDPOINT_SIZE_32, ENDPOINT_BANK_SINGLE, ENDPOINT_ALLOCATION_SET); // Configure endpoint size and bank parametrization // TODO should use ENDPOINT0_SIZE macro instead
        UEIENX = (1 << RXSTPE);  // Enable the "received setup packet" interrupt flag
        usbConfigurationValue = 0; // Device is unconfigured TODO why do I need to have this here?
		usbAddressConfig |= (1 << 1);
    }

    // Check if start of frame interrupt flag as occured to know when USB "Start Of Frame" PID (SOF) has been detected
    // Note here is where we would report keyword presses to the USB host   
	if (udint_bits & (1 << SOFI)) {
		if (usbConfigurationValue) {
            // Unsure of what I want to do here...
            // Seems like we release the FIFO buffer for host to use when needed. With this we also flush the buffer
            // Unknown why its 0x3A
		}
	}
}

// USB Endpoint Interrupt Service Routine
ISR(USB_COM_vect) {
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;

    const uint8_t *list;
	uint16_t desc_val;
	const uint8_t *desc_addr;
	uint8_t	desc_length;

	uint16_t packetLength;

    // Select the endpoint number so that the CPU can then access to the various endpoint registers and data
    UENUM = ENDPOINT_0_CONTROL_TRANSFER;
    // Check if a new setup packet has been received by reading interrupt flag RXSTPI
    if (UEINTX & (1 << RXSTPI)) {
        bmRequestType = UEDATX; // Determine the direction of the request
        bRequest = UEDATX; // Determine what is the particular request being made
        wValue = UEDATX;
        wValue |= (UEDATX << 8);
        wIndex = UEDATX;
        wIndex |= (UEDATX << 8);
        wLength = UEDATX; // Specify the number of bytes to be transferred should there be a data phase
        wLength |= (UEDATX << 8);

		char buffer[4];
		uart_print("bmRequestType bRequest ");
		itoa(bmRequestType, buffer, 10);  // Convert uint8_t to string (base 10)
		uart_print(buffer);
		uart_print(" ");
		itoa(bRequest, buffer, 10);  // Convert uint8_t to string (base 10)
		uart_print(buffer);
		uart_print(" wValue ");
		itoa((wValue >> 8), buffer, 10);  // Convert uint8_t to string (base 10)
		uart_print(buffer);
		uart_print(" ");
		itoa((wValue & 0xFF), buffer, 10);  // Convert uint8_t to string (base 10)
		uart_print(buffer);
		uart_print(" ");
		uart_print(" Wlength ");
		itoa((wLength >> 8), buffer, 10);  // Convert uint8_t to string (base 10)
		uart_print(buffer);
		uart_print(" ");
		itoa((wLength & 0xFF), buffer, 10);  // Convert uint8_t to string (base 10)
		uart_print(buffer);
		uart_print("\r\n");

        /* 
        RXSTPI is set when a new SETUP is received. It shall be cleared by firmware to acknowledge the packet
        and to clear the endpoint bank.
        RXOUTI is set when a new OUT data is received. It shall be cleared by firmware to acknowledge the
        packet and to clear the endpoint bank.
        TXINI is set when the bank is ready to accept a new IN packet. It shall be cleared by firmware to send the
        packet and to clear the endpoint bank.
        */
        UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI)); // Clear flags. Why am I clearing these out and in flags?

        if (bRequest == GET_DESCRIPTOR && bmRequestType == 0x80) {
			uart_print("GET_DESCRIPTOR\t");
			uint8_t descriptorType = (wValue >> 8);
	        uint8_t descriptorIndex = (wValue & 0xFF);
			uint16_t transferLength = 0;
			uint16_t descriptorLength = 0;
			uint8_t *descriptorAddr = NULL;
            switch (descriptorType) {
                case DESCRIPTOR_TYPE_DEVICE:
					uart_print("Selecting DESCRIPTOR_TYPE_DEVICE\r\n");
					PORTE |= (1 << PORTE6);
                    descriptorLength = sizeof(deviceDescriptor_t);
					descriptorAddr = (uint8_t*)&usbDescriptors.deviceDescriptor;
                break;
                case DESCRIPTOR_TYPE_CONFIGURATION:
					uart_print("Selecting DESCRIPTOR_TYPE_CONFIGURATION\r\n");
                    descriptorLength = (sizeof(configurationDescriptor_t) + sizeof(interfaceDescriptor_t) + sizeof(hidDescriptor_t) + sizeof(endpointDescriptor_t));
					descriptorAddr = (uint8_t*)&usbDescriptors.configurationDescriptor;
                break;
                // case STRING_DESCRIPTOR_TYPE:
				// 	// Pending
                // break;
                default:
					uart_print("Stalling...\r\n");
                    UECONX = (1 << STALLRQ) | (1 << EPEN); //stall
            }

			transferLength = (wLength < 256) ? wLength : 255; // Maximum payload size is the endpoint size // This is rather the size of uint8
			if (transferLength > descriptorLength) {
				transferLength = descriptorLength;
			}

			while (transferLength) {
				while (!(UEINTX & (1 << TXINI))) {
					// Wait until bank signals its ready to accept a new IN packet
				}
				packetLength = (transferLength < ENDPOINT0_SIZE) ? transferLength : ENDPOINT0_SIZE;
				for (uint8_t i = 0; i < packetLength; i++) {
					UEDATX = pgm_read_byte(descriptorAddr++);
				}
				transferLength -= packetLength;
				UEINTX = ~(1<<TXINI); // Clear flag to send data and wipe endpoint bank
			}

			return;
        }
		if (bRequest == SET_ADDRESS && bmRequestType == 0x00) {
			PORTC |= (1 << PORTC6);
			UEINTX = ~(1<<TXINI); // Firmware to sends an IN command of 0 bytes 
			while (!(UEINTX & (1<<TXINI))) {
				// Wait until bank signals its ready to accept a new IN packet. 
				// We should be at status stage now?
			}
			// Apperantly you are not suppose to do this at the same time. RIP
			UDADDR = wValue | (1<<ADDEN); // Record the received address and enable the USB device address
			usbAddressConfig |= (1 << 2);
			return;
		}
		if (bRequest == SET_CONFIGURATION && bmRequestType == 0x00) {
			usbConfigurationValue = wValue & 0xFF; // Configuration in lower byte. Some call it status because once this variable as a value in signals device its configured
			UEINTX = ~(1<<TXINI); // Clear flag to send data and wipe endpoint bank
			UENUM = ENDPOINT_3_KEYBOARD;
			UECONX = 1;
			UECFG0X = 0b11000001;  // EPTYPE Interrupt IN
			UECFG1X = 0b00000110;  // Dual Bank Endpoint, 8 Bytes, allocate memory
			UERST = 0x1E;          // Reset all of the endpoints
			UERST = 0;
			return;
		} // Do a survey of what we actually need to get started.  
		if (bRequest == GET_CONFIGURATION && bmRequestType == 0x80) {
			while (!(UEINTX & (1<<TXINI))) {
				// Wait until bank signals its ready to accept a new IN packet. 
				// We should be at status stage now?
			}
			UEDATX = usbConfigurationValue;
			UEINTX &= ~(1 << TXINI);
			return;
		}
		if (bRequest == GET_STATUS) {
			while (!(UEINTX & (1 << TXINI)));
			UEDATX = 0;
			UEDATX = 0;
			UEINTX &= ~(1 << TXINI);
			return;
		}
        UECONX = (1<<STALLRQ) | (1<<EPEN);	// stall
    }



    // Clear RXSTPI the to acknowledge the setup packet and to clear the endpoint bank

    // Free the bank by clearing FIFOCON when all the data is read. 
    // The FIFOCON and RWAL fields are irrelevant with CONTROL endpoints
}