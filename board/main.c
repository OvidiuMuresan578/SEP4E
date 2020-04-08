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
static SemaphoreHandle_t  xMutex1 = NULL;


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
static int score1 ; // score for p1
static int score2 ; // --//--//--p2

static int ballX ;
static int ballY ;
static int ballDirectionX ; //can be 1 or -1 going right or left
static int ballDirectionY ; //can be 1 or -1 going down or up

//static int speed;

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
		if( xSemaphoreTake( xMutex1, 250) ){
			result = xQueueReceive(_x_com_received_chars_queue, &byte, 1000L); //this is what it receives
			xSemaphoreGive(xMutex1);}
		if (result) { //if there is a result
			//simple check if what was received corresponds to known values, do a thing. 
			if(byte=='s')
				if (player2Y != 0)
					player2Y = player2Y - 1;
			if(byte=='w')
				if (player2Y != 8)
					player2Y = player2Y + 1;

		vTaskDelay(200);
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
			vTaskDelay(1000);  // Delay at the start of the game to get players ready
				innitMaze();
				player1Y = 4;
			    player2Y = 4;
			    end = 0;
				
				//randomly place the ball somewhere in the middle
				if(random(0,100)<50)
					ballX = 7;
				else 
					ballX = 8;
					
				if(random(0,100)<50)
					ballY = 5;
				else 
					ballY = 6;
			    
			    
				
			    //ballDirectionX = 1; //can be 1 or -1 going right or left
			    //ballDirectionY = -1 //can be 1 or -1 going down or up
				if(random(0,100)<50)
					ballDirectionX=1;
				else 
					ballDirectionX=-1;
					
				if(random(0,100)<50)
					ballDirectionY=1;
				else 
					ballDirectionY=-1;
				
		 //boolean for ending the game
		//From here on the game starts.
		while (end == 0) {
			if(score2>=5){//can set end condition here
				
				if( xSemaphoreTake( xMutex, 250) ){
					if( xSemaphoreTake( xMutex1, 250) ){
						com_send_bytes((uint8_t*)"PLAYER2 WINS", 12);//send back to 2
						xSemaphoreGive(xMutex1);
					}
					setEndGameScreen(2);
					printMaze();
					xSemaphoreGive(xMutex);
				}
				//while(1);//ends the game
				vTaskDelay(5000);
				score1=0;
				score2=0;
				end=1;
				
			}
			if(score1>=5){//can set end condition here
				
				if( xSemaphoreTake( xMutex, 250) ){
					if( xSemaphoreTake( xMutex1, 250) ){
					com_send_bytes((uint8_t*)"PLAYER1 WINS", 12);//send back to 2
					xSemaphoreGive(xMutex1);
					}
					setEndGameScreen(1);
					printMaze();
					xSemaphoreGive(xMutex);
				}
				//while(1);//ends the game
				vTaskDelay(5000);
				score1=0;
				score2=0;
				end=1;
			}
			
			int PinCValue = PINC & 0b11000011 ; // Masking PINC
			//Moving the character
				
			if(PinCValue==up)
				if (player1Y != 0){
						player1Y = player1Y - 1;}
					//	player2Y = player2Y - 1;} //temporary until PC connectivity is implemented
					
			if(PinCValue==down)
				if (player1Y != 8){
						player1Y = player1Y + 1;}
					//	player2Y = player2Y + 1;} //temporary until PC connectivity is implemented

			//Move ball				int maze[10][14];
				if(ballY+ballDirectionY==0 && ballX+ballDirectionX==0 || ballY+ballDirectionY==0 && ballX+ballDirectionX==14 || ballY+ballDirectionY==10 && ballX+ballDirectionX==0 || ballY+ballDirectionY==10 && ballX+ballDirectionX==14 ){//corner bug fixed here
					ballDirectionY=ballDirectionY*(-1);
					ballDirectionX=ballDirectionX*(-1);
				}
				else if(maze[ballY+ballDirectionY][ballX+ballDirectionX]==1) //if ball is going and on the next position would enter the same pixel as a wall, it will change direction
					ballDirectionY=ballDirectionY*(-1);
				
				//else if(maze[ballX+ballDirectionX][ballY+ballDirectionY]==2)
				else if(ballX==0 || ballX==13)
				{							 //if ball is going and on the next position would enter game over position, one player wins the game
				if(ballDirectionX==-1){
					score2++;
					if( xSemaphoreTake( xMutex1, 250) ){
						com_send_bytes((uint8_t*)"2", 12);
						xSemaphoreGive(xMutex1);}
					
					}
				else{
					score1++;
					if( xSemaphoreTake( xMutex1, 250) ){
						com_send_bytes((uint8_t*)"1", 12);
						xSemaphoreGive(xMutex1);}
					
					}
					
				end = 1;
				}
				
				else if(maze[ballY+ballDirectionY][ballX+ballDirectionX]==3) // if ball is going and on next position hits a player, it will change direction
					{
						ballDirectionX=ballDirectionX*(-1);
					}
					
				ballX=ballX+ballDirectionX;
				ballY=ballY+ballDirectionY;
				
			vTaskDelay(200);  // Task delay
				}
			}
}
				
	
//-----------------------------------------
void refresh_display(void *pvParameters)
{
	//#if (configUSE_APPLICATION_TASK_TAG == 1)
	// Set task no to be used for tracing with R2R-Network
	//vTaskSetApplicationTaskTag( NULL, ( void * ) 1 );
	//#endif
	
	for(;;){
			if( xSemaphoreTake( xMutex, 250) ){
			printMaze();
			vTaskDelay(100); //interval of time at which the maze is printed. This needs to be long enough for the other tasks to execute. 
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
	xMutex1 = xSemaphoreCreateMutex();
	
	// Initialization of tasks etc. can be done here
	BaseType_t t1 = xTaskCreate(connection, (const char *)"Connection", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY+3, NULL);
	BaseType_t t2 = xTaskCreate(move_player_and_ball, (const char *)"Move", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY+3, NULL); // game logic
	BaseType_t t3 = xTaskCreate(refresh_display, (const char *)"Refresh", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY+1, NULL);
 
	
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

void setEndGameScreen(int player){
	for(int i=0;i<10;i++)
		for(int j=0;j<14;j++){
			maze[i][j]=0;
		}
		ballX=4;
		ballY=13;
		
	if(player==1){//shows appropriate number on display
			maze[4][13]=1;
			maze[4][12]=1;
			maze[4][11]=1;
			maze[4][10]=1;
			maze[4][9]=1;
			maze[4][8]=1;
			maze[3][12]=1;
			maze[2][11]=1;
			maze[2][8]=1;
			maze[3][8]=1;
			maze[5][8]=1;
			maze[6][8]=1;
			}
	if(player==2){
			maze[3][13]=1;
			maze[4][13]=1;
			maze[5][13]=1;
			maze[2][12]=1;
			maze[6][12]=1;
			maze[2][11]=1;
			maze[6][11]=1;
			maze[6][10]=1;
			maze[4][9]=1;
			maze[5][9]=1;
			maze[3][8]=1;
			maze[2][7]=1;
			maze[3][7]=1;
			maze[4][7]=1;
			maze[5][7]=1;
			maze[6][7]=1;
	}
	//displays GG
	maze[0][5]=1;
	maze[1][5]=1;
	maze[2][5]=1;
	maze[3][5]=1;
	maze[5][5]=1;
	maze[6][5]=1;
	maze[7][5]=1;
	maze[8][5]=1;
	maze[0][4]=1;
	maze[5][4]=1;
	maze[0][3]=1;
	maze[2][3]=1;
	maze[3][3]=1;
	maze[5][3]=1;
	maze[7][3]=1;
	maze[8][3]=1;
	maze[0][2]=1;
	maze[3][2]=1;
	maze[5][2]=1;
	maze[8][2]=1;
	maze[0][1]=1;
	maze[1][1]=1;
	maze[2][1]=1;
	maze[3][1]=1;
	maze[5][1]=1;
	maze[6][1]=1;
	maze[7][1]=1;
	maze[8][1]=1;
	
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

int random(int lower, int upper)
{
	int i;
	int num = (rand() %(upper - lower +1)) + lower;
	return num;

}

void printMaze()  // PRINTS MAZE  /////////////////////////////////////////////////////////////////////////////////////
{

	for (int j = 0; j < 14; j++) {
		int dec = 0;
		int power = 0;
		for (int i = 0; i < 10; i++)
		{
			if (maze[i][j] == 1  || maze[i][j] == 3 || (j==ballX && ballY== i)) {
				//(j==0 && player1Y == i) || (j==0 && player1Y+1 == i) || (j==0 && player1Y+1 == i) || (j==13 && player2Y == i) || (j==13 && player2Y+1 == i) || (j==13 && player2Y+1 == i)
				dec += pow(2, power);
			}
			if(j==0 || j==13){
				maze[i][j]	=2;
				if((j==0 && player1Y == i) || (j==0 && player1Y+1 == i) || (j==0 && player1Y+1 == i) || (j==13 && player2Y == i) || (j==13 && player2Y+1 == i) || (j==13 && player2Y+1 == i)){
					maze[i][j]	=3;
				}
			}
			
				power++;
		}
		frame_buf[j]=dec; //j is a column, and on the column j, each bit with value 1 is lit. That's why I need to transform the normal array into bit values and this is what dec holds: the bit value for that column 
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