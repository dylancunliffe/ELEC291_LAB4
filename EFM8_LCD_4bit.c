#include <EFM8LB1.h>
#include <stdio.h>

#define SYSCLK    72000000L // SYSCLK frequency in Hz
#define BAUDRATE    115200L // Baud rate of UART in bps

#define LCD_RS P1_7
// #define LCD_RW Px_x // Not used in this code.  Connect to GND
#define LCD_E1 P2_0
#define LCD_E2 P2_1
#define LCD_D4 P1_3
#define LCD_D5 P1_2
#define LCD_D6 P1_1
#define LCD_D7 P1_0
#define CHARS_PER_LINE 16

char _c51_external_startup (void)
{
	// Disable Watchdog with 2-byte key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif

	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X00;
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because ((SYSCLK/BAUDRATE)/(2L*12L))>0xFF
	#endif
	// Configure Uart 0
	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000 ; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
	
	return 0;
}


// Uses Timer3 to delay <us> micro-seconds. 
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON0:
	CKCON0|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN0 = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN0 & 0x80));  // Wait for overflow
		TMR3CN0 &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN0 = 0 ;                   // Stop Timer3 and clear overflow flag
}

void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) Timer3us(250);
}

void LCD1_pulse (void)
{
	LCD_E1=1;
	Timer3us(40);
	LCD_E1=0;
}
void LCD2_pulse (void)
{
	LCD_E2=1;	
	Timer3us(40);
	LCD_E2=0;
}

void LCD_byte (unsigned char x, bit LCD_select)
{
	// The accumulator in the C8051Fxxx is bit addressable!
	ACC=x; //Send high nible
	LCD_D7=ACC_7;
	LCD_D6=ACC_6;
	LCD_D5=ACC_5;
	LCD_D4=ACC_4;
	if(LCD_select == 0)
	{
		LCD1_pulse();
	}
	else
	{
		LCD2_pulse();
	}
	Timer3us(40);
	ACC=x; //Send low nible
	LCD_D7=ACC_3;
	LCD_D6=ACC_2;
	LCD_D5=ACC_1;
	LCD_D4=ACC_0;
	if(LCD_select == 0)
	{
		LCD1_pulse();
	}
	else
	{
		LCD2_pulse();
	}
}

void WriteData (unsigned char x, bit LCD_select)
{
	LCD_RS=1;
	LCD_byte(x, LCD_select);
	waitms(2);
}

void WriteCommand (unsigned char x, bit LCD_select)
{
	LCD_RS=0;
	LCD_byte(x, LCD_select);
	waitms(5);
}

void LCD_4BIT (void)
{
	LCD_E1=0; // Resting state of LCD's enable is zero
	LCD_E2=0;

	// LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33, 0);
	WriteCommand(0x33, 0);
	WriteCommand(0x32, 0); // Change to 4-bit mode
	waitms(20);
	WriteCommand(0x33, 1);
	WriteCommand(0x33, 1);
	WriteCommand(0x32, 1); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28, 0);
	WriteCommand(0x0c, 0);
	WriteCommand(0x01, 0); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
	WriteCommand(0x28, 1);
	WriteCommand(0x0c, 1);
	WriteCommand(0x01, 1); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, bit clear, bit LCD_select)
{
	int j;

	WriteCommand(line==2?0xc0:0x80, LCD_select); // Set the cursor to the beginning of the line
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j], LCD_select);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' ', LCD_select); // Clear the rest of the line
}

int getsn (char * buff, int len)
{
	int j;
	char c;
	
	for(j=0; j<(len-1); j++)
	{
		c=getchar();
		if ( (c=='\n') || (c=='\r') )
		{
			buff[j]=0;
			return j;
		}
		else
		{
			buff[j]=c;
		}
	}
	buff[j]=0;
	return len;
}

void main (void)
{
	char buff[17];
	int line;
	int lcd;
	// Configure the LCD
	LCD_4BIT();
	
   	// Display something in the LCD
	LCDprint("Print w/ Putty", 1, 1, 0);
	while(1)
	{
		printf("What LCD do you want to write to? (1 or 2): ");
		getsn(buff, sizeof(buff));
		lcd=(int)buff[0];
		printf("\n");
		printf("What line do you want to write to? (1 or 2): ");
		getsn(buff, sizeof(buff));
		line=(int)buff[0];
		printf("\n");
		printf("Type what you want to display in line %d (16 char max): ", line);
		getsn(buff, sizeof(buff));
		printf("\n");

		LCDprint(buff, line-48, 1, lcd-49);
		LCDprint(buff, line-48, 1, lcd-49);
	}
}
