/*
Temperature Controller
ECE411 Practicum Project
Team 9:
 Dai Ho, Dawit Amare, Erik Fox, Surya Ravikumar
 
Description: This program/ firmware controlls temperature inddors. It has User mode and Auto mode.
User mode is activated with a toggle button. Once user mode is activated, user can set desired temperature outside the
comfy zone: 75 +/- 3.
In Auto Mode, system controlls temperature with an intent to keep temp in the comfy zone. It controlls fan or heater accordingly
 */
 
//#define F_CPU   8000000
 
#include <avr/io.h>
//#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define BIT_IS_SET(byte, bit)(byte & (1 << bit))
#define BIT_IS_CLEAR(byte, bit)(!(byte &(1<<bit)))
#define WAIT_MS  10// Time in ms to wait for debounce.

#define control_bus PORTD
#define controlbus_direction DDRD
#define data_bus PORTD
#define databus_direction DDRD

#define rs 2
#define en 3
#define d4 4
#define d5 5
#define d6 6
#define d7 7

char buff[16];

uint16_t temp()
{
	uint8_t low, high;

	ADMUX &= (0 << ADLAR) & (0 << REFS1);
	ADMUX &= (0 << MUX3) & (0 << MUX1) & (0 << MUX1) & (0 << MUX0);
	ADMUX |= (1 << REFS0);// | ( 1 << MUX2) | (1 << MUX0);

	ADCSRA &= (0 << ADIF) & (0 << ADIE);
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | ( 1 << ADPS0);
	ADCSRA |= (1 << ADEN);
	ADCSRA |= (1 << ADSC);
	
	_delay_ms(1000);

	low  = ADCL;
	high = ADCH;
	
	return (high << 8) | low;
}

void LCD_CmdWrite( char a)
{
	if(a & 0x80) data_bus|=(1<<d7); else data_bus&= ~(1<<d7);
	if(a & 0x40) data_bus|=(1<<d6); else data_bus&= ~(1<<d6);
	if(a & 0x20) data_bus|=(1<<d5); else data_bus&= ~(1<<d5);
	if(a & 0x10) data_bus|=(1<<d4); else data_bus&= ~(1<<d4);
	
	control_bus &=~(1<<rs);control_bus |=(1<<en);

	_delay_ms(2);

	control_bus &=~(1<<en);

	_delay_ms(2);	

	if(a & 0x08) data_bus|=(1<<d7); else data_bus&= ~(1<<d7);
	if(a & 0x04) data_bus|=(1<<d6); else data_bus&= ~(1<<d6);
	if(a & 0x02) data_bus|=(1<<d5); else data_bus&= ~(1<<d5);
	if(a & 0x01) data_bus|=(1<<d4); else data_bus&= ~(1<<d4);

	control_bus &=~(1<<rs);control_bus |=(1<<en);

	_delay_ms(2);

	control_bus &=~(1<<en);	

	_delay_ms(2);
}

void LCD_DataWrite( char a)
{
	if(a & 0x80) data_bus|=(1<<d7); else data_bus&= ~(1<<d7);
	if(a & 0x40) data_bus|=(1<<d6); else data_bus&= ~(1<<d6);
	if(a & 0x20) data_bus|=(1<<d5); else data_bus&= ~(1<<d5);
	if(a & 0x10) data_bus|=(1<<d4); else data_bus&= ~(1<<d4);

	control_bus |=(1<<rs)|(1<<en);

	_delay_ms(2);

	control_bus &=~(1<<en);

	_delay_ms(2);

	if(a & 0x08) data_bus|=(1<<d7); else data_bus&= ~(1<<d7);
	if(a & 0x04) data_bus|=(1<<d6); else data_bus&= ~(1<<d6);
	if(a & 0x02) data_bus|=(1<<d5); else data_bus&= ~(1<<d5);
	if(a & 0x01) data_bus|=(1<<d4); else data_bus&= ~(1<<d4);

	control_bus |=(1<<rs)|(1<<en);

	_delay_ms(2);

	control_bus &=~(1<<en);

	_delay_ms(2);
}

void LCD_Init()
{
	controlbus_direction |= ((1<<rs)|(1<<en));

	databus_direction |= ((1<<d7)|(1<<d6)|(1<<d5)|(1<<d4));

	_delay_ms(2);

	LCD_CmdWrite(0x01); // clear display
	LCD_CmdWrite(0x02); // back to home
	LCD_CmdWrite(0x28); // 4bit,2line,5x7 pixel
	LCD_CmdWrite(0x06); // entry mode,cursor increments by cursor shift
	LCD_CmdWrite(0x0c); // display ON,cursor OFF
	LCD_CmdWrite(0x80); // force cursor to begin at line1
}

void LCD_Disp(const char *p)
{
	while(*p!='\0')
	{
		LCD_DataWrite(*p);
		p++; _delay_ms(2);
	}
}

void LCD_setCursor(int a, int b)
{
	int i=0;

	switch(b)
	{
		case 0:LCD_CmdWrite(0x80);break;

		case 1:LCD_CmdWrite(0xC0);break;

		case 2:LCD_CmdWrite(0x94);break;

		case 3:LCD_CmdWrite(0xd4);break;
		}

	for(i=0;i<a;i++)
	    LCD_CmdWrite(0x14);
}

void fanOn()
{
	LCD_setCursor(7,0);
	LCD_Disp("fan");
	PORTB &= 0 << PINB0;								// heat off;
	//PORTB = 0xFF;
	PORTB |= 1 << PINB1;								// fan on blu light on
	//PORTD |= 1 << PIND6;								// Display on
	//PORTB |= 1 << PINB2;								// heat on red light on
	return;
}

void heatOn()
{	
	LCD_setCursor(6,0);
	LCD_Disp("heat");
	//PORTB |= 1 << PINB2;								// heat on red light on
	PORTB &= 0 << PINB1;								// fan off;
	//PORTD |= 1 << PIND6;								// Display on
	PORTB |= 1 << PINB0;								// heat on red light on
	return;
}

 
void control (int setPoint, int room_temp)
{
	room_temp = (((temp()/1024.0 * 5 ) - 0.5 ) * 100);
	room_temp = (room_temp * 9 / 5) + 32;
	
	if ((room_temp <= setPoint+3)&&(room_temp >=setPoint-3))
	{
		_delay_ms(50); 
		// Display Temp 
		PORTB &= 0 << PINB1;								// fan off;
		PORTB &= 0 << PINB2;								// heat on red light on
		
	}
	else if ( room_temp > setPoint + 3)
	{
		fanOn();
	}
	else if ( room_temp < setPoint - 3)
	{
		heatOn();
	}
}


void potSvc ()
{
	/* Set ADMUX register -- use to pick ref Voltage and ADC pin*/
	ADMUX = (1 << REFS0) | (1<<ADLAR)|(1 << MUX0) | (1 << MUX2);
	/* Set ADSCRA register leave ADSCRB register zero -- resolution*/
	ADCSRA = (1 << ADEN) | (0 << ADATE) | (1 << ADPS0) | (1 << ADPS1) | ( 1 << ADPS2);
	ADCSRB = 0x00;
	
}

int main(void)
{
	
	DDRD = 0xFE; //(1 << PORTD);							// Output LED on PD6 -- LCD indicator
	DDRB = (1 << PORTB1);								// Output LED on PD6 -- FAN indicator
	DDRB = (1 << PORTB2);								// Output LED on PD6 -- Heat indicator
	
	int cur = PIND;
	int prev = PIND;
	int toggle = 1;
	float potValue = 0;
	int current_temp = 0;
	int setTo = 0;
		
	LCD_Init();
	
	current_temp = temp();
	current_temp = (((current_temp/1024.0 * 5 ) - 0.5 ) * 100);
	current_temp = (current_temp * 9 / 5) + 32;
	
	_delay_ms(500);
	// Display Temp
	LCD_setCursor(2,0);
	itoa(current_temp, buff, 10);
	LCD_Disp(buff);
	LCD_Disp("F");
	
	while(1)
	{
		_delay_ms(200);
		// Measure Temp-----------------------Put code here

		current_temp = temp();
		current_temp = (((temp()/1024.0 * 5 ) - 0.5 ) * 100);
		current_temp = (current_temp * 9 / 5) + 32;
		
		// Display Temp-----------------------Put code here
		LCD_setCursor(2,0);
		itoa(current_temp, buff, 10);
		LCD_Disp(buff);
		LCD_Disp("F");
		
		// Check for button press-------------Code is below
		cur = PIND;
		if ((prev & (1 << PIND0))<(cur & (1 << PIND0)))
		{
			cur = PIND;
			toggle ^= 1;			// toggle is needed to  switch between Auto and User mode
		}
		prev = PIND;
		_delay_ms(WAIT_MS); 			// Wait for debounce.
			switch (toggle)
			{
			case 0:						// Turn USER on
				
				// Turn Auto OFF------------------|CODE HERE
				// Read POT-----------------------|CODE HERE
				_delay_ms(5000); 
				potSvc();
				ADCSRA |= (1<<ADSC);			// Start Conversion pot input
				while (BIT_IS_SET(ADCSRA, ADSC)) {} 	// poll until ADSC bit is set
				// Quantize potvalue between 50c to 90c
				// Get pot value and display here  
				// Use this pot value to set temp -- GO TO CONTROL
				potValue = ADCH;
				potValue = (potValue/256)*5*100;
                    
               			potValue= (potValue/12.5)+50;
                    
				LCD_setCursor(1, 1);
				itoa(potValue, buff, 10);
				LCD_Disp(buff);																				// capture pot value
			
				// Display POT value--------------|CODE HERE
				// Go to Control with POT Value---|CODE HERE
				// CONTROL-- procedure takes setPoint and decides on temp value
				
				control(potValue, current_temp);
				break;
			case 1:						// Turn AUTO on   
				//PORTB &= 0 << PINB1;
				//PORTD &= 0 << PIND6;
				// Measure room temp
				
				setTo = 75;
				control(setTo, current_temp);
				break;
			}
		 
	 }
  
}
//
