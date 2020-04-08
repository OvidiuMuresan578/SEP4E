/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define mainRECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define	mainSEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )

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

	char c;
	setUpConnection();

	while(1)
	{

		printf("In send task\r\n");
		c = _getch();

		char bytes_to_send[1];
		bytes_to_send[0] = c;

		// Send specified text:  bytes_to_send
		DWORD bytes_written, total_bytes_written = 0;
		if (c == 'w' || c == 's');
			if (!WriteFile(hSerial, bytes_to_send, 1, &bytes_written, NULL))
			{
			fprintf(stderr, "Error\n");
			CloseHandle(hSerial);
			return 1;
			}
			vTaskDelay(1);
	}
	/*-----------------------------------------------------------*/

}

static void prvReceiveTask(void *pvParameters)
{
	/* Prevent the compiler warning about the unused parameter. */
	(void)pvParameters;

	//printf("Before receive task\r\n");
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
		//	fprintf(stderr, "%d Number of bytes read\n", bytes_read);
			fprintf(stderr, "%s\n", bytes_to_receive);
		}

	}
}
/*-----------------------------------------------------------*/


static int setUpConnection() {
	DCB dcbSerialParams = { 0 };
	COMMTIMEOUTS timeouts = { 0 };

	fprintf(stderr, "Opening serial port...");

	// In "Device Manager" find "Ports (COM & LPT)" and see which COM port the game controller is using (here COM3)
	hSerial = CreateFile(
		"COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
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