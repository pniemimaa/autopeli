#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>


//Funktioiden esittelyt
void tarkista_napit(void);
void alusta(void);
void piirra_naytto(void);
int tarkista_osuma(void);
void vierita_nayttoa(void);
void liikuta_autoa(int);
void tayta_tie(void);
void resetoi_peliarvot(void);


//Makrot
#define TRUE       1
#define FALSE      0
#define NAPPIMAARA 4
#define LEVEYS     (2)
#define PITUUS     (16)

#define B2_NAPPI_YLHAALLA (PINA & (1 << PA1))

char naytto [LEVEYS] [PITUUS]= {
		{' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_'},
		{' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_',' ','_'}

};


typedef struct autopositio {
	int kaista;
	int kohta;
} autopositio;

enum ETyyppi {
	oikea = 0,
	vasen,
	molemmat };

typedef struct Este {
	int kaista;
	int kohta;
	enum ETyyppi tyyppi;} Este;

	autopositio ap = {0,(PITUUS-1)};
	int nakyy = FALSE;
	volatile int matka = 0;
	volatile int este=0;
	volatile Este e2 = {0xFF,0xFF,0xFF};
	int peli_paalla = FALSE;



	int main(void)
	{

		/* alusta laitteen komponentit */
		alusta();
		/* Enabloi interruptit */
		sei();
		peli_paalla = TRUE;

		while (1) {
#if 0
			lcd_gotoxy(0,0);
			lcd_write_string("Alkuteksti");
#endif
		
			if (peli_paalla == FALSE)
			{
			
				// Resetoi peli kun nappia B2 painetaan
				while (1)
				{
					if ( ! (B2_NAPPI_YLHAALLA) )
					{
						PORTA &= ~(1<<PA6);
						resetoi_peliarvot();
						break;
					}
				}
			}
			else
			{
			// Hae satunnaissiemenluku timer countterista
			srand(TCNT1H <<8 | TCNT1L);

			// Ledi pois päältä
			PORTA &= ~(1 << PA6);
			//Tyhjennä näyttö
			lcd_write_ctrl(LCD_CLEAR);
			tarkista_napit();
			piirra_naytto();
			peli_paalla=tarkista_osuma();
			_delay_ms(1000);
			}

		}
	}

	/* Funktiot */
    void resetoi_peliarvot(void)
	{
		ap.kaista = 0;
		ap.kohta = (PITUUS-1);
		nakyy = FALSE;
		matka = 0;
		este=0;
		e2.kaista = 0xFF;
		e2.kohta = 0xFF;
		e2.tyyppi = 0xFF;
		peli_paalla = TRUE;
		lcd_write_ctrl(LCD_CLEAR);	
		sei();
	}

	int tarkista_osuma()
	{
		int osuma = FALSE;
		char ajettu[16];
		if (e2.kohta == ap.kohta)
		{
			if (e2.tyyppi == molemmat)
				osuma = TRUE;
			else if (e2.kaista == ap.kaista)
				osuma = TRUE;
		}

		if (osuma)
		{
			//Disabloi interruptit
			cli();
			//sytytä Ledi
			PORTA |= (1<<PA6);
			//Näytön tyhjennys ja lopputuloksen kirjoitus
			lcd_write_ctrl(LCD_CLEAR);
			lcd_gotoxy(0,0);
			lcd_write_string("Peli loppui...");
			lcd_gotoxy(0,1);
			sprintf(ajettu,"Matka: %d",matka);
			lcd_write_string(ajettu);
			
			return FALSE;
			
		}
		return TRUE;
	}

	void vierita_nayttoa()
	{

		if (e2.kohta < 16)
		{
			naytto [e2.kaista] [e2.kohta] = ' ';
			if (e2.tyyppi == molemmat)
				naytto [e2.kaista+1] [e2.kohta] = ' ';
			e2.kohta++;
		}
		else
		{
			e2.kohta = 0xFF;
		}
	}

	void piirra_naytto()
	{
		int x,y;
		switch (e2.tyyppi)
		{
		case oikea:
			e2.kaista = 0;
			//o
			break;
		case vasen:
			//v
			e2.kaista = 1;
			break;
		case molemmat:
			//kummatkin
			e2.kaista = 0;
			if (e2.kohta < 16)
				naytto [1] [e2.kohta] =  '*';
			break;
		}
		if (e2.kohta < 16)
			naytto [e2.kaista] [e2.kohta] =  '*';
		//naytto [ap.kaista] [ap.kohta] = '<';
		naytto [ap.kaista] [ap.kohta] = 0;
		for (y=0;y<2;y++)
			for (x=0;x<16;x++)
			{
				lcd_gotoxy(x,y);
				lcd_write_data(naytto[y][x]);

			}


	}

	void liikuta_autoa(int i)
	{
		//tyhjää ed paikka
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
			ap.kohta = ( ((ap.kohta+1) <= (PITUUS-1) ) ? (ap.kohta+1) : ap.kohta);
			break;
		case 3:
			ap.kaista = (ap.kaista ? 1 : (ap.kaista+1));
			break;
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

		/* asetetaan OCR1A rekisterin arvoksi 0x3d09 (Keskeytys kerran sekunnissa) */
		OCR1AH = 0x3d;
		OCR1AL = 0x09;


		/* käynnistä ajastin ja käytä kellotaajuutena (16 000 000 / 1024) Hz */
		TCCR1B |= (1 << CS12) | (1 << CS10);

		/* näppäin pinnit sisääntuloksi */
		DDRA &= ~(1 << PA0);
		DDRA &= ~(1 << PA2);
		DDRA &= ~(1 << PA3);
		DDRA &= ~(1 << PA4);

		// Nappi B2 sisääntuloksi
		DDRA &= ~(1 << PA1);

		/* rele/led pinni ulostuloksi */
		DDRA |= (1 << PA6);

		/* lcd-näytön alustaminen */
		lcd_init();
		lcd_write_ctrl(LCD_ON);
		lcd_write_ctrl(LCD_CLEAR);

	   /* Talleta kaksi eri auton grafiikkaa CGRAM positioon 0 ja 1*/
		const char auto_grafiikat [2] [8] = {
				{0,0,9,31,31,9,0,0},
				{0,0,17,14,14,17,0,0} };
		int i,j;
		for (i=0;i<2;i++)
		{
			lcd_write_ctrl(0x40 + (8*i));
			for (j=0;j<8;j++)
			{
				lcd_write_data(auto_grafiikat[i][j]);
			}
		}
	}

	void tarkista_napit()
	{
		unsigned int masked;
		const int taulu[] = { 1,4,8,16 }; /* 0001 0101 */

		// Zero everything above 5 lowest bits
		masked = PINA & 31;
		// Invert the bits (so that presses become 1:s...)
		masked = ~(masked);

		int i;
		for (i=0;i<NAPPIMAARA;i++)
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
					//Hyppää
					PORTA |= (1 << PA6);
					liikuta_autoa(i);
					break;
				case 2:
					//Taaksepäin
					PORTA |= (1 << PA6);
					liikuta_autoa(i);
					break;
				case 3:
					//Vasemmalle kaistalle
					PORTA |= (1 << PA6);
					liikuta_autoa(i);
					break;
				}
			}
		}

	}

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
	}
	/* Keskeytysrutiinit */

	ISR(TIMER1_COMPA_vect) {

		vierita_nayttoa();
		tayta_tie();
		matka++;
		este++;

		if ((matka % 16)==0)
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
