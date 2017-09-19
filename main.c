#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* All functions after main should be initialized here */
char inchar(void);
void outchar(char x);
void shiftout(char);
void lcdwait(void);
void send_byte(char);
void send_i(char);
void chgline(char);
void print_c(char);
void pmsglcd(char[]);
void fdisp(void);
void converted_distance(void);
void go_ahead(int speed);   //speed should be in the range of 0 - 255
void turn_right(void);
void turn_left(void);
void stop(void);



/* Variable declarations */
int distance_l = 0;
int distance_fl = 0;
int distance_f = 0;
int distance_fr = 0;
int distance_r = 0;
int distance_l_prv = 0;
int distance_fl_prv = 0;
int distance_f_prv = 0;
int distance_fr_prv = 0;
int distance_r_prv = 0;
int forward = 1;
int ct = 0;
int sum = 0;
int sens_l = 1;   //1 means can go to such dir, 0 means road blocked
int sens_fl = 1;
int sens_f = 1;
int sens_fr =1;
int sens_r =1;
int turning = 0; //0 means not currently turning


/* Special ASCII characters */
#define CR 0x0D		// ASCII return�
#define LF 0x0A		// ASCII new line�

/* LCD COMMUNICATION BIT MASKS (note - different than previous labs) */
#define RS 0x10		// RS pin mask (PTT[4])
#define RW 0x20		// R/W pin mask (PTT[5])
#define LCDCLK 0x40	// LCD EN/CLK pin mask (PTT[6])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1  0x80	// LCD line 1 cursor position
#define LINE2  0xC0	// LCD line 2 cursor position


/*
***********************************************************************
 Initializations
***********************************************************************
*/

void  initializations(void) {

/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; //; disengage PLL from system
  PLLCTL = PLLCTL | 0x40; //; turn on PLL
  SYNR = 0x02;            //; set PLL multiplier
  REFDV = 0;              //; set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; //; engage PLL

/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, interrupts off initially */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port

/* Initialize peripherals */
  ATDCTL2 = 0x80;
  ATDCTL3 = 0x20;
  ATDCTL4 = 0x85;

  TSCR1 = 0x80;
  TIOS = 0x80;
  TSCR2 = 0x0F;
  TC7 = 1500;
  TIE = 0x80;


	DDRT = 0xFF;
  DDRM = 0x30;
  DDRAD = 0;
  ATDDIEN = 0xC0;

  SPICR1 = 0x50;
  SPICR2 = 0x00;
  SPIBR = 0x01;

  PWME = 0x03;
  PWMPOL = 0x03;
  MODRR = 0x03;
  PWMCTL = 0x00;
  PWMCAE = 0x00;
  PWMPER0 = 0xFF;
  PWMPER1 = 0xFF;
  PWMDTY0 = 0x00;
  PWMDTY1 = 0x00;
  PWMCLK = 0x03;
  PWMPRCLK = 0x01;



/* Initialize interrupts */
  PTT_PTT4 = 1;
  PTT_PTT3 = 0;
  send_i(LCDON);
  send_i(TWOLINE);
  send_i(LCDCLR);
  lcdwait();


}


/*
***********************************************************************
Main
***********************************************************************
*/

void main(void) {
  DisableInterrupts
	initializations();
	EnableInterrupts;

 //pmsglcd("Distance:");

 //Initial State go ahead 255
 for(;;) {
     //Sensor Status
     sens_l = 1;
     sens_fl = 1;
     sens_f = 1;
     sens_fr = 1;
     sens_r = 1;
     //Detect wall and toggle sensor's flag
     if((distance_fl <= distance_fl_prv && distance_fl <= 20)) {
          sens_fl = 0;
    }

    if((distance_f <= distance_f_prv && distance_f <= 20)) {
          sens_f = 0;
    }

     if((distance_fr <= distance_fr_prv && distance_fr <= 20)) {
          sens_fr = 0;
    }

    if((distance_r <= distance_r_prv && distance_r <= 20)) {
          sens_r = 0;
    }

    if((distance_l <= distance_l_prv && distance_l <= 20)) {
          sens_l = 0;
    }


    //go right
    if (sens_l == 0 && sens_f == 0 || turning == 1){
        turning = 1;
        stop();
        if (sens_fl == 0 || sens_f == 0){   //
            turn_right();
        }
        else {
         turning = 0;
         go_ahead(255);
        }
    }

    //go left
    if (sens_r == 0 && sens_f == 0 || turning == 1){
        turning = 1;
        stop();
        if (sens_fr == 0 || sens_f == 0){   //
            turn_left();
        }
        else {
         turning = 0;
         go_ahead(255);
        }
    }

   } /* loop forever */

}   /* do not leave main */

/*
***********************************************************************   ����  � ������   ��
 RTI interrupt service routine: RTI_ISR
************************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flagt
  	CRGFLG = CRGFLG | 0x80;
}

/*
***********************************************************************   ����  � ������   ��
  TIM interrupt service routine
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
  // clear TIM CH 7 interrupt flag
 	TFLG1 = TFLG1 | 0x80;
  ATDCTL5 = 0x10;
  while (!ATDSTAT0_SCF) {  }
  converted_distance();

  distance_l_prv = distance_l;
  distance_fl_prv = distance_fl;
  distance_f_prv = distance_f;
  distance_fr_prv = distance_fr;
  distance_r_prv = distance_r;

  ct++;
  sum += forward;

  if((distance_fl >= 20) && (distance_f >= 20) &&  (distance_fr >= 20))   {   //move forward if enough space is given
      forward = 1;
  }
  else {
      forward = 0;
  }

  if(ct == 10) {
      if(sum >= 5) {
          forward = 1;
          go_ahead(255);
      } else{
          forward = 0;
          stop();
      }
      outchar(forward + 48);
      fdisp();
      ct = 0;
      sum = 0;
  }

}

//Direction helper functions
void turn_left() {
    PWMDTY0 = 255;
    PWMDTY1 = 0;
}

void turn_right() {
    PWMDTY1 = 255;
    PWMDTY0 = 0;
}

void stop() {
    PWMDTY0 = 0;
    PWMDTY1 = 0;
}

void go_ahead(int speed) {
    PWMDTY1 = speed;
    PWMDTY0 = speed;
}

void converted_distance() {
      //Left Sensor
      if(ATDDR0H <= 26){
          distance_l = 70;
      }
      else if (ATDDR0H >= 158){
          distance_l = 10;
      }
      else{
          distance_l = (180-ATDDR0H) / 2;
      }

      //Front Left Sensor
      if(ATDDR1H <= 26){
          distance_fl = 70;
      }
      else if (ATDDR1H >= 158){
          distance_fl = 10;
      }
      else{
          distance_fl = (180-ATDDR1H) / 2;
      }

      //Front Sensor
      if(ATDDR2H <= 26){
          distance_f = 70;
      }
      else if (ATDDR2H >= 158){
          distance_f = 10;
      }
      else{
          distance_f = (180-ATDDR2H) / 2;
      }

      //Front Right Sensor
      if(ATDDR3H <= 26){
          distance_fr = 70;
      }
      else if (ATDDR3H >= 158){
          distance_fr = 10;
      }
      else{
          distance_fr = (180-ATDDR3H) / 2;
      }

      //Right
      if(ATDDR4H <= 26){
          distance_r = 70;
      }
      else if (ATDDR4H >= 158){
          distance_r = 10;
      }
      else{
          distance_r = (180-ATDDR4H) / 2;
      }
}
