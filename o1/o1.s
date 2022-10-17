.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO

.text
	.global Start
	
Start:

    // Skriv din kode her...
	LDR R0, =GPIO_BASE //R0 = GPIO_BASE
	LDR R1, =PORT_SIZE //R1 = PORT_SIZE

   LDR R3, =GPIO_PORT_DIN // R3 = GPIO_PORT_DIN
   LDR R4, =BUTTON_PORT // R4 = BUTTON_PORT
   MUL R4, R4, R1 // R4 = R4 * R1 = BUTTON_PORT * PORT_SIZE
   ADD R4, R4, R0 // R4 = R4 + R0 = (BUTTON_PORT * PORT_SIZE) + GPIO_BASE
   ADD R3, R3, R4 // R3 = R3 + R4 = Avstanden fra BUTTON_PORT til DIN.

   LDR R4, = LED_PORT // R4 = LED_PORT
   MUL R4, R4, R1 // R4 = R4 * R1 = LED_PORT * PORT_SIZE
   ADD R4, R4, R0 // R4 = adressen til LED_PORT

   MOV R1, #1 // Setter 1 inn i R1 001
   LSL R1, R1, #LED_PIN // Left shift #LED_PIN = 2 på R1 slik at R1 = 100

   MOV R7, #1 // Setter 1 inn i R7
   LSL R7, R7, #BUTTON_PIN // Left shift #BUTTON_PIN = 9 slik at R7 = 1000000000

   LDR R0, =GPIO_PORT_DOUTSET // R0 = GPIO_PORT_DOUTSET
   LDR R8, =GPIO_PORT_DOUTCLR // R8 = GPIO_PORT_DOUTCLR


Check_button:

   LDR R6, [R3] // Lagrer minnelokasjonen til R3 = DIN i R6
   AND R6, R6, R7 // Legger sammen R6 = DIN og R7 for å sjekke om knappen blir trykket på
   CMP R6, 0 // Undersøker om R6 er lik 0
   BEQ Button_pressed // Dersom R6 == 0 går vi inn i funksjonen Button_pressed


Turn_off_LED:
	STR R1, [R4, R8] // Bruker DOUTCLR til å skru av LEDen
	B Check_button // Hopper tilbake til Check_button for å fotsette å lytte på knappen

Button_pressed:
	STR R1, [R4, R0] // Bruker DOUTSET til å skru på LEDen
	B Check_button //Hopper tilbake til Check_button for å fortsette å lytte på knappen



NOP // Behold denne pÃ¥ bunnen av fila

