#include <avr/io.h>
#include "timer.h"

unsigned char gametime = 0x00;
unsigned char SCORE = 0x00;
int correct = 0;
int gameOn = 0;
int userREADY = 0;
int checkScoreREADY = 0;
int checkDone = 0;
const unsigned short post_time = 1250; // 1250
unsigned short solve_time [4] = {6250, 5000, 3000, 2000};


#define col1_r 0x7FFF
#define col2_r 0xBFFF
#define col3_r 0xDFFF
#define col4_r 0xEFFF
#define col5_r 0xF7FF
#define col6_r 0xFBFF
#define col7_r 0xFDFF
#define col8_r 0xFEFF

void transmit_data(unsigned short data){
	int i;
	for (i = 0; i < 16 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTC = 0x08;
		// set SER = next bit of data to be sent.
		PORTC |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTC |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTC |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTC = 0x00;
}

void adc_init(){
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 16000000/128 = 125000
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

unsigned short readadc(uint8_t ch)
{
	ch&=0b00000111;         //ANDing to limit input to 7
	ADMUX = (ADMUX & 0xf8)|ch;  //Clear last 3 bits of ADMUX, OR with ch
	ADCSRA|=(1<<ADSC);        //START CONVERSION
	while((ADCSRA)&(1<<ADSC));    //WAIT UNTIL CONVERSION IS COMPLETE
	return(ADC);        //RETURN ADC VALUE
}

unsigned char game_pic[4][8] = {{0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00},
{0x00, 0x10, 0x18, 0x08, 0x0C, 0x00, 0x00, 0x00},
{0x00, 0x2A, 0x54, 0x2A, 0x54, 0x2A, 0x54, 0x00},
{0x00, 0x10, 0x24,0x20, 0x20, 0x24, 0x10, 0x00}};
unsigned char user_pic[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//unsigned char PORT1 [8] = {col1_r, col2_r, col3_r, col4_r, col5_r, col6_r, col7_r, col8_r};

int i = 0;
int curr_pic = 0;
int flag = 1;

unsigned char tempB = 0x00;
enum Check_Score {CS_Start, CS_Comp, CS_Reset} CS_state;
void Check_Score_SM(){
	switch(CS_state){
		case CS_Start:
			if(checkScoreREADY){
				CS_state = CS_Comp;
			}
		break;
		case CS_Comp:
			CS_state = CS_Reset;
			
		break;
		case CS_Reset:
			CS_state = CS_Start;
		break;
	}
	
	switch(CS_state){
		case CS_Start:
		break;
		case CS_Comp:
			for(int j = 0; j < 8; j++){
				if(user_pic[j] == game_pic[curr_pic][j] && flag){
					flag = 1;
				}
				else{flag = 0;}
			}
			if(flag){
				SCORE = SCORE + 5;
				tempB = 0x04;
				correct = 1;
			}
			else{
				tempB = 0x08;
				flag = 1; //reset
				correct = 0;
			}
		break;
		case CS_Reset:
			checkScoreREADY = 0;
			checkDone = 1;
			tempB = 0x00;
		break;
	}
	PORTB = (tempB & 0x0C) | PORTB;
};

enum Matrix {M_Start, M_Array1, M_UserTurn, M_Stop, M_Next}M_state;
unsigned char m_wait = 0x00;


unsigned char column_val = 0x00; // sets the pattern displayed on columns (rows)
unsigned short column_sel = 0x7FFF; // grounds column to display pattern  (column)
unsigned short M_elapsedtime = 0x00;
unsigned char dot_row = 0x80;
unsigned short dot_column = col8_r;

enum Joystick_states {J_Start, J_User, J_Wait}J_state;

int up = 0x00;
int down = 0x00;
int right = 0x00;
int left = 0x00;
unsigned short udV = 0x00;
unsigned short lrV = 0x00;

void joystick(){
	udV = readadc(7);
	lrV = readadc(6);
	switch(J_state){
		case J_Start:
			if(userREADY){
				J_state = J_User;
			}
		break;
		case J_User:
			if(checkScoreREADY){
				J_state = J_Start;
				userREADY = 0;
			}
		break;
		case J_Wait:
			if(udV < 700 && udV > 500 && lrV < 700 && lrV > 500 ){
				J_state = J_User;
			}
		break;
	}
	switch(J_state){
		case J_Start:
		
		break;
		case J_User:
			if ( lrV < 500 && dot_row != 0x80){    // joystick down in game
				dot_row = dot_row << 1;
				J_state = J_Wait;
			}
			else if ( lrV > 700 && dot_row != 0x01 ){   // joystick going up
				dot_row = dot_row >> 1;
				J_state = J_Wait;
			}
			else if ( udV < 700 && udV > 500 && lrV < 700 && lrV > 500 ){ //joystick is in center
			}
			else if ( udV > 700 && dot_column != 0x7FFF){    // joystick left
				dot_column = dot_column << 1 | 0x0001;
				J_state = J_Wait;
			}
			else if ( udV < 500 && dot_column != 0xFEFF) {   //joystick right
				dot_column = dot_column >> 1 | 0x8000;
				J_state = J_Wait;
			}
			//column_sel = dot_column;
			//column_val = dot_row;
		break;
		
		case J_Wait:
		break;
	}
	
}

unsigned char butt_sel = 0x00;
unsigned short chill = 0x0000;
int butsel_wait = 0;
void M_Tick(){
	
	switch(M_state){
		case M_Start:
			if(gameOn){
				M_state = M_Array1;
			}
		break;
		case M_Array1:
			if(M_elapsedtime >= post_time){
				M_elapsedtime = 0;
				//dot_row = 0x80;
				//dot_column = col8_r;
				column_sel = 0x7FFF;
				M_state = M_UserTurn;
				userREADY = 1;
				i = -1;		
				}
		break;
		case M_UserTurn:
			if(M_elapsedtime >= solve_time[curr_pic]){
				M_state = M_Stop;
				i = 0;
				checkScoreREADY = 1;
			}
		break;
		case M_Stop:
			if(checkDone && chill >= post_time ){
				M_state = M_Next;
				checkDone = 0;
			}
			chill++;
		break;
		case M_Next:
			if(correct){
				M_state = M_Start;
				if(curr_pic < 5){
					curr_pic++;
				}
				//ADD WHEN GAME IS WON
			}
			else{
				M_state = M_Start;
				gameOn = 0;
				curr_pic = 0;
			}
			
		break;
		
	}
	switch(M_state){
		case M_Start:
			if(gameOn){
				column_val = game_pic[curr_pic][i];
				column_sel = 0x7FFF;
			}
			M_elapsedtime = 0;
		break;
		case M_Array1:
			if(i < 8 && column_sel != 0xFEFF && M_elapsedtime < post_time){
				i++;
				column_val = game_pic[curr_pic][i];
				column_sel = (column_sel >> 1) | 0x8000;
			}
			else if ((i >= 8 || column_sel == 0xFEFF) && M_elapsedtime < post_time){
				i = 0;
				column_sel = col1_r;
				column_val = game_pic[curr_pic][i];
			}
			else{};
			M_elapsedtime++;
		break;
		case M_UserTurn:
			butt_sel = ~PINA & 0x02;
			if(butt_sel){
				if(!butsel_wait && column_sel == dot_column){
					user_pic[i] =  user_pic[i]| dot_row;
					butsel_wait = 1;
				}
			}
			else{butsel_wait = 0;}
			if(i < 8 && column_sel != 0xFEFF && M_elapsedtime < solve_time[curr_pic]){
				i++;
				column_sel = (column_sel >> 1) | 0x8000;
				if(column_sel == dot_column){
					column_val = user_pic[i] | dot_row;
				}
				else {column_val = user_pic[i];}
			}
			else if ((i >= 8 || column_sel == col8_r) && M_elapsedtime < solve_time[curr_pic]){
				column_sel = col1_r;
				i = 0;
				if(column_sel == dot_column){
					column_val = user_pic[i] | dot_row;
				}
				else {column_val = user_pic[i];}
			}
			M_elapsedtime++;
			
		break;
		case M_Stop:
			column_val = 0x00;
			column_sel = 0x00;
		break;
		case M_Next:
			for (int z = 0; z < 8; z++)
			{
				user_pic[z] = 0x00;
			}
			dot_column = 0xFEFF;
			dot_row = 0x80;
			chill = 0;
		break;
		
	}
	PORTD = column_val;
	transmit_data(column_sel);
};

enum Game_Play {GP_Start, GP_Init, GP_On, GP_Wait} GP_state;
void GP_Tick(){
	unsigned char button_on = 0x00;
	switch(GP_state){
		case GP_Start:
			GP_state = GP_Init;
		break;
		case GP_Init:
			button_on  = ~PINA & 0x01;
			if(button_on && !gameOn){
				GP_state = GP_On;
			}
			else if(button_on && gameOn){
				
			}
		break;
		case GP_On:
			if(gameOn && !button_on){
				GP_state = GP_Wait;
			}
		break;
		case GP_Wait:
			GP_state = GP_Start;
			
		break;
	}
	switch(GP_state){
		case GP_Start:
		break;
		case GP_Init:
		break;
		case GP_On:
			//call function to print words : FIX ME
			gameOn = 1;
		break;
		case GP_Wait:
		break;
	}
	
};

int main(void)
{
    DDRA = 0x00;
    PORTA = 0xFF;
    DDRC = 0xFF;
    PORTC = 0x00;
    DDRD = 0xFF;
    PORTD = 0x00;
    DDRB = 0xFF;
    PORTB = 0x00;
	
	TimerSet(4);//4
	TimerOn();
	adc_init();
	GP_state = GP_Start;
	M_state = M_Start;
	
	
    while (1) 
    {
		GP_Tick();
		M_Tick();
		joystick();
		Check_Score_SM();
		
		
		while(!TimerFlag);
		TimerFlag = 0;
		
    }
}
