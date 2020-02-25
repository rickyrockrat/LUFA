/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

#include "USBMode.h"
#if defined(USB_CAN_BE_HOST)

#include "Host.h"

uint8_t USB_Host_WaitMS(uint8_t MS)
{
	bool    SOFGenEnabled  = USB_HOST_SOFGeneration_IsEnabled();
	uint8_t ErrorCode      = HOST_WAITERROR_Sucessful;
	
	USB_INT_Clear(USB_INT_HSOFI);
	USB_HOST_SOFGeneration_Enable();

	while (MS)
	{
		if (USB_INT_HasOccurred(USB_INT_HSOFI))
		{
			USB_INT_Clear(USB_INT_HSOFI);
			MS--;
		}
					
		if ((USB_IsConnected == false) || (USB_CurrentMode == USB_MODE_DEVICE))
		{
			ErrorCode = HOST_WAITERROR_DeviceDisconnect;
			
			break;
		}

		if (Pipe_IsError() == true)
		{
			Pipe_ClearError();
			ErrorCode = HOST_WAITERROR_PipeError;
			
			break;
		}
		
		if (Pipe_IsStalled() == true)
		{
			Pipe_ClearStall();
			ErrorCode = HOST_WAITERROR_SetupStalled;
			
			break;			
		}
	}

	if (!(SOFGenEnabled))
	  USB_HOST_SOFGeneration_Disable();

	return ErrorCode;
}

void USB_Host_ResetDevice(void)
{
	bool SOFGenEnabled = USB_HOST_SOFGeneration_IsEnabled();

	USB_INT_Disable(USB_INT_DDISCI);
	
	USB_Host_WaitMS(20);

	USB_HOST_ResetBus();
	while (!(USB_HOST_ResetBus_IsDone()));

	USB_INT_Clear(USB_INT_HSOFI);
	USB_HOST_SOFGeneration_Enable();	
	
	for (uint8_t MSRem = 10; MSRem != 0; MSRem--)
	{
		/* Workaround for powerless-pullup devices. After a USB bus reset,
		   all disconnection/connection interrupts are supressed while a
		   USB frame is looked for - if it is found within 10ms, the device
		   is still present.                                                */

		if (USB_INT_HasOccurred(USB_INT_HSOFI))
		{
			USB_INT_Clear(USB_INT_DDISCI);
			break;
		}
		
		_delay_ms(1);
	}

	if (!(SOFGenEnabled))
	  USB_HOST_SOFGeneration_Disable();

	USB_INT_Enable(USB_INT_DDISCI);
}

#endif
