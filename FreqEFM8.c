// FreqEFM8.c: Measure the frequency of a signal on pin T0.
  
#include <EFM8LB1.h>
#include <stdio.h>
#include "EFM8_LCD_4bit.h"

#define SYSCLK      72000000L  // SYSCLK frequency in Hz
#define BAUDRATE      115200L  // Baud rate of UART in bps

unsigned char overflow_count;
char buff[17];
char cap[16];
char freq[16];

void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0101; // Timer/Counter 0 used as a 16-bit counter
	TR0=0; // Stop Timer/Counter 0
}

double calculate_capacitance(double f){
	return ( (1.44) / (3 * 10000 * f) );
}

void main (void) 
{
	unsigned long F;
	unsigned long C;
	
	TIMER0_Init();

	// Configure the LCD
	LCD_4BIT();

	waitms(500); // Give PuTTY a chance to start.
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.

	printf ("EFM8 Frequency measurement using Timer/Counter 0.\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);

	LCDprint("test", 1, 1, 0);
	LCDprint("Frequency:", 1, 1, 1);
	LCDprint("Capacitance:", 2, 1, 1);

	while(1)
	{
		TL0=0;
		TH0=0;
		overflow_count=0;
		TF0=0;
		TR0=1; // Start Timer/Counter 0
		waitms(1000);
		TR0=0; // Stop Timer/Counter 0
		F=overflow_count*0x10000L+TH0*0x100L+TL0;
		C=calculate_capacitance(F);
		printf("\rf=%luHz", C);

		sprintf(cap, "%lf", C);
		sprintf(freq, "%lf", F);
		LCDprint(freq, 1, 2, 1);
		LCDprint(cap, 2, 2, 1);
		printf("\x1b[0K"); // ANSI: Clear from cursor to end of line.
	}
	
}
