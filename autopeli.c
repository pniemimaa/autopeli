#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>


//Funktioiden esittelyt
void tarkista_napit(void);
void alusta(void);
void piirra_naytto(void);

#define LEVEYS (2)
#define PITUUS (16)

char naytto [LEVEYS] [PITUUS]= {
	{' ','_',' ','_',' ','_',' ','_',' ',' ',' ','_',' ','_',' ','<'},
	{' ','_',' ','_',' ','_',' ','_',' ',' ',' ','_',' ','_',' ','_'}

};


typedef struct autopositio {
	int kaista;
	int kohta;
} autopositio;

autopositio ap = {0,15};
int nakyy = 0;


int main(void)
{
	
	/* alusta laitteen komponentit */
	alusta();
	
	while (1) {
		
		PORTA &= ~(1 << PA6);
		lcd_write_ctrl(LCD_CLEAR);
		tarkista_napit();
		piirra_naytto();
		_delay_ms(1000);
		
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
		lcd_write_data(naytto[y][x]);
		
	}
	

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
	const int taulu[] = { 1,4,16 }; /* 0001 0101 */

	// Zero everything above 5 lowest bits
	masked = PINA & 31;
	// Invert the bits (so that presses become 1:s...)
	masked = ~(masked);

	int i;
	for (i=0;i<3;i++)
	{
		if (taulu[i] & masked)
		{
			switch(i) 
			{
			case 0:
			//Oikealle kaistalle
			PORTA |= (1 << PA6);
			break;
			case 1:
			//keskinappi
			//Hyppää
			PORTA |= (1 << PA6);
			break;
			case 2:
			//Vasemmalle kaistalle
			PORTA |= (1 << PA6);
			break;
			}	
		}
	}

}

ISR(TIMER1_COMPA_vect) {

	/* vaihdetaan kaiutin pinnien tilat XOR operaatiolla */
	PORTE ^= (1 << PE4) | (1 << PE5);
}

