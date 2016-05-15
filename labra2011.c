/********************************************************/
/* Tommi Kilponen 										*/
/* Jaakko Vartiainen		         			        */
/* Laiteläheinen ohjelmointi, Harjoitustyö, kevät 2012 	*/
/********************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"


void kirjoita_nayttoon(char start, int pituus, char *tieto);

char valikko();

void peli();

int anna_panos(int *saldo_ptr);

void luo_pakka(char *pakka);

void sekoita_pakka(char *pakka);

void jaa_kortti(char *pakka, char *pelaaja, int *x_ptr, int *y_ptr);

char kysy_korttia();

int korttien_summa(char *pelaaja, int pelaaja_indeksi);

char peli_valikko();

char voittaja_selville(int p_summa, int j_summa, int pelaaja_indeksi);

void init(void) {

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
        OCR1AL = 0x15;

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



int main(void) 
{
	/* alusta laitteen komponentit */
	init();

	unsigned char valinta;

    while (1){
		lcd_write_ctrl(LCD_CLEAR);
		valinta = valikko();
		if(valinta == 0){
			peli();
		}
		_delay_ms(1000);
	}
}


ISR(TIMER1_COMPA_vect) {

		/* vaihdetaan kaiutin pinnien tilat XOR operaatiolla */
 		PORTE ^= (1 << PE4) | (1 << PE5); 
		}
		

/********************************************/
/*				aliohjelmat					*/
/********************************************/




void kirjoita_nayttoon(char start, int pituus, char *tieto){
	lcd_write_ctrl(LCD_DDRAM | start);
	for(int i = 0; i < pituus; i++){
		lcd_write_data(tieto[i]);
	}
}

char valikko(){
	unsigned char paivita_ruutu = 0, valinta = 0;
	
	kirjoita_nayttoon(0x00, 9, "BLACKJACK");
	kirjoita_nayttoon(0x40,8,"New game");
	while(1){
		if(!(PINA & (1 <<PA0))){
			if(valinta > 0) valinta --;
			else valinta = 2;
			paivita_ruutu = 1;
			_delay_ms(300);
		}
		
		if(!(PINA & (1 <<PA4))){
			if(valinta < 2) valinta ++;
			else valinta = 0;
			paivita_ruutu = 1;
			_delay_ms(300);
		}
		
		if(!(PINA & (1 <<PA2))){
			switch(valinta){
				case 0: 
					return 0;
					break;
				default:
					return 0;
					break;
			}
			_delay_ms(300);
		}
		
		if(paivita_ruutu){
			lcd_write_ctrl(LCD_CLEAR);
			kirjoita_nayttoon(0x00, 9, "BLACKJACK");
			switch(valinta){
				case 0:
					kirjoita_nayttoon(0x40,8,"New game");
					break;
				default:
					break;
			}
			paivita_ruutu = 0;
		}
		
	}
}




void peli(){
	int saldo = 100;
			
	while(saldo > 0){
		char pakka[53] = { 0 } , pelaaja[16] = { 0 }, jakaja[16] = { 0 };
		int pelaaja_indeksi, jakaja_indeksi, x = 0, y = 0, p_summa, j_summa;
		int panos = 0
		
		panos = anna_panos(&saldo);
		luo_pakka(pakka);
		sekoita_pakka(pakka);
		lcd_write_ctrl(LCD_CLEAR);
		
		
		for(int i = 0; i < 2; i++){ 
			jaa_kortti(pakka, pelaaja, &x, &y);
			kirjoita_nayttoon(0x00,i+1,pelaaja);
			_delay_ms(500);
		}
						
		while(1){			
			if(kysy_korttia() == 0){
				jaa_kortti(pakka, pelaaja, &x, &y);
				kirjoita_nayttoon(0x00,y,pelaaja);
				_delay_ms(500);
				if(korttien_summa(pelaaja,y)>21){
					break;
				}
			}
			else break;
		}
			
		p_summa = korttien_summa(pelaaja,y);
		
		if(p_summa > 21){
			lcd_write_ctrl(LCD_CLEAR);
			char rivi[13] = "Dealer wins.";
			kirjoita_nayttoon(0x00, 12, rivi);
			_delay_ms(1000);
			continue;	
		}
							
		pelaaja_indeksi = y;
		y = 0;
		for(int i = 0; i<2; i++){ 
			kirjoita_nayttoon(0x40,15,"               ");
			jaa_kortti(pakka, jakaja, &x, &y);
			kirjoita_nayttoon(0x40,y,jakaja);
			_delay_ms(500);
		}
				
		while(korttien_summa(jakaja,y) <= 16){
			jaa_kortti(pakka, jakaja, &x, &y);
			kirjoita_nayttoon(0x40,y,jakaja);
			_delay_ms(500);
		}
				
		j_summa = korttien_summa(jakaja,y);
		jakaja_indeksi = y;
		
		_delay_ms(3000);
		switch(voittaja_selville(p_summa,j_summa,pelaaja_indeksi)){
			case 0:
				saldo += 2*panos;
				break;
			case 1:
				saldo += panos + 1.5*panos;
				break;
			case 2:
				saldo += panos;
				break;
			case 3:
				break;
		}
		if(peli_valikko() == 0) continue;
		else break;
		
	}
}


int anna_panos(int *saldo_ptr){
	int saldo = *saldo_ptr;
	int panos = 0;
	char buffer[20];
	unsigned char paivita_ruutu = 1;
	int x = 0;
	
	lcd_write_ctrl(LCD_CLEAR);
	kirjoita_nayttoon(0x00,15,"Place your bet:");
	_delay_ms(1000);
	
	while(1){
		if(paivita_ruutu == 1){ 
			lcd_write_ctrl(LCD_CLEAR);
			paivita_ruutu = 0;
		}

		char rivi1[16] = "Points:";
		sprintf(buffer, "%d", saldo); 
		strcat(rivi1, buffer);
		kirjoita_nayttoon(0x00, strlen(rivi1), rivi1);

		char rivi2[16] = "Bet:";
		sprintf(buffer, "%d", panos);
		strcat(rivi2, buffer);
		kirjoita_nayttoon(0x40, strlen(rivi2), rivi2);

		if(!(PINA & (1 <<PA0))){
			if(x == 0){
				if(saldo > 0){
					panos = panos + 5;
					saldo = saldo - 5;
					paivita_ruutu = 1;
					x = 1; 
				}
			}
		}
		
		else if((!(PINA & (1 <<PA2)))){
			*saldo_ptr = saldo;
			return panos;
		}

		else if((!(PINA & (1 <<PA4)))){
			if(x == 0){
				if(panos > 0){
					panos = panos - 5;
					saldo = saldo + 5;
					paivita_ruutu = 1;
					x = 1;
				}
		}
		
	}
	else x = 0;
	}
}


void luo_pakka(char *pakka){				
	char kortit[53] = "23456789TJQKA23456789TJQKA23456789TJQKA23456789TJQKA";
	strncpy(pakka,kortit,52);
	pakka[52]='\0';
}
									
void sekoita_pakka(char *pakka) { /* http://www.fredosaurus.com/notes-cpp/misc/random-shuffle.html */
	srand(TCNT1);
	for (int i=0; i<51; i++) {
    	int r = i + (rand() % (52-i)); /* Random remaining position. */
    	int temp = pakka[i]; pakka[i] = pakka[r]; pakka[r] = temp;
	}
}

void jaa_kortti(char *pakka, char *pelaaja, int *x_ptr, int *y_ptr){
	int x = *x_ptr;
	int y = *y_ptr;
	pelaaja[y] = pakka[x];
	x++;
	y++;
	*x_ptr = x;
	*y_ptr = y;
}

char kysy_korttia(){
	unsigned char paivita_ruutu = 0, valinta = 0;

	kirjoita_nayttoon(0x40, 15, "More cards? Yes");
	while(1){
		if(!(PINA & (1 <<PA0))){
			if(valinta > 0) valinta --;
			else valinta = 1;
			paivita_ruutu = 1;
			_delay_ms(300);
		}
		
		if(!(PINA & (1 <<PA4))){
			if(valinta < 1) valinta ++;
			else valinta = 0;
			paivita_ruutu = 1;
			_delay_ms(300);
		}
		
		if(!(PINA & (1 <<PA2))){
			switch(valinta){
				case 0: 
					return 0;
					break;
				case 1:
					return 1;
					continue;
			}
			_delay_ms(300);
		}
		
		if(paivita_ruutu){
			switch(valinta){
				case 0:
					kirjoita_nayttoon(0x40,15,"More cards? Yes");
					break;
				case 1:
					kirjoita_nayttoon(0x40,15,"More cards? No ");
					break;
			}
			paivita_ruutu = 0;
		}
		
	}
	
}



int korttien_summa(char *pelaaja, int pelaaja_indeksi){
	int summa = 0, i;
	char luku[2] = { '0', '\0' } ;
	
	for(i = 0; i < pelaaja_indeksi; i++){
		if(pelaaja[i] == 'T'){
			summa += 10;
		}
		if(pelaaja[i] == 'J'){
			summa += 10;
		}
		if(pelaaja[i] == 'Q'){
			summa += 10;
		}
		if(pelaaja[i] == 'K'){
			summa += 10;
		}
		if(pelaaja[i] == 'A'){
			if(i == 0)
				summa += 11;
			else 
				summa += 1;
		}
		else{
			luku[0] = (char)(pelaaja[i]);
			summa += atoi(luku);
		}
	}
	return summa;
}



char peli_valikko(){
	unsigned char paivita_ruutu = 0, valinta = 0;

	lcd_write_ctrl(LCD_CLEAR);
	kirjoita_nayttoon(0x00, 13, "Keep Playing?");
	kirjoita_nayttoon(0x40,8,"Continue");
	while(1){
		if(!(PINA & (1 <<PA0))){
			if(valinta > 0) valinta --;
			else valinta = 1;
			paivita_ruutu = 1;
			_delay_ms(300);
		}
		
		if(!(PINA & (1 <<PA4))){
			if(valinta < 1) valinta ++;
			else valinta = 0;
			paivita_ruutu = 1;
			_delay_ms(300);
		}
		
		if(!(PINA & (1 <<PA2))){
			switch(valinta){
				case 0: 
					return 0;
					break;
				case 1:
					return 1;
					continue;
			}
			_delay_ms(300);
		}
		
		if(paivita_ruutu){
			lcd_write_ctrl(LCD_CLEAR);
			kirjoita_nayttoon(0x00, 13, "Keep Playing?");
			switch(valinta){
				case 0:
					kirjoita_nayttoon(0x40,8,"Continue");
					break;
				case 1:
					kirjoita_nayttoon(0x40,4,"Quit");
					break;
			}
			paivita_ruutu = 0;
		}
		
	}
}


char voittaja_selville(int p_summa, int j_summa, int pelaaja_indeksi){
	unsigned char x;
	char rivi[16];
	
	if(p_summa <= 21){
		if(p_summa < j_summa){
			if(j_summa > 21){
				x = 0;
			}
			else{
				x = 3;		
			}
		}
		if(p_summa == j_summa){
			x = 2;
		}
		if(pelaaja_indeksi == 2 && p_summa == 21){
			x = 1;
		}
	}
	else{
		x = 3;
	}

	lcd_write_ctrl(LCD_CLEAR);	
	switch(x){
			/*Normaali voitto*/
			case 0: 
				strcpy(rivi,"You win.");
				kirjoita_nayttoon(0x00, 8, rivi);
				_delay_ms(1000);
				return 0;
				break;
			/*Blackjack*/	
			case 1:
				strcpy(rivi,"You win.");
				kirjoita_nayttoon(0x00, 8, rivi);
				_delay_ms(1000);
				return 1;
				break;
			/*Tasapeli*/
			case 2:
				strcpy(rivi,"It's a tie.");
				kirjoita_nayttoon(0x00, 11, rivi);
				_delay_ms(1000);
				return 2;
				break;
			/*Jakaja voittaa*/
			case 3:
				strcpy(rivi,"Dealer wins.");
				kirjoita_nayttoon(0x00, 12, rivi);
				_delay_ms(1000);
				return 3;
				break;
	}

}
