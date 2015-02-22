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
	{' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_'},
	{' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_'}

};


typedef struct autopositio {
	int kaista;
	int kohta;
} autopositio;

typedef struct Este {
	int kaista;
	int kohta;
	int tyyppi;} Este;

autopositio ap = {0,15};
int nakyy = 0;
int matka = 0;
volatile int este=0;
volatile Este e2 = {0xFF,0xFF,0xFF};

void tayta_tie(void)
{
	char a='_';
	char b=' ';
	int y,x;

	if (matka % 2)
	{
		a=' ';
		b='_';
	}
	for (y=0;y<LEVEYS;y++)
	for (x=0;x<PITUUS;x++)
	{
		naytto [y][x] = ((x % 2) ? b : a); 
	}
	matka++;
}

int main(void)
{
	
	/* alusta laitteen komponentit */
	alusta();
	sei();

	while (1) {
	srand(TCNT1H <<8 | TCNT1L);
		tayta_tie();
		PORTA &= ~(1 << PA6);
		lcd_write_ctrl(LCD_CLEAR);
		vierita_nayttoa();
		tarkista_napit();
		piirra_naytto();
		_delay_ms(1000);
		
	}
}

void vierita_nayttoa()
{
	
	if (e2.kohta < 15)
	{
		naytto [e2.kaista] [e2.kohta] = ' ';
		e2.kohta++;
	}
	else 
	{
		e2.kohta = 0xFF;
	}
	if (ap.kohta < 15)
	{
		naytto [ap.kaista] [ap.kohta] = ' ';
		ap.kohta++;
	}
}

void piirra_naytto()
{
	int x,y;
	switch (e2.tyyppi)
	{
		case 0:
		e2.kaista = 0;
		//o
		break;
		case 1:
		//v
		e2.kaista = 1;
		break;
		case 2:
		//kummatkin
		e2.kaista = 0;
		naytto [1] [e2.kohta] =  '*';
		break;
	}
	if (e2.kohta < 16)
	naytto [e2.kaista] [e2.kohta] =  '*';
	naytto [ap.kaista] [ap.kohta] = '<';
	for (y=0;y<2;y++)
	for (x=0;x<16;x++)
	{
		lcd_gotoxy(x,y);
		lcd_write_data(naytto[y][x]);
		
	}
	

}

void liikuta_autoa(int i)
{
	//tyhj�� ed paikka
	naytto [ap.kaista] [ap.kohta] = ' ';
	switch(i)
	{
		case 0:
		ap.kaista = (ap.kaista ? (ap.kaista-1) : 0);
		break;
		case 1:
		ap.kohta = ( ((ap.kohta-2) >= 0) ? (ap.kohta-2) : 0);
		break;
		case 2:
		ap.kaista = (ap.kaista ? 1 : (ap.kaista+1));
		break;
	}
}

void alusta(void) {

	/* estet��n kaikki keskeytykset */
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

	/* asetetaan OCR1A rekisterin arvoksi 0x3d09 (Keskeytys kerran sekunnissa) */
	OCR1AH = 0x3d;
	OCR1AL = 0x09;
	

	/* k�ynnist� ajastin ja k�yt� kellotaajuutena (16 000 000 / 1024) Hz */
	TCCR1B |= (1 << CS12) | (1 << CS10);

	/* n�pp�in pinnit sis��ntuloksi */
	DDRA &= ~(1 << PA0);
	DDRA &= ~(1 << PA2);
	DDRA &= ~(1 << PA4);

	/* rele/led pinni ulostuloksi */
	DDRA |= (1 << PA6);

	/* lcd-n�yt�n alustaminen */
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
			liikuta_autoa(i);
			break;
			case 1:
			//keskinappi
			//Hypp��
			PORTA |= (1 << PA6);
			liikuta_autoa(i);
			break;
			case 2:
			//Vasemmalle kaistalle
			PORTA |= (1 << PA6);
			liikuta_autoa(i);
			break;
			}	
		}
	}

}

ISR(TIMER1_COMPA_vect) {
	
	este++;
	
	if (este == 8)
	{
		este =0;
		e2.kaista = 0;
		e2.kohta = 0;
		e2.tyyppi = rand()%3; 
		PORTA |= (1 << PA6);
	}
	/* vaihdetaan kaiutin pinnien tilat XOR operaatiolla */
	PORTE ^= (1 << PE4) | (1 << PE5);
}

