#include "o3.h"
#include "gpio.h"
#include "systick.h"
#include <stdio.h>

//Globale tellevariabler
int sec = 0;
int min = 0;
int hour = 0;


typedef struct {
	 volatile word CTRL;
	 volatile word MODEL;
	 volatile word MODEH;
	 volatile word DOUT;
	 volatile word DOUTSET;
	 volatile word DOUTCLR;
	 volatile word DOUTTGL;
	 volatile word DIN;
	 volatile word PINLOCKN;
} gpio_port_map_t;

	typedef struct {
		volatile gpio_port_map_t ports[6];
		volatile word unused_space[10];
		volatile word EXTIPSELL;
		volatile word EXTIPSELH;
		volatile word EXTIRISE;
		volatile word EXTIFALL;
		volatile word IEN;
		volatile word IF;
		volatile word IFS;
		volatile word IFC;
		volatile word ROUTE;
		volatile word INSENSE;
		volatile word LOCK;
		volatile word CTRL;
		volatile word CMD;
		volatile word EM4WUEN;
		volatile word EM4WUPOL;
		volatile word EM4WUCAUSE;
	} gpio_map_t;

typedef struct {
	volatile word CTRL;
	volatile word LOAD;
	volatile word VAL;
	volatile word CALIB;
} systick_t;

//Definerer hvor vi skal finne LED_ og BUTTON_PORT.
#define LED_PORT GPIO_PORT_E
#define BUTTON_PORT GPIO_PORT_B

//Lager de forskjellige tilstandene klokken kan være i.
#define SET_SECONDS 0
#define SET_MINUTES 1
#define SET_HOURS 2
#define COUNTDOWN 3
#define ALARM 4
int state = SET_SECONDS;


volatile gpio_map_t* gpio_map;
volatile systick_t* systick;

//Skrur på LED
void turnOnLED() {
	gpio_map->ports[LED_PORT].DOUTSET = 0b0100;
}
//Skrur av LED
void turnOffLED() {
	gpio_map->ports[LED_PORT].DOUTCLR = 0b0100;
}

//Setter opp GPIO-registrene vi skal bruke.
//Dvs, begge knappene PB0 og PB1, og led-lyset LED0
void setUpGPIO() {
	gpio_map = (gpio_map_t*) GPIO_BASE;

	//Oppsett av LED0
	int mask = 0b1111;
	mask = mask << 8;
	mask = ~mask;
	word model = gpio_map->ports[LED_PORT].MODEL;
	model = model & mask;
	mask = GPIO_MODE_OUTPUT; //Setter LEDen til å være en output
	mask = mask << 8;
	model = model | mask;
	gpio_map->ports[LED_PORT].MODEL = model;

	//Oppsett av PB0
	gpio_map->ports[BUTTON_PORT].DOUT = 0;
	gpio_map->ports[BUTTON_PORT].MODEH = ((~(0b1111<<4))&gpio_map->ports[BUTTON_PORT].MODEH)|(GPIO_MODE_INPUT<<4); //Setter knappen til å være input
	gpio_map->EXTIPSELH = ((~(0b1111<<4))&gpio_map->EXTIPSELH)|(0b0001<<4);
	gpio_map->EXTIFALL = gpio_map->EXTIFALL|(1<<9);
	gpio_map->IFC = gpio_map->IFC|(1<<9);
	gpio_map->IEN = gpio_map->IEN|(1<<9);

	//Oppsett av PB1
	gpio_map->ports[BUTTON_PORT].MODEH = ((~(0b1111<<8))&gpio_map->ports[BUTTON_PORT].MODEH)|(GPIO_MODE_INPUT<<8); //Setter knappen til å være input.
	gpio_map->EXTIPSELH = ((~(0b1111<<8))&gpio_map->EXTIPSELH)|(0b0001<<8);
	gpio_map->EXTIFALL = gpio_map->EXTIFALL|(1<<10);
	gpio_map->IFC = gpio_map->IFC|(1<<10);
	gpio_map->IEN = gpio_map->IEN|(1<<10);



}

//Øker sekunder
void increaseSeconds() {
	if (sec == 59) {
		sec = 59;
	} else {
		sec = sec + 1;
	}
}

//Øker minutter
void increaseMinutes() {
	if (min == 59) {
		min = 59;
	} else {
		min = min + 1;
	}
}

//Øker timer
void increaseHours() {
	hour = hour + 1;
}

//Setter opp SysTick registeret
void setUpSysTick() {
	systick = (systick_t*) SYSTICK_BASE;

	systick = (systick_t*) SYSTICK_BASE;
    int mask = (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk);
    systick->CTRL = mask;
    systick->LOAD = FREQUENCY;
}

//Oppdaterer lcd-skjermen
void update_lcd() {
	char timestamp[7];
	time_to_string(timestamp, hour, min, sec);
	lcd_write(timestamp);
}


void alarm() {
	turnOnLED();
}

//Starter klokken
void start() {
	systick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

//Sier hva som skal skje når når PB1 blir trykket på
//PB1 er knappen som fungerer som en OK-knapp.
void GPIO_EVEN_IRQHandler() {
	switch (state) {
	//Når PB1 blir trykket på når vi er i SET_HOURS skal nedtelllingen starte
	case SET_HOURS:
		state = COUNTDOWN;
		start();
		break;
	//Hvis PB1 blir trykket på mens vi teller ned, så skjer det ingenting.
	case COUNTDOWN:
		break;
	//Når vi trykker på PB1 etter at alarmen har gått, nullstiller vi klokken tilbake til SET_SECONDS
	case ALARM:
		state = SET_SECONDS;
		turnOffLED();
		break;
	//Hvis man er i en tilstand som er ingen av de over, bare økes tilstanden slik at man kommer til neste tilstand.
	default:
		state = state + 1;
	};
	gpio_map->IFC = 1<<10;
}


//Bestemmer hva som skjer når PB0 trykkes på.
//PB0 brukes til å øke sekundene/minuttene/timene
void GPIO_ODD_IRQHandler() {
	switch (state) {
	case SET_SECONDS:
		increaseSeconds();
		break;
	case SET_MINUTES:
		increaseMinutes();
		break;
	case SET_HOURS:
		increaseHours();
		break;
	};
	gpio_map->IFC = 1<<9;
}

//Teller ned tiden.
void countdown() {
	sec = sec - 1;
	if (hour == 0 && min == 0 && sec == 0) { //Forteller hva som skjer når nedtellingen har nådd 0.
		state = ALARM;
		alarm();
		return;
	}
	if (sec == -1) {
		min = min - 1;
		sec = 59;
		if (min == -1) {
			hour = hour - 1;
				min = 59;
			}
			}
}

//Forteller hva som skjer for hver klokkesykel
//I dette tilfellet kjøres et kall til countdown(), slik at ett sekund blir telt ned.
void SysTick_Handler() {
	switch (state) {
	case COUNTDOWN:
		countdown();
	};
}



/**************************************************************************//**
 * @brief Konverterer nummer til string 
 * Konverterer et nummer mellom 0 og 99 til string
 *****************************************************************************/
void int_to_string(char *timestamp, unsigned int offset, int i) {
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
	    if (i >= 10) {
		    i -= 10;
		    timestamp[offset]++;
		
	    } else {
		    timestamp[offset+1] = '0' + i;
		    i=0;
	    }
    }
}

/**************************************************************************//**
 * @brief Konverterer 3 tall til en timestamp-string
 * timestamp-argumentet mÃ¥ vÃ¦re et array med plass til (minst) 7 elementer.
 * Det kan deklareres i funksjonen som kaller som "char timestamp[7];"
 * Kallet blir dermed:
 * char timestamp[7];
 * time_to_string(timestamp, h, m, s);
 *****************************************************************************/
void time_to_string(char *timestamp, int h, int m, int s) {
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, h);
    int_to_string(timestamp, 2, m);
    int_to_string(timestamp, 4, s);
}

int main(void) {
    init();
    setUpGPIO();
    setUpSysTick();

    //En while-løkke som kjører konstant, og som kontant oppdaterer lcd-skjermen.
    while (1) {
    	update_lcd();
    }

return 0;



	}






