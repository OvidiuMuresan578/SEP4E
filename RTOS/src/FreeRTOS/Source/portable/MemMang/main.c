/*
* FreeRTOS_Project
SEP4 Stefan-Daniel Horvath
*/

#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// FfreeRTOS Includes
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>
#include <semphr.h>

#include "src/board/board.h"

static const uint8_t _COM_RX_QUEUE_LENGTH = 30;
static QueueHandle_t _x_com_received_chars_queue = NULL;
static SemaphoreHandle_t  xMutex = NULL;


// frame_buf contains a bit pattern for each column in the display
static uint16_t frame_buf[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int  down=		 0b0011000010;
int  left=		 0b0001000011;
int  up=		 0b0010000011;
int  right=		 0b0011000001;
int  released=	 0b0011000011;
int  middle=	 0b0000000011;


//game variables
int maze[10][14];

static int player1Y ;
static int player2Y ;
static int end ;
static int score1 ;
static int score2 ;

static int ballX ;
static int ballY ;
static int ballDirectionX ; //can be 1 or -1 going right or left
static int ballDirectionY ; //can be 1 or -1 going down or up

//-----------------------------------------

void connection(void *pvParameters)
{
	// The parameters are not used
	( void ) pvParameters;

	#if (configUSE_APPLICATION_TASK_TAG == 1)
	// Set task not to be used for tracing with R2R-Network
	vTaskSetApplicationTaskTag( NULL, ( void * ) 2 );
	#endif
	
	BaseType_t result = 0;
	uint8_t byte;
	
	while(1)
	{
		result = xQueueReceive(_x_com_received_chars_queue, &byte, 1000L); //this is what it receives
		
		if (result) { //if there is a result
			com_send_bytes(&byte, 1); //send byte back
			}else {//if there is no result
			com_send_bytes((uint8_t*)"TO", 2);//send back to 2 ?
		}
	}
}
//-----------------------------------------
void move_player_and_ball(void *pvParameters){
	// The parameters are not used
	( void ) pvParameters;
	#if (configUSE_APPLICATION_TASK_TAG == 1)
	// Set task no to be used for tracing with R2R-Network
	vTaskSetApplicationTaskTag( NULL, ( void * ) 1 );
	#endif
	//Game logic for P1 and ball from here
		while(1){
				player1Y = 1;
			    player2Y = 1;
			    end = 0;
			    ballX = 3;
			    ballY = 4;
			    ballDirectionX = 1; //can be 1 or -1 going right or left
			    ballDirectionY = 1; //can be 1 or -1 going down or up
		 //boolean for ending the game
		//From here on the game starts. If a game ends, it will just start again.
		while (end == 0) {
			if( xSemaphoreTake( xMutex, 500) ){
			
			int PinCValue = PINC & 0b11000011 ; // Masking PINC
			//Moving the character
				
			if(PinCValue==up)
				if (player1Y != 1){
						player1Y = player1Y - 1;
						player2Y = player2Y - 1;} //temporary until PC connectivity is implemented
					
			if(PinCValue==down)
				if (player1Y != 6){
						player1Y = player1Y + 1;
						player2Y = player2Y + 1;} //temporary until PC connectivity is implemented

			//Move ball

				if(maze[ballY+ballDirectionY][ballX+ballDirectionX]==1) //if ball is going and on the next position would enter the same pixel as a wall, it will change direction
					ballDirectionY=ballDirectionY*(-1);
					
				else if(maze[ballX+ballDirectionX][ballY+ballDirectionY]==2)
				{							 //if ball is going and on the next position would enter game over position, one player wins the game
				if(ballDirectionX==1){
					score1++;}
				else{
					score2++;}
					
				end = 1;
				}
				
				else if(maze[ballY+ballDirectionY][ballX+ballDirectionX]==3) // if ball is going and on next position hits a player, it will change direction
					{
						ballDirectionY=ballDirectionY*(-1);
						ballDirectionX=ballDirectionX*(-1);
					}
					
				ballX=ballX+ballDirectionX;
				ballY=ballY+ballDirectionY;
				
				xSemaphoreGive(xMutex);
				}
			vTaskDelay(100);  // Task delay
				}
			}
}
				
	
//-----------------------------------------
void refresh_display(void *pvParameters)
{
	for(;;){
			if( xSemaphoreTake( xMutex, 250) ){
			printMaze();
			vTaskDelay(100);
			xSemaphoreGive( xMutex );
		}
	}
}

void startup_task(void *pvParameters)
{
	// The parameters are not used
	( void ) pvParameters;

	#if (configUSE_APPLICATION_TASK_TAG == 1)
	// Set task no to be used for tracing with R2R-Network
	vTaskSetApplicationTaskTag( NULL, ( void * ) 1 );
	#endif
	
	_x_com_received_chars_queue = xQueueCreate( _COM_RX_QUEUE_LENGTH, ( unsigned portBASE_TYPE ) sizeof( uint8_t ) );
	init_com(_x_com_received_chars_queue);
	xMutex = xSemaphoreCreateMutex();
	innitMaze();
	// Initialization of tasks etc. can be done here
	BaseType_t t1 = xTaskCreate(connection, (const char *)"Connection", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY+1, NULL);
	BaseType_t t2 = xTaskCreate(move_player_and_ball, (const char *)"Move", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY+3, NULL);
	BaseType_t t3 = xTaskCreate(refresh_display, (const char *)"Move", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY+2, NULL);

	
	// Lets send a start message to the console
	//com_send_bytes((uint8_t *)"Then we Start!\n", 15);
	
	while(1)
	{
		vTaskDelay( 1000 );
	}
}

// Prepare shift register setting SER = 1
void prepare_shiftregister()
{
	// Set SER to 1
	PORTD |= _BV(PORTD2);
}

// clock shift-register
void clock_shift_register_and_prepare_for_next_col()
{
	// one SCK pulse
	PORTD |= _BV(PORTD5);
	PORTD &= ~_BV(PORTD5);
	
	// one RCK pulse
	PORTD |= _BV(PORTD4);
	PORTD &= ~_BV(PORTD4);
	
	// Set SER to 0 - for next column
	PORTD &= ~_BV(PORTD2);
}

// Load column value for column to show
void load_col_value(uint16_t col_value)
{
	PORTA = ~(col_value & 0xFF);
	
	// Manipulate only with PB0 and PB1
	PORTB |= 0x03;
	PORTB &= ~((col_value >> 8) & 0x03);
}

//-----------------------------------------
void handle_display(void)
{
	static uint8_t col = 0;
	
	if (col == 0)
	{
		prepare_shiftregister();
	}
	
	load_col_value(frame_buf[col]);
	
	clock_shift_register_and_prepare_for_next_col();
	
	// count column up - prepare for next
	col++;
	if (col > 13)
	{
		col = 0;
	}
}

//----------------------------------------- FUNCTIONS
//Game 


void innitMaze()
{
	for (int i = 0; i < 10; i++)
	for (int j = 0; j < 14; j++) {
		maze[i][j] = 0;
		if(j==0 || j==13)
			maze[i][j] = 2; //left and right 'walls' that the ball needs to reach to end the game
		if (i == 0 || i ==9)
			maze[i][j] = 1; //top and bottom borders
	}
}

int pow( int base, int exponent)

{   // Does not work for negative exponents. (But that would be leaving the range of int)
	if (exponent == 0) return 1;  // base case;
	int temp = pow(base, exponent/2);
	if (exponent % 2 == 0)
	return temp * temp;
	else
	return (base * temp * temp);
}

void printMaze()  // PRINTS MAZE  /////////////////////////////////////////////////////////////////////////////////////
{

	for (int j = 0; j < 14; j++) {
		int dec = 0;
		int power = 0;
		for (int i = 0; i < 10; i++)
		{
			if (maze[i][j] == 1 || (j==0 && player1Y == i) || (j==0 && player1Y+1 == i) || (j==0 && player1Y+2 == i) || (j==13 && player2Y == i) || (j==13 && player2Y+1 == i) || (j==13 && player2Y+2 == i) || (j==ballX && ballY== i)) {
				dec += pow(2, power);
			}
			if(j==0 || j==13){
				maze[i][j]	=2;
				if((j==0 && player1Y == i) || (j==0 && player1Y+1 == i) || (j==0 && player1Y+2 == i) || (j==13 && player2Y == i) || (j==13 && player2Y+1 == i) || (j==13 && player2Y+2 == i)){
					maze[i][j]	=3;
				}
			}
			
				power++;
		}
		frame_buf[j]=dec;
	}
}


void vApplicationIdleHook( void )
{
	//
}

//-----------------------------------------
int main(void)
{
	init_board();
	
	// Shift register Enable output (G=0)
	PORTD &= ~_BV(PORTD6);
	
	//creates tasks
	BaseType_t t1 = xTaskCreate(startup_task, (const char *)"Startup", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL); // This task starts the other tasks as well. I guess.

	// Start the display handler timer
	init_display_timer(handle_display);
	
	
	sei();
	//Start the scheduler
	vTaskStartScheduler();
	//Should never reach here
	for(;;);
}