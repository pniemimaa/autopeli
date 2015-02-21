#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <string.h>


//Funktioiden esittelyt
void tarkista_napit(void);
void alusta(void);
void piirra_naytto(void);

#define LEVEYS (2)
#define PITUUS (16)

char naytto [LEVEYS] [PITUUS]= {
	{' ','_',' ','_',' ','_',' ','_',' ','<'},
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}

};

int nakyy = 0;


int main(void)
{

	/* alusta laitteen komponentit */
	alusta();

	while (1) {
		if (nakyy)
		{
			lcd_write_ctrl(LCD_CLEAR);
			nakyy = 0;
		}
		//Disabloidaan interruptit
//		SREG &= ~(1 << 7);
		lcd_write_ctrl(LCD_CLEAR);
		piirra_naytto();
		
//		tarkista_napit();
	}
}
void piirra_naytto()
{
	int x,y;
	for (y=0;y<2;y++)
	for (x=0;x<16;x++)
	{
		lcd_gotoxy(x,y);
		lcd_write_data((char)('0'+x));
		
	}
	/*
	for (x=0;x<LEVEYS;x++)
	{
		//lcd_gotoxy(0,x);
		for (y=0;y<PITUUS;y++)
			lcd_write_data(naytto [x][y]);
		
	}*/

}
void alusta(void) {

	/* estetään kaikki keskeytykset */
	cli();

	/* kaiutin pinnit ulostuloksi */
	DDRE  |=  (1 << PE4) | (1 << PE5);
	/* pinni PE4 nollataan */
	PORTE &= ~(1 << PE4);
	/* pinni PE5 asetetaan */
	PORTE |=  (1 << PE5);

	/* ajastin nollautuu, kun sen ja OCR1A rekisterin arvot ovat samat */
	TCCR1A &= ~( (1 << WGM11) | (1 << WGM10) );
	TCCR1B |=    (1 << WGM12);
	TCCR1B &=   ~(1 << WGM13);

	/* salli keskeytys, jos ajastimen ja OCR1A rekisterin arvot ovat samat */
	TIMSK |= (1 << OCIE1A);

	/* asetetaan OCR1A rekisterin arvoksi 0x3e (~250hz) */
	OCR1AH = 0x00;
	OCR1AL = 0x13;
	/* OCR1AL = 0x3e; */

	/* käynnistä ajastin ja käytä kellotaajuutena (16 000 000 / 1024) Hz */
	TCCR1B |= (1 << CS12) | (1 << CS10);

	/* näppäin pinnit sisääntuloksi */
	DDRA &= ~(1 << PA0);
	DDRA &= ~(1 << PA2);
	DDRA &= ~(1 << PA4);

	/* rele/led pinni ulostuloksi */
	DDRA |= (1 << PA6);

	/* lcd-näytön alustaminen */
	lcd_init();
	lcd_write_ctrl(LCD_ON);
	lcd_write_ctrl(LCD_CLEAR);
	

}

void tarkista_napit()
{
	unsigned int masked;
	char buf [40];
	const int taulu[] = { 1,2,4,8,16 }; /* five lowest bits in PINA */

	// Zero everything above 5 lowest bits
	masked = PINA & 31;
	// Invert the bits (so that presses become 1:s...)
	masked = ~(masked);

	int i,count=0;
	for (i=0;i<5;i++)
	{
		if (taulu[i] & masked)
		{
			count++;
		}
	}
	buf[6] = '\0';
	if (count > 0)
	{
		/* Sallitaan keskeytykset */
		SREG |= (1 << 7);
		// rele päälle
		PORTA |= (1 << PA6);
		nakyy=1;
		i=7;
		lcd_write_ctrl(LCD_CLEAR);
		while (i > -1)
		{
			char a = '0'+ (1 & (masked >> i));
			lcd_write_data(a);
			i--;
		}
		lcd_write_data(' ');
		lcd_write_data(' ');
	}
	else
		PORTA &= ~(1 << PA6);
}

ISR(TIMER1_COMPA_vect) {

	/* vaihdetaan kaiutin pinnien tilat XOR operaatiolla */
	PORTE ^= (1 << PE4) | (1 << PE5);
}

