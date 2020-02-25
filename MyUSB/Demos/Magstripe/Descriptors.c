/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

/*
	Demonstration application for a TTL magnetic stripe reader (such as the
	Omron V3B-4K) by Denver Gingerich. See http://ossguy.com/ss_usb/ for the
	demonstration project website, including construction and support details.

	This example is based on the MyUSB Keyboard demonstration application,
	written by Denver Gingerich.
*/

#include "Descriptors.h"

USB_Descriptor_HID_Keyboard_Report_t KeyboardReport PROGMEM =
{
	ReportData:
	{
		0x05, 0x01,          /* Usage Page (Generic Desktop)                    */
		0x09, 0x06,          /* Usage (Keyboard)                                */
		0xa1, 0x01,          /* Collection (Application)                        */
		0x05, 0x07,          /*   Usage Page (Keyboard)                         */
		0x19, 0xe0,          /*   Usage Minimum (Keyboard LeftControl)          */
		0x29, 0xe7,          /*   Usage Maximum (Keyboard Right GUI)            */
		0x15, 0x00,          /*   Logical Minimum (0)                           */
		0x25, 0x01,          /*   Logical Maximum (1)                           */
		0x75, 0x01,          /*   Report Size (1)                               */
		0x95, 0x08,          /*   Report Count (8)                              */
		0x81, 0x02,          /*   Input (Data, Variable, Absolute)              */
		0x95, 0x01,          /*   Report Count (1)                              */
		0x75, 0x08,          /*   Report Size (8)                               */
		0x25, 0x65,          /*   Logical Maximum (101)                         */
		0x19, 0x00,          /*   Usage Minimum (Reserved (no event indicated)) */
		0x29, 0x65,          /*   Usage Maximum (Keyboard Application)          */
		0x81, 0x00,          /*   Input (Data, Array, Absolute)                 */
		0xc0                 /* End Collection                                  */
	}
};

USB_Descriptor_Device_t DeviceDescriptor PROGMEM =
{
	Header:                 {Size: sizeof(USB_Descriptor_Device_t), Type: DTYPE_Device},
		
	USBSpecification:       0x0101,
	Class:                  0x00,
	SubClass:               0x00,
	Protocol:               0x00,
				
	Endpoint0Size:          ENDPOINT_CONTROLEP_SIZE,
		
	VendorID:               0x0000,
	ProductID:              USB_PRODUCT_ID('M', 'C'),
	ReleaseNumber:          0x0000,
		
	ManafacturerStrIndex:   0x01,
	ProductStrIndex:        0x02,
	SerialNumStrIndex:      0x03,
		
	NumberOfConfigurations: 1
};
	
USB_Descriptor_Configuration_t ConfigurationDescriptor PROGMEM =
{
	Config:
		{
			Header:                 {Size: sizeof(USB_Descriptor_Configuration_Header_t), Type: DTYPE_Configuration},

			TotalConfigurationSize: sizeof(USB_Descriptor_Configuration_t),
			TotalInterfaces:        1,
				
			ConfigurationNumber:    1,
			ConfigurationStrIndex:  NO_DESCRIPTOR_STRING,
				
			ConfigAttributes:       (USB_CONFIG_ATTR_BUSPOWERED | USB_CONFIG_ATTR_SELFPOWERED),
			
			MaxPowerConsumption:    USB_CONFIG_POWER_MA(100)
		},
		
	Interface:
		{
			Header:                 {Size: sizeof(USB_Descriptor_Interface_t), Type: DTYPE_Interface},

			InterfaceNumber:        KEYBOARD_INTERFACE_NUMBER,
			AlternateSetting:       KEYBOARD_INTERFACE_ALTERNATE,
			
			TotalEndpoints:         KEYBOARD_INTERFACE_ENDPOINTS,
				
			Class:                  KEYBOARD_INTERFACE_CLASS,
			SubClass:               KEYBOARD_INTERFACE_SUBCLASS,
			Protocol:               KEYBOARD_INTERFACE_PROTOCOL,
				
			InterfaceStrIndex:      NO_DESCRIPTOR_STRING
		},

	KeyboardHID:
		{  
			Header:                 {Size: sizeof(USB_Descriptor_HID_t), Type: DTYPE_HID},
			
			HIDSpec:                0x1001,
			CountryCode:            0x00,
			TotalHIDDescriptors:    0x01,
			HIDReportType:          0x22,
			HIDReportLength:        sizeof(USB_Descriptor_HID_Keyboard_Report_t)  
		},
		
	KeyboardEndpoint:
		{
			Header:                 {Size: sizeof(USB_Descriptor_Endpoint_t), 
			Type: DTYPE_Endpoint},
			EndpointAddress:        (ENDPOINT_DESCRIPTOR_DIR_IN | KEYBOARD_EPNUM),
			Attributes:             EP_TYPE_INTERRUPT,
			EndpointSize:           KEYBOARD_EPSIZE,
			PollingIntervalMS:      0x02
		}
};

USB_Descriptor_Language_t LanguageString PROGMEM =
{
	Header:                         {Size: sizeof(USB_Descriptor_Language_t), Type: DTYPE_String},
		
	LanguageID:                     LANGUAGE_ID_ENG
};

USB_Descriptor_String_t ManafacturerString PROGMEM =
{
	Header:                         {Size: USB_STRING_LEN(16), Type: DTYPE_String},
		
	UnicodeString:                  {'D','e','n','v','e','r',' ','G','i','n','g','e','r','i','c','h'}
};

USB_Descriptor_String_t ProductString PROGMEM =
{
	Header:                         {Size: USB_STRING_LEN(22), Type: DTYPE_String},
		
	UnicodeString:                  {'M','y','U','S','B',' ','C','a','r','d',' ','R','e','a','d','e','r',' ','D','e','m','o'}
};

USB_Descriptor_String_t SerialNumberString PROGMEM =
{
	Header:                 {Size: USB_STRING_LEN(13), Type: DTYPE_String},
		
	UnicodeString:          {'0','.','0','.','0','.','0','.','0','.','0','.','0'}
};

bool USB_GetDescriptor(const uint8_t Type, const uint8_t Index,
                       void** const DescriptorAddr, uint16_t* const Size)
{
	switch (Type)
	{
		case DTYPE_Device:
			*DescriptorAddr = (void*)&DeviceDescriptor;
			*Size           = sizeof(USB_Descriptor_Device_t);
			return true;
		case DTYPE_Configuration:
			*DescriptorAddr = (void*)&ConfigurationDescriptor;
			*Size           = sizeof(USB_Descriptor_Configuration_t);
			return true;
		case DTYPE_String:
			switch (Index)
			{
				case 0x00:
					*DescriptorAddr = (void*)&LanguageString;
					*Size           = sizeof(USB_Descriptor_Language_t);
					return true;
				case 0x01:
					*DescriptorAddr = (void*)&ManafacturerString;
					*Size           = pgm_read_byte(&ManafacturerString.Header.Size);
					return true;
				case 0x02:
					*DescriptorAddr = (void*)&ProductString;
					*Size           = pgm_read_byte(&ProductString.Header.Size);
					return true;
				case 0x03:
					*DescriptorAddr = (void*)&SerialNumberString;
					*Size           = pgm_read_byte(&SerialNumberString.Header.Size);
					return true;
			}
			
			break;
		case DTYPE_HID:
			*DescriptorAddr = (void*)&ConfigurationDescriptor.KeyboardHID;
			*Size           = sizeof(USB_Descriptor_HID_t);

			return true;
		case DTYPE_Report:
			*DescriptorAddr = (void*)&KeyboardReport;
			*Size           = sizeof(USB_Descriptor_HID_Keyboard_Report_t);

			return true;
	}
		
	return false;
}
