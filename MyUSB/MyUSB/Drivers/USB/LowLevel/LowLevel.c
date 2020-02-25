/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

#include "USBMode.h"
#include "LowLevel.h"

volatile uint8_t USB_CurrentMode = USB_MODE_NONE;
         uint8_t USB_Options;

void USB_Init(const uint8_t Mode, const uint8_t Options)
{
	if (USB_IsInitialized)
	  USB_ShutDown();

	USB_CurrentMode = Mode;

	if (Mode == USB_MODE_NONE)
	{
		RAISE_EVENT(USB_PowerOnFail, POWERON_ERROR_NoUSBModeSpecified);
		return;
	}

	#if defined(USB_DEVICE_ONLY)
	if (Mode != USB_MODE_DEVICE)
	  RAISE_EVENT(USB_PowerOnFail, POWERON_ERROR_UnavailableUSBModeSpecified);
	else
	  UHWCON |=  (1 << UIMOD);
	#elif defined(USB_HOST_ONLY)
	if (Mode != USB_MODE_HOST)
	  RAISE_EVENT(USB_PowerOnFail, POWERON_ERROR_UnavailableUSBModeSpecified);
	else
	  UHWCON &= ~(1 << UIMOD);
	#else
	if (Mode == USB_MODE_UID)
	{
		UHWCON |=  (1 << UIDE);

		USB_INT_Clear(USB_INT_IDTI);
		USB_INT_Enable(USB_INT_IDTI);
		
		USB_CurrentMode = USB_GetUSBModeFromUID();
	}
	else if (Mode == USB_MODE_DEVICE)
	{
		UHWCON |=  (1 << UIMOD);
	}
	else if (Mode == USB_MODE_HOST)			
	{
		UHWCON &= ~(1 << UIMOD);
	}
	#endif

	#if defined(USB_CAN_BE_BOTH)
	USB_InitTaskPointer();
	#else
	USB_IsInitialized = true;
	USB_IsConnected   = false;
	#endif

	#if defined(USB_CAN_BE_HOST)
	USB_ControlPipeSize = PIPE_CONTROLPIPE_DEFAULT_SIZE;
	#endif
	
	USB_OTGPAD_On();
	
	USB_Options = Options;

	#if defined(USB_DEVICE_ONLY)
	USB_INT_Enable(USB_INT_VBUS);
	#elif defined(USB_HOST_ONLY)
	USB_SetupInterface();
	#else
	if (USB_CurrentMode == USB_MODE_DEVICE)
	  USB_INT_Enable(USB_INT_VBUS);
	else
	  USB_SetupInterface();
	#endif
	
	sei();
}

void USB_ShutDown(void)
{
	USB_ResetInterface();

	USB_CurrentMode = USB_MODE_NONE;
	USB_Options     = 0;

	USB_Interface_Disable();
	USB_PLL_Off();
	USB_REG_Off();
	USB_OTGPAD_Off();

	#if defined(USB_CAN_BE_BOTH)
	UHWCON &= ~(1 << UIDE);
	#endif
}

void USB_ResetInterface(void)
{
	if (USB_IsConnected)
	  RAISE_EVENT(USB_Disconnect);

	USB_INT_DisableAllInterrupts();
	USB_INT_ClearAllInterrupts();

	#if defined(USB_CAN_BE_DEVICE)
	Endpoint_ClearEndpoints();
	#endif

	#if defined(USB_CAN_BE_HOST)
	Pipe_ClearPipes();
	USB_HOST_VBUS_Auto_Enable();
	USB_HOST_VBUS_Auto_Off();
	USB_HOST_HostMode_Off();
	#endif
	
	USB_Detach();
	USB_REG_Off();

	USB_IsConnected         = false;
	USB_IsInitialized       = false;

	#if defined(USB_CAN_BE_HOST)
	USB_HostState           = HOST_STATE_Unattached;
	#endif

	#if defined(USB_CAN_BE_DEVICE)
	USB_ConfigurationNumber = 0;
	#endif
}

bool USB_SetupInterface(void)
{	
	USB_ResetInterface();

	#if defined(USB_CAN_BE_BOTH)
	if (UHWCON & (1 << UIDE))
	{
		USB_INT_Clear(USB_INT_IDTI);
		USB_INT_Enable(USB_INT_IDTI);
	}
	#endif

	#if defined(USB_CAN_BE_DEVICE)
	Endpoint_ClearEndpoints();
	#endif
	
	#if defined(USB_CAN_BE_HOST)
	Pipe_ClearPipes();
	#endif
	
	#if defined(USB_CAN_BE_BOTH)
	if (UHWCON & (1 << UIDE))
	{
		USB_INT_Enable(USB_INT_IDTI);
		USB_CurrentMode = USB_GetUSBModeFromUID();
	}

	if (USB_CurrentMode == USB_MODE_DEVICE)
	{
		if (USB_Options & USB_DEV_OPT_LOWSPEED)
		  USB_DEV_SetLowSpeed();
		else
		  USB_DEV_SetHighSpeed();
		  
		USB_INT_Enable(USB_INT_VBUS);
	}
	#elif defined(USB_DEVICE_ONLY)
	if (USB_Options & USB_DEV_OPT_LOWSPEED)
	  USB_DEV_SetLowSpeed();
	else
	  USB_DEV_SetHighSpeed();
		  
	USB_INT_Enable(USB_INT_VBUS);
	#endif
		
	if (!(USB_Options & USB_OPT_REG_DISABLED))
	  USB_REG_On();
	
	USB_PLL_On();
			
	while (!(USB_PLL_IsReady()));
		
	USB_Interface_Disable();
	USB_Interface_Enable();
	USB_CLK_Unfreeze();

	#if defined(USB_CAN_BE_HOST)
	USB_INT_Enable(USB_INT_VBERRI);
	#endif
	
	#if defined(USB_DEVICE_ONLY)
	Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL,
	                           ENDPOINT_DIR_OUT, ENDPOINT_CONTROLEP_SIZE,
	                           ENDPOINT_BANK_SINGLE);

	if (Endpoint_IsConfigured())
	{
		USB_Attach();
	}
	else
	{
		RAISE_EVENT(USB_DeviceError, DEVICE_ERROR_ControlEndpointCreationFailed);
		return USB_SETUPINTERFACE_FAIL;
	}

	USB_INT_Enable(USB_INT_SUSPEND);
	USB_INT_Enable(USB_INT_EORSTI);	
	#elif defined(USB_HOST_ONLY)
	USB_Attach();
	USB_HOST_HostMode_On();
	#else
	if (USB_CurrentMode == USB_MODE_DEVICE)
	{
		Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL,
		                           ENDPOINT_DIR_OUT, ENDPOINT_CONTROLEP_SIZE,
		                           ENDPOINT_BANK_SINGLE);

		if (Endpoint_IsConfigured())
		{
			USB_Attach();
		}
		else
		{
			RAISE_EVENT(USB_PowerOnFail, DEVICE_ERROR_ControlEndpointCreationFailed);
			return USB_SETUPINTERFACE_FAIL;
		}

		USB_INT_Enable(USB_INT_SUSPEND);
		USB_INT_Enable(USB_INT_EORSTI);
	}
	else if (USB_CurrentMode == USB_MODE_HOST)
	{
		USB_Attach();
		USB_HOST_HostMode_On();
	}
	#endif
	
	#if defined(USB_CAN_BE_BOTH)
	USB_InitTaskPointer();
	#else
	USB_IsInitialized = true;
	USB_IsConnected   = false;
	#endif

	return USB_SETUPINTERFACE_OK;
}
