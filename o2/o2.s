.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO
.include "sys-tick_constants.s" // Register-adresser og konstanter for SysTick

.text
	.global Start
	

Start:
	//Oppsett av SysTick interrupt
	//Oppsett av CTRL-registert
	LDR R0, =SYSTICK_BASE + SYSTICK_CTRL
	//Sier at det skal utløses et interrupt når telleregisteret har kommet til 0, og at det er core-klokken som skal brukes
	LDR R1, =SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk
	STR R1, [R0] //Lagrer bitene

	//Oppsett av LOAD-registeret
	LDR R1, =SYSTICK_BASE + SYSTICK_LOAD
	//Setter at LOAD skal settes til frekvensen av core-klokken/10 for å kunne telle ned til 0 hvert tidelssekund
	//Altså blir det utløst et interrupt hvert tidelssekund
	LDR R2, = FREQUENCY / 10
	STR R2, [R1] //Lagrer frekvensen til LOAD

	//Oppsett av telleregisteret (VAL-registeret)
	LDR R3, =SYSTICK_BASE + SYSTICK_VAL
	//Setter telleregisteret til å telle fra frekvens/10 slik at første interrupt kommer etter et tienedelssekund
	STR R2, [R3]

	//Interrupt-oppsett for GPIO

	LDR R2, =GPIO_EXTIPSELH + GPIO_BASE

	//Oppsett av EXTIPSELH fra kompendiet
	MOV R4, #0b1111 << 4
	MVN R4, R4//Inverterer verdien i R4
	LDR R5, [R2]
	AND R6, R4, R5
	MOV R7, #PORT_B << 4
	ORR R6, R6, R7
	STR R6, [R2]

	//Oppsett av GPIO_EXTIFALL fra kompendiet.
	LDR R4, =GPIO_EXTIFALL + GPIO_BASE

	//Setter inn verdien 1 i R5, og flytter denne BUTTON_PIN=9 plasser til venstre
	MOV R5, #1 << BUTTON_PIN
	LDR R6, [R4]
	ORR R6, R6, R5
	STR R6, [R4]

	//Oppsett av GPIO_IFC fra kompendiet
	LDR R5, =GPIO_IFC + GPIO_BASE

	//Setter verdien 1 inn i R6, flytter denne BUTTON_PIN=9 plasser til venstre.
	MOV R6, #1 << BUTTON_PIN
	LDR R7, [R5]
	ORR R7, R7, R6
	STR R7, [R5]

	//Oppsett av GPIO_IEN fra kompendiet
	LDR R6, =GPIO_IEN + GPIO_BASE

	//Setter verdien 1 inn i R7, og flytter denne BUTTON_PIN=9 plasser til venstre.
	MOV R7, #1 << BUTTON_PIN
	LDR R8, [R6]
	ORR R8, R8, R7
	STR R8, [R6]


Loop:
	WFI //Wait for interrupt
	B Loop

.global SysTick_Handler
.thumb_func
SysTick_Handler:
	LDR R0, =tenths //Henter innholdet til minnelokasjonen til 'tenths' og legger det til i R0
	LDR R1, [R0] //Henter innholdet fra R0 og legger det til i R1
	ADD R1, #1 //Adderer innholdet i R1 med 1.
	CMP R1, #10 //Sammenligner verdien i R1 med 10
	BNE Show_tenths //Dersom verdien i R1 ikke er 10, hopper koden over økning av sekunder og minutter
	MOV R1, #0 //Dersom verdien i R1 er 10, resettes verdien til 0

	//Finner LED-lampen
	LDR R6, =GPIO_BASE
	LDR R7, =LED_PORT * PORT_SIZE
	LDR R8, =GPIO_PORT_DOUTTGL
	ADD R6, R6, R7
	ADD R6, R6, R8
	//Legger til på LED0 sin pin slik at den blinker for hvert sekund
	MOV R7, #1 << LED_PIN
	STR R7, [R6]

	LDR R2, =seconds //Henter innholdet fra minnelokasjonen til 'seconds' og legger dette til i R2
	LDR R3, [R2] //Henter innholdet i R2 og legger det i R3
	ADD R3, #1 //Adderer verdien i R3 med 1
	CMP R3, #60 //Sammenligner verdien i R3 med 60
	BNE Show_seconds //Dersom verdien i R3 ikke er 60, hopper koden over økning av minutter
	MOV R3, #0 //Dersom verdien i R3 er 60, resettes verdien til 0

	LDR R4, =minutes //Henter innholdet fra minnelokasjon 'minutes' og legger dette til i R4
	LDR R5, [R4] //Henter innholdet i R4 og legger det i R5
	ADD R5, #1 //Adderer verdien i R5 med 1
	B Show_minutes

//I disse metodene blir antall tienedelssekunder, sekunder og minutter lagret på minnelokasjonene
//'tenths', 'seconds' og 'minutes' slik at verdiene vises på skjermen på kortet.
Show_minutes:
	STR R5, [R4]
Show_seconds:
	STR R3, [R2]
Show_tenths:
	STR R1, [R0]

	BX LR

.global GPIO_ODD_IRQHandler
.thumb_func
GPIO_ODD_IRQHandler:
	//Setter enable-bitet i CTRL-registeret slik at SysTick interrupsene starter
	LDR R0, =SYSTICK_BASE + SYSTICK_CTRL
	LDR R1, [R0]
	EOR R1, #SysTick_CTRL_ENABLE_Msk
	STR R1, [R0]

	//Registrerer at SysTick interruptet er blitt behandlet.
	LDR R0, =GPIO_BASE + GPIO_IFC
	MOV R1, #1 << BUTTON_PIN
	STR R1, [R0]

	BX LR


NOP // Behold denne pÃ¥ bunnen av fila

