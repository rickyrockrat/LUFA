/*
             MyUSB Library
     Copyright (C) Dean Camera, 2008.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com

 Released under the LGPL Licence, Version 3
*/

#ifndef _TESTAPP_H_
#define _TESTAPP_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/interrupt.h>

		#include <MyUSB/Drivers/USB/USB.h>                        // USB Functionality
		#include <MyUSB/Scheduler/Scheduler.h>                    // Simple scheduler for task management
		#include <MyUSB/MemoryAllocator/DynAlloc.h>               // Auto-defragmenting Dynamic Memory allocation
		#include <MyUSB/Common/ButtLoadTag.h>                     // PROGMEM tags readable by the ButtLoad project
		#include <MyUSB/Drivers/Misc/TerminalCodes.h>             // ANSI Terminal Escape Codes
		#include <MyUSB/Drivers/AT90USB_64x_128x/ADC.h>           // ADC driver
		#include <MyUSB/Drivers/AT90USB_64x_128x/Serial_Stream.h> // USART Stream driver
		#include <MyUSB/Drivers/USBKEY/Joystick.h>                // Joystick driver for the USBKEY
		#include <MyUSB/Drivers/USBKEY/Bicolour.h>                // Bicolour LEDs driver for the USBKEY
		#include <MyUSB/Drivers/USBKEY/HWB.h>                     // Hardware Button driver for the USBKEY
		#include <MyUSB/Drivers/USBKEY/Temperature.h>             // Temperature sensor driver
		
	/* Task Definitions: */
		TASK(TestApp_CheckJoystick);
		TASK(TestApp_CheckHWB);
		TASK(TestApp_CheckTemp);

#endif
