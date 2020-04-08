/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

 /******************************************************************************
  * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
  * do not expect to get real time behaviour from the FreeRTOS Windows port, or
  * this demo application.  Also, the timing information in the FreeRTOS+Trace
  * logs have no meaningful units.  See the documentation page for the Windows
  * port for further information:
  * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
  *
  * /

  /* Standard includes. */
#include <stdio.h>
#include <conio.h>

  /* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define mainRECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define	mainSEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/*-----------------------------------------------------------*/

// Tasks
static void prvReceiveTask(void *pvParameters);
static void prvSendTask(void *pvParameters);

// Prototypes
static int setUpConnection();
static int closeConnection();

/*-----------------------------------------------------------*/

// Declare variables and structures
HANDLE hSerial;
#define nrOfBytes 10;

/*-----------------------------------------------------------*/


/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_blinky(void)
{
	// Create tasks:
	xTaskCreate(prvReceiveTask,			/* The function that implements the task. */
		"Rx", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
		configMINIMAL_STACK_SIZE, 		/* The size of the stack to allocate to the task. */
		NULL, 							/* The parameter passed to the task - not used in this simple case. */
		mainRECEIVE_TASK_PRIORITY,/* The priority assigned to the task. */
		NULL);							/* The task handle is not required, so NULL is passed. */

	xTaskCreate(prvSendTask, "TX", configMINIMAL_STACK_SIZE, NULL, mainSEND_TASK_PRIORITY, NULL);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	for (;; );
}
/*-----------------------------------------------------------*/

static void prvSendTask(void *pvParameters)
{
	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;
	
	char c='A';
	setUpConnection();
	
	for (;; )
	{
		printf("In send task %d\r\n", c);
		
		printf("%c", c);

		char bytes_to_send[1];
		bytes_to_send[0] = c;
		
		// Send specified text:  bytes_to_send
		DWORD bytes_written, total_bytes_written = 0;
		fprintf(stderr, "Sending bytes...");

		if (!WriteFile(hSerial, bytes_to_send, 1, &bytes_written, NULL))
		{
			fprintf(stderr, "Error\n");
			CloseHandle(hSerial);
			return 1;
		}
		c++;
		vTaskDelay(1000);
	}
	/*-----------------------------------------------------------*/

}

static void prvReceiveTask(void *pvParameters)
{
	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	printf("Before receive task\r\n");
	vTaskDelay(1000);

	char bytes_to_receive[50];

	for (;; )
	{


	//	printf("In receive task\r\n");
		vTaskDelay(100);

		// Read text 
		DWORD bytes_read, total_bytes_read = 0;

		ReadFile(hSerial, bytes_to_receive, 50, &bytes_read, total_bytes_read);

		if (bytes_read != 0) {
			bytes_to_receive[bytes_read] = '\0';
			fprintf(stderr, "%d Number of bytes read\n", bytes_read);
			fprintf(stderr, "Text read:  %s\n", bytes_to_receive);
		}
		
	}
}
/*-----------------------------------------------------------*/


static int setUpConnection() {
	DCB dcbSerialParams = { 0 };
	COMMTIMEOUTS timeouts = { 0 };

	fprintf(stderr, "Opening serial port...");

	// In "Device Manager" find "Ports (COM & LPT)" and see which COM port the game controller is using (here COM7)
	hSerial = CreateFile(
		"COM7", GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hSerial == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error\n");
		return 1;
	}
	else fprintf(stderr, "OK\n");
	vTaskDelay(1000);

	// Set device parameters (115200 baud, 1 start bit,
	// 1 stop bit, no parity)
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (GetCommState(hSerial, &dcbSerialParams) == 0)
	{
		fprintf(stderr, "Error getting device state\n");
		CloseHandle(hSerial);
		return 1;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (SetCommState(hSerial, &dcbSerialParams) == 0)
	{
		fprintf(stderr, "Error setting device parameters\n");
		CloseHandle(hSerial);
		return 1;
	}

	// Set COM port timeout settings
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (SetCommTimeouts(hSerial, &timeouts) == 0)
	{
		fprintf(stderr, "Error setting timeouts\n");
		CloseHandle(hSerial);
		return 1;
	}
	return 0;


}

static int closeConnection() {

	// Close serial port
	fprintf(stderr, "Closing serial port...");
	if (CloseHandle(hSerial) == 0)
	{
		fprintf(stderr, "Error\n");
		return 1;
	}
	return 0;
}