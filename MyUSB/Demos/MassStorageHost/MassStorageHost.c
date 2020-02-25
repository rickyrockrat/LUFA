/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

/*
	Mass Storage host demonstration application. This gives a simple reference
	application for implementing a USB Mass Storage host, for USB storage devices
	using the standard Mass Storage USB profile.
	
	The first 512 bytes (boot sector) of an attached disk's memory will be dumped
	out of the serial port when it is attached to the AT90USB1287 AVR.
	
	Requires header files from the Mass Storage Device demonstation application.
*/

#include "MassStorageHost.h"

/* Project Tags, for reading out using the ButtLoad project */
BUTTLOADTAG(ProjName,  "MyUSB MS Host App");
BUTTLOADTAG(BuildTime, __TIME__);
BUTTLOADTAG(BuildDate, __DATE__);

/* Scheduler Task List */
TASK_LIST
{
	{ Task: USB_USBTask          , TaskStatus: TASK_RUN  },
	{ Task: USB_MassStore_Host   , TaskStatus: TASK_RUN  },
};

/* Globals */
uint8_t  MassStoreEndpointNumber_IN;
uint8_t  MassStoreEndpointNumber_OUT;
uint16_t MassStoreEndpointSize_IN;
uint16_t MassStoreEndpointSize_OUT;
uint8_t  MassStore_NumberOfLUNs;
uint32_t MassStore_Tag = 1;

int main(void)
{
	/* Disable Clock Division */
	CLKPR = (1 << CLKPCE);
	CLKPR = 0;

	/* Hardware Initialization */
	SerialStream_Init(9600);
	Bicolour_Init();
	
	/* Initial LED colour - Double red to indicate USB not ready */
	Bicolour_SetLeds(BICOLOUR_LED1_RED | BICOLOUR_LED2_RED);
	
	/* Initialize USB Subsystem */
	USB_Init(USB_MODE_HOST, USB_OPT_REG_ENABLED);

	/* Startup message */
	puts_P(PSTR(ESC_RESET ESC_BG_WHITE ESC_INVERSE_ON ESC_ERASE_DISPLAY
	       "MassStore Host Demo running.\r\n" ESC_INVERSE_OFF));
		   
	/* Scheduling - routine never returns, so put this last in the main function */
	Scheduler_Start();
}

EVENT_HANDLER(USB_DeviceAttached)
{
	puts_P(PSTR("Device Attached.\r\n"));
	Bicolour_SetLeds(BICOLOUR_NO_LEDS);	
}

EVENT_HANDLER(USB_DeviceUnattached)
{
	puts_P(PSTR("\r\nDevice Unattached.\r\n"));
	Bicolour_SetLeds(BICOLOUR_LED1_RED | BICOLOUR_LED2_RED);
}

EVENT_HANDLER(USB_HostError)
{
	USB_ShutDown();

	puts_P(PSTR(ESC_BG_RED "Host Mode Error\r\n"));
	printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);

	Bicolour_SetLeds(BICOLOUR_LED1_RED | BICOLOUR_LED2_RED);
	for(;;);
}

EVENT_HANDLER(USB_DeviceEnumerationFailed)
{
	puts_P(PSTR(ESC_BG_RED "Dev Enum Error\r\n"));
	printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);
	printf_P(PSTR(" -- In State %d\r\n"), USB_HostState);
}

TASK(USB_MassStore_Host)
{
	uint8_t ErrorCode;

	/* Block task if device not connected */
	if (!(USB_IsConnected))
		return;

	switch (USB_HostState)
	{
		case HOST_STATE_Addressed:
			/* Standard request to set the device configuration to configuration 1 */
			USB_HostRequest = (USB_Host_Request_Header_t)
				{
					RequestType: (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE),
					RequestData: REQ_SetConfiguration,
					Value:       1,
					Index:       0,
					DataLength:  0,
				};
				
			/* Send the request, display error and wait for device detatch if request fails */
			if (USB_Host_SendControlRequest(NULL) != HOST_SENDCONTROL_Sucessful)
			{
				puts_P(PSTR("Control error."));

				/* Indicate error via status LEDs */
				Bicolour_SetLeds(BICOLOUR_LED1_RED);

				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}
				
			USB_HostState = HOST_STATE_Configured;
			break;
		case HOST_STATE_Configured:
			puts_P(PSTR("Getting Config Data.\r\n"));
		
			/* Get and process the configuration descriptor data */
			ErrorCode = GetConfigDescriptorData();
			
			/* Check if the configuration descriptor processing was sucessful */
			if (ErrorCode != SuccessfulConfigRead)
			{
				switch (ErrorCode)
				{
					case InterfaceNotFound:
						puts_P(PSTR("Invalid Device Type.\r\n"));
						break;
					default:
						puts_P(PSTR("Control Error.\r\n"));
						break;
				}

				/* Indicate error via status LEDs */
				Bicolour_SetLeds(BICOLOUR_LED1_RED);
				
				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}

			/* Configure the data pipes */
			Pipe_ConfigurePipe(MASS_STORE_DATA_IN_PIPE, EP_TYPE_BULK, PIPE_TOKEN_IN,
			                   MassStoreEndpointNumber_IN, MassStoreEndpointSize_IN,
			                   PIPE_BANK_DOUBLE);

			Pipe_ConfigurePipe(MASS_STORE_DATA_OUT_PIPE, EP_TYPE_BULK, PIPE_TOKEN_OUT,
			                   MassStoreEndpointNumber_OUT, MassStoreEndpointSize_OUT,
			                   PIPE_BANK_DOUBLE);

			Pipe_SelectPipe(MASS_STORE_DATA_IN_PIPE);
			Pipe_SetInfiniteINRequests();

			Pipe_SelectPipe(MASS_STORE_DATA_OUT_PIPE);

			puts_P(PSTR("Mass Storage Disk Enumerated.\r\n"));
				
			USB_HostState = HOST_STATE_Ready;
			break;
		case HOST_STATE_Ready:
			/* Indicate device busy via the status LEDs */
			Bicolour_SetLeds(BICOLOUR_LED2_ORANGE);
			
			/* Request to prepare the disk for use */
			USB_HostRequest = (USB_Host_Request_Header_t)
				{
					RequestType: (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE),
					RequestData: MASS_STORAGE_RESET,
					Value:       0,
					Index:       0,
					DataLength:  0,
				};

			/* Send the request, display error and wait for device detatch if request fails */
			if (USB_Host_SendControlRequest(NULL) != HOST_SENDCONTROL_Sucessful)
			{
				puts_P(PSTR("Control error."));

				/* Indicate error via status LEDs */
				Bicolour_SetLeds(BICOLOUR_LED1_RED);

				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}
			
			/* Request to retrieve the maximum LUN index from the device */
			USB_HostRequest = (USB_Host_Request_Header_t)
				{
					RequestType: (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE),
					RequestData: GET_MAX_LUN,
					Value:       0,
					Index:       0,
					DataLength:  1,
				};

			/* Send the request, display error and wait for device detatch if request fails */
			if (USB_Host_SendControlRequest(&MassStore_NumberOfLUNs) != HOST_SENDCONTROL_Sucessful)
			{
				puts_P(PSTR("Control error."));

				/* Indicate error via status LEDs */
				Bicolour_SetLeds(BICOLOUR_LED1_RED);

				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}

			/* Create a new buffer capabable of holding a single block from the device */
			uint8_t BlockBuffer[DEVICE_BLOCK_SIZE];
						
			/* Read in the first 512 byte block from the device */
			if (!(MassStore_ReadDeviceBlock(0, 1, BlockBuffer)))
			{
				puts_P(PSTR(ESC_BG_RED "Error reading disk.\r\n"));

				/* Indicate device error via the status LEDs */
				Bicolour_SetLeds(BICOLOUR_LED1_RED);				

				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}
			else
			{
				/* Print the block bytes out through the serial USART */
				for (uint16_t Byte = 0; Byte < DEVICE_BLOCK_SIZE; Byte++)
				  Serial_TxByte(BlockBuffer[Byte]);
			}
			
			/* Indicate device no longer busy */
			Bicolour_SetLeds(BICOLOUR_LED2_GREEN);			
			
			/* Wait until USB device disconnected */
			while (USB_IsConnected);
			
			break;
	}
}

uint8_t GetConfigDescriptorData(void)
{
	uint8_t* ConfigDescriptorData;
	uint16_t ConfigDescriptorSize;
	
	/* Get Configuration Descriptor size from the device */
	if (AVR_HOST_GetDeviceConfigDescriptor(&ConfigDescriptorSize, NULL) != HOST_SENDCONTROL_Sucessful)
	  return ControlError;
	
	/* Ensure that the Configuration Descriptor isn't too large */
	if (ConfigDescriptorSize > MAX_CONFIG_DESCRIPTOR_SIZE)
	  return DescriptorTooLarge;
	  
	/* Allocate enough memory for the entire config descriptor */
	ConfigDescriptorData = __builtin_alloca(ConfigDescriptorSize);

	/* Retrieve the entire configuration descriptor into the allocated buffer */
	AVR_HOST_GetDeviceConfigDescriptor(&ConfigDescriptorSize, ConfigDescriptorData);
	
	/* Validate returned data - ensure first entry is a configuration header descriptor */
	if (((USB_Descriptor_Header_t*)ConfigDescriptorData)->Type != DTYPE_Configuration)
	  return ControlError;
	
	while (ConfigDescriptorSize)
	{
		/* Find next interface descriptor */
		while (ConfigDescriptorSize)
		{
			/* Get the next descriptor from the configuration descriptor data */
			AVR_HOST_GetNextDescriptor(&ConfigDescriptorSize, &ConfigDescriptorData);	  

			/* Check to see if the next descriptor is an interface descriptor, if so break out */
			if (((USB_Descriptor_Header_t*)ConfigDescriptorData)->Type == DTYPE_Interface)
			  break;
		}

		/* If reached end of configuration descriptor, error out */
		if (ConfigDescriptorSize == 0)
		  return InterfaceNotFound;	

		/* Check the descriptor's class/subclass/protocol, break out if class matches expected values */
		if ((((USB_Descriptor_Interface_t*)ConfigDescriptorData)->Class    == MASS_STORE_CLASS) &&
		    (((USB_Descriptor_Interface_t*)ConfigDescriptorData)->SubClass == MASS_STORE_SUBCLASS) &&
			(((USB_Descriptor_Interface_t*)ConfigDescriptorData)->Protocol == MASS_STORE_PROTOCOL))
		{
			break;
		}
	}
	
	/* If reached end of configuration descriptor, error out */
	if (ConfigDescriptorSize == 0)
	  return InterfaceNotFound;
	
	/* Find the IN and OUT endpoint descriptors after the mass storage interface descriptor */
	while (ConfigDescriptorSize)
	{
		/* Get the next descriptor from the configuration descriptor data */
		AVR_HOST_GetNextDescriptor(&ConfigDescriptorSize, &ConfigDescriptorData);	  		

		/* Check if current descritor is a BULK type endpoint descriptor */
		if ((((USB_Descriptor_Header_t*)ConfigDescriptorData)->Type == DTYPE_Endpoint) &&
		    (((USB_Descriptor_Endpoint_t*)ConfigDescriptorData)->Attributes == EP_TYPE_BULK))
		{
			uint8_t  EPAddress = ((USB_Descriptor_Endpoint_t*)ConfigDescriptorData)->EndpointAddress;
			uint16_t EPSize    = ((USB_Descriptor_Endpoint_t*)ConfigDescriptorData)->EndpointSize;
		
			/* Set the appropriate endpoint data address based on the endpoint direction */
			if (EPAddress & ENDPOINT_DESCRIPTOR_DIR_IN)
			{
				MassStoreEndpointNumber_IN  = EPAddress;
				MassStoreEndpointSize_IN    = EPSize;
			}
			else
			{
				MassStoreEndpointNumber_OUT = EPAddress;
				MassStoreEndpointSize_OUT   = EPSize;
			}
		}
		
		/* If both data pipes found, exit the loop */
		if (MassStoreEndpointNumber_IN && MassStoreEndpointNumber_OUT)
		  break;
		
		/* If new interface descriptor found (indicating the end of the current interface), error out */
		if (((USB_Descriptor_Header_t*)ConfigDescriptorData)->Type == DTYPE_Interface)
		  return NoEndpointFound;
	}	

	/* If reached end of configuration descriptor, error out */
	if (ConfigDescriptorSize == 0)
	  return NoEndpointFound;
	
	/* Valid data found, return success */
	return SuccessfulConfigRead;
}

bool MassStore_ReadDeviceBlock(const uint32_t BlockAddress, const uint8_t Blocks, uint8_t* BufferPtr)
{
	uint16_t BytesRem = (Blocks * DEVICE_BLOCK_SIZE);

	/* Create a CBW with a SCSI command to read in the given blocks from the device */
	CommandBlockWrapper_t SCSICommand =
		{
			Header:
				{
					Signature:          CBW_SIGNATURE,
					Tag:                MassStore_Tag,
					DataTransferLength: BytesRem,
					Flags:              COMMAND_DIRECTION_DATA_IN,
					LUN:                0x00,
					SCSICommandLength:  10
				},
					
			SCSICommandData:
				{
					SCSI_CMD_READ_10,
					0x00,                   // Unused (control bits, all off)
					(BlockAddress >> 24),   // MSB of Block Address
					(BlockAddress >> 16),
					(BlockAddress >> 8),
					(BlockAddress & 0xFF),  // LSB of Block Address
					0x00,                   // Unused (reserved)
					0x00,                   // MSB of Total Blocks to Read
					Blocks,                 // LSB of Total Blocks to Read
					0x00                    // Unused (control)
				}
		};
			
	uint8_t* CommandByte = (uint8_t*)&SCSICommand;

	/* Each transmission should have a unique tag value */
	MassStore_Tag++;

	/* Select the OUT data pipe for CBW transmission */
	Pipe_SelectPipe(MASS_STORE_DATA_OUT_PIPE);
	Pipe_Unfreeze();

	/* Write the CBW to the OUT pipe */
	for (uint8_t Byte = 0; Byte < sizeof(CommandBlockWrapper_t); Byte++)
	  Pipe_Write_Byte(*(CommandByte++));
	  
	/* Send the data in the OUT pipe to the attached device */
	Pipe_FIFOCON_Clear();
	Pipe_Freeze();

	/* Select the IN data pipe for data reception */
	Pipe_SelectPipe(MASS_STORE_DATA_IN_PIPE);
	Pipe_Unfreeze();	
		
	/* Wait until data recieved in the IN pipe */
	while (!(Pipe_ReadWriteAllowed()))
	{
		Pipe_SelectPipe(MASS_STORE_DATA_OUT_PIPE);

		/* Check if pipe stalled (command failed by device) */
		if (Pipe_IsStalled())
		  return false;

		Pipe_SelectPipe(MASS_STORE_DATA_IN_PIPE);

		/* Check if pipe stalled (command failed by device) */
		if (Pipe_IsStalled())
		  return false;
		  
		/* Check to see if the device was disconnected, if so exit function */
		if (!(USB_IsConnected))
		  return false;
	};
	
	/* Loop until all bytes read */
	while (BytesRem)
	{
		/* Load each byte into the buffer */
		*(BufferPtr++) = Pipe_Read_Byte();
		
		/* Decrement the bytes remaining counter */
		BytesRem--;
		
		/* Check to see if the device was disconnected, if so exit function */
		if (!(USB_IsConnected))
		  return false;

		/* When pipe is empty, clear it and wait for the next packet */
		if (!(Pipe_BytesInPipe()))
		{
			Pipe_FIFOCON_Clear();
			while (!(Pipe_ReadWriteAllowed()));
		}
	}
			
	/* Ignore the returned CSW */
	while (!(Pipe_ReadWriteAllowed()));
	Pipe_FIFOCON_Clear();
	Pipe_Freeze();

	/* Wait one frame for the device to prepare for next command */
	USB_Host_WaitMS(1);

	return true;
}

bool MassStore_WriteDeviceBlock(const uint32_t BlockAddress, const uint8_t Blocks, uint8_t* BufferPtr)
{
	uint16_t BytesRem        = (Blocks * DEVICE_BLOCK_SIZE);
	uint16_t BytesInEndpoint = 0;

	/* Create a CBW with a SCSI command to write the given blocks to the device */
	CommandBlockWrapper_t SCSICommand =
		{
			Header:
				{
					Signature:          CBW_SIGNATURE,
					Tag:                MassStore_Tag,
					DataTransferLength: BytesRem,
					Flags:              COMMAND_DIRECTION_DATA_OUT,
					LUN:                0x00,
					SCSICommandLength:  10
				},
					
			SCSICommandData:
				{
					SCSI_CMD_WRITE_10,
					0x00,                   // Unused (control bits, all off)
					(BlockAddress >> 24),   // MSB of Block Address
					(BlockAddress >> 16),
					(BlockAddress >> 8),
					(BlockAddress & 0xFF),  // LSB of Block Address
					0x00,                   // Unused (reserved)
					0x00,                   // MSB of Total Blocks to Read
					Blocks,                 // LSB of Total Blocks to Read
					0x00                    // Unused (control)
				}
		};
			
	uint8_t* CommandByte = (uint8_t*)&SCSICommand;
			
	/* Each transmission should have a unique tag value */
	MassStore_Tag++;

	/* Select the OUT data pipe for CBW transmission */
	Pipe_SelectPipe(MASS_STORE_DATA_OUT_PIPE);
	Pipe_Unfreeze();

	/* Write the CBW to the OUT pipe */
	for (uint8_t Byte = 0; Byte < sizeof(CommandBlockWrapper_t); Byte++)
	  Pipe_Write_Byte(*(CommandByte++));
	  
	/* Send the data in the OUT pipe to the attached device */
	Pipe_FIFOCON_Clear();

	/* Write the block data to the pipe */
	while (BytesRem)
	{
		Pipe_Write_Byte(*(BufferPtr++));
		
		BytesRem--;
		BytesInEndpoint++;
		
		/* Check if the pipe is full */
		if (BytesInEndpoint == MassStoreEndpointSize_OUT)
		{
			/* Send the pipe data, clear the counter */
			Pipe_FIFOCON_Clear();
			BytesInEndpoint = 0;
		}
	}
	
	/* Check to see if any data is still in the pipe - if so, send it */
	if (BytesInEndpoint)
	  Pipe_FIFOCON_Clear();
	
	/* Freeze OUT pipe after use */
	Pipe_Unfreeze();	

	/* Select the IN data pipe for data reception */
	Pipe_SelectPipe(MASS_STORE_DATA_IN_PIPE);
	Pipe_Unfreeze();	

	/* Ignore the returned CSW */
	while (!(Pipe_ReadWriteAllowed()));
	Pipe_FIFOCON_Clear();
	Pipe_Freeze();

	/* Wait one frame for the device to prepare for next command */
	USB_Host_WaitMS(1);

	return true;
}
