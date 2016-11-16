/*
 * Projekt_Gruppe5.c
 * Created: 29.05.2015 13:37:10
 */ 


#define F_CPU 16000000UL

// Einbindung von Bibliotheken 
#include <avr/io.h>					
#include <avr/interrupt.h>
//#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <avr/pgmspace.h>
#include <math.h>

#include "FFT.h"
#include "uart.h"																
#include "lcd.h"
#include "twi.h"
#include "dataflash.h"
#include "delay.h"


#define log2(x) (log((float)(x))/log(2.0f))

// Deklaration Funktionen, welche nachfolgend noch erläutert werden
int8_t joystick(void);			
void joystick_Push (void);
void joystick_Right (void);
void joystick_Left (void);
void joystick_Down (void);
void joystick_Up (void);

void Display_Hauptmenue (void);
void Display_Stimmgeraet (void);
void Display_Metronom (void);

void Toggle_Buchstabe (uint8_t x0, uint8_t x1,uint8_t Page);


void Timer3_Init(void);


void Werteausgabe (void);

void Stimmgeraet_Zeiger (double Winkel,uint8_t mode);

void Tonabspielen(uint16_t frequenz,uint16_t dauer);  //Spielt einen Ton mit übergebener Frequenz und Dauer (in ms) ab
void Tonstarten(uint16_t frequenz); //Startet die Tonausgabe
void Tonstoppen(void); //Beendet die Tonausgabe

void ctc_on(void);  
void ctc_off(void); 

void timer_init(void);
void adc_init(void);
void adc(void);
void FFT(void);
void main_Judge();

// Deklaration von globalen Variablen, welche nachfolgend eläuertert werden
											
volatile uint16_t cnt_adc_result = 0;
volatile int16_t data_adc_result[FFT_N];
//volatile int16_t * data_adc_result = NULL;
//volatile uint16_t spektrum[FFT_N/2];		/* Spectrum output buffer */
volatile uint16_t *spektrum = NULL;		/* Spectrum output buffer */
volatile complex_t bfly_buff[FFT_N];		/* FFT buffer */
//volatile complex_t *bfly_buff = NULL;		/* FFT buffer */
volatile uint16_t maxima [20];

#define J_PUSH        0
#define J_RIGHT       1
#define J_LEFT        2
#define J_DOWN        3
#define J_UP          4

// Variablen für Metronom und Stimmgeraet
uint8_t Metronom_StartStopp = 0;				// Variable = 0 bedeutet, dass Metronom aus ist. Variable = 1, Metronom ist gestartet.				
uint8_t Stimmgeraet_TonausgabeStartStopp = 0;	// Variable = 0 bedeutet, dass Tonausgabe aus ist. Variable = 1, Tonausgabe ist gestartet.
uint8_t Stimmgeraet_Auswahl = 0;	            // Variable für Festlegung, welche Option im Stimmgeraetmenue ausgewählt wurde.
												// Stimmgeraet_Auswahl     Option
												//        0					Automatische Aunfahme
												//        1                 Manuelle Aufnahme
												//        2                 Tonausgabe

// Variablen, die bei Auslösen des Interrupts (jede ms) um 1 erhöht werden.
uint8_t timer_1ms=0;					
uint8_t timer_20ms=0;
uint8_t timer_40ms=0;
uint16_t timer_100ms=0;

// Variablen für Entprellen des Tasters
uint8_t zaehlen = 0;			
uint8_t low = 0;

// Für Menüansteuerung
uint8_t Cursor_Richtung = 2;		// Für Bestimmung, wo man sich im Menue befindet. Weitere Erklärungen weiter unten.
uint8_t BeatsPerMinute = 60;		// Wie viele Beats pro Minute gespielt werden sollen, einstellbar.	
uint8_t Takt = 1;					// Taktart durch integer beschrieben. 
									// Takt       Taktart
									// 0            3/4
									// 1            4/4
									// 2            8/8
typedef struct{
	const uint16_t f_max;
	const float f_frequenz;		// Index of the frequency in spectrum array
	char note[3];				// Name of the Note
	const uint16_t f_ton;	    // Frequenz des Tons
}GUITAR_STRING;

typedef enum{
	FFT_GET_ADC_VALUES,
	FFT_CALC_FFT,
	FFT_CALC_TONE,
	FFT_DISABLE
}FFT_STATUS;

FFT_STATUS fftStatus = FFT_DISABLE;

int8_t stringIndex;					//[-1, .. , 5] (-1 heißt kein Ton erkannt)

int16_t angle = 0;					//[-90, .. , 90] (signalisiert auch gleich ob zu klein ober zu groß (<0 -> "-" oder >0 -> "+")
int16_t anglevorher = 0;
uint8_t keinTonCounter = 0;

const GUITAR_STRING GUITAR[6] = {
	{84,  82.41,  "E2",82},
	{112, 110,    "A2",110},
	{149, 146.83, "D3",147},
	{200, 196,    "G3",196},
	{251, 246.94, "H3",247},
	{336, 329.63, "E4",330}
};

int8_t Saite = -0;	 // Variable, welche Saite gespielt werden soll/ gespielt wurde  [-1, .. , 5] (-1 heißt automatischer Modus)
char s[50];
uint32_t summ = 0;

uint8_t mallocCounter = 0;

//Saite   Note
//  0		E4
//	1		H3
//	2       G3
//	3		D3	
//	4		A2
//  5		E2


int main (void)
{
	
	spektrum = (uint16_t *)(data_adc_result);

	DDRA &= ~((1 << PA3) | (1 << PA4) | (1 << PA5)|(1 << PA6) | (1 << PA7));		// Pins PA3-PA7 auf Eingang schalten für Joystickansteuerung
	PORTA |= ((1<<PA3)|(1<<PA4)|(1<<PA5)|(1<<PA6)|(1<<PA7));						// interne Pull-Up-Widerstände werden aktiviert, für Joystickansteuerung als low active
	DDRD |= 0x20;																	// PB5 als Ausgang schalten
	PORTD =0x00;																	// Beim Start des Programms den Sound ausschalten
	
	LCD_Init();		// Initialisierung Display				
	UART_Init();
	adc_init();
	timer_init();
	Timer3_Init();
	
	Display_Hauptmenue ();			// Anzeige des Hauptmenues
	Backlight_LED(BL_RED_ON | BL_GREEN_ON | BL_BLUE_ON);
	int8_t joystick_position = -1;	// Startwert für Variable zur Joystickbedienung
	
	//Timer0_Init ();				// Einstellung der Eigenschaften des Interrupts (wird noch erläutert)			
	
	sei();							// Interrupts global erlauben

	// loop forever
	while(1)
	{	
		if (timer_1ms >= 1)							// es wird jede ms ein Interrupt ausgelöst. Dabei werden die Variablen "timer_1ms","timer_20ms""timer_100ms" um je eine ms erhöht. In der main erfolgen über if-Abfragen z.B. die Joysticksteuerung oder die Animationen.
		{
			timer_1ms = 0;							// die if-Abfrage soll jede ms abgerufen werden, weshalb die Variable zu null gesetzt wird.			
			
			static uint16_t hochzaehlen = 0;		// für die Animation des Metronoms werden drei Variablen eingeführt				
			static uint8_t zahl = 1;
			volatile uint8_t Zahl_Maximal;
			
			if (Cursor_Richtung < 16)				// LCD-Display besteht aus 128x64 Pixel, wobei diese in Blöcke von 6x8 Pixel unterteilt sind. Das Display hat somit 8 Zeilen. Über die Variable Curser_Richtung werden die Menues bedient (Hauptmenue, Menue für Stimmgeraet, Menue für Metronom).
				zahl = 1;							// Über die Variable ist die Position des Cursors einsehbar, welcher mit Hilfe des Joysticks bewegt wird. Für Curser_Richtung 0-7 befindet man sich im Hauptmenue, bei Curser_Richtung 8-15 im Stimmgeraetmenue, bei Curser_Richtung 16-23 im Metronommenue.

			if (Metronom_StartStopp == 1 && Cursor_Richtung >= 16)	// Über Variable Metronom_StartStopp wird Status festgehalten, ob die Animation des Metronoms gestartet wurde (Metronom_StartStopp = 1) oder nicht  (Metronom_StartStopp = 0).
			{
				switch (Takt)						// Takt = 1 entspricht 3/4-Takt.
				{									// Takt = 2 entspricht 4/4-Takt.
					// 3/4-Takt						// Takt = 3 entspricht 8/8-Takt.
					case 1:
					Zahl_Maximal = 3;				// Es werden die Zahlen von 1-3 periodisch durchlaufen.
					break;
					
					// 4/4-Takt
					case 2:
					Zahl_Maximal = 4;				// Es werden die Zahlen von 1-4 periodisch durchlaufen.
					break;
					
					// 8/8-Takt
					case 3:
					Zahl_Maximal = 8;				// Es werden die Zahlen von 1-8 periodisch durchlaufen.
					break;
				}
						
				if ( hochzaehlen >= (uint16_t)( 60000.0f / 1.05f / ((float)(BeatsPerMinute)) ) )		// Die im Metronommenue variierbare Variable BeatsPerMinute reicht von 40 - 180. Durch den Ausdruck 60000/BeatsPerMinute erhält man die Zeit in ms, welche vergeht bis zur nächsten Tonausgabe. Die Variable hochzaehlen wird verwendet, um das Zeitintervall zwischen zwei Tonausgaben durchlaufen zu lassen. 
				{
						Metronom_Zahl(zahl);					// Die Animations des Metronoms besteht aus Zahlen von 1-8, welche den Takt angeben. Die Variable zahl steht für die Zahl, welche auf dem Display angezeigt werden soll.
																// Über die Funktion Metronom_Zahl (ist in lcd.c erläutert) wird eine Zahl von 1-8 auf dem Display ausgegeben.
						switch (zahl)							// Fallunterscheidung für das Aufblinken der LEDs und Abspielen der Töne des Metronoms 
						{
							case 1:												
					    	PORTB = ~0x01;	//LED PB0 an
							
							Tonabspielen(500,50);				// Die "1" soll anders betont werden
							break;
							
							case 5:
							PORTB = ~0x01;	//LED PB0 an
							
							Tonabspielen(100,50);
							break;

							case 2: case 6:
							PORTB = ~0x03; //LED PB0+1 an
							
							Tonabspielen(100,50);
							break;
								
							case 3: case 7:
							PORTB = ~0x07; //LED PB0+1+2 an
							
							Tonabspielen(100,50);
							break;	
					
							case 4: case 8:
							PORTB = ~0x0f; //LED PB0+1+2+3 an

							Tonabspielen(100,50);
							break;
						}
																			
						LCD_Update();				// Aktualisation des Displays.
						
						if (zahl < Zahl_Maximal)	// Die auf die Display auszugebene Zahl wird um 1 erhöht. Es werden die Zahlen von 1 bis maximal 8 periodisch angezeigt.
						{
						zahl ++;
						} else { zahl = 1;}
						
						hochzaehlen = 0;			// Nachdem eine Zahl angezeigt wurde, wird hochzaehlen wieder auf null gesetzt, um die Zeit bis zur nächsten Tonausgabe/ Zahlanzeige hochzuzaehlen.
			     }	 
				 hochzaehlen ++;
			} 
		}
		
		if(timer_20ms >= 20)												
		{
			timer_20ms = 0;
			
			joystick_position= joystick();		// Alle 20 ms wird der Status der Eingänge der Joystick-Pins (PA3-PA7) abgefragt. Die Funktion joystick wird später erklärt.
			
			switch (joystick_position)
			{
				case J_PUSH:					// Je nach dem, welcher Eingang auf low gezogen ist, wird eine Funktion aufgerufen, durch welche die entsprechende Aktion auf dem Display ausgeführt wird.
				joystick_Push ();
				break;
				
				case J_RIGHT:
				joystick_Right ();
				break;
				
				case J_LEFT:
				joystick_Left ();
				break;
				
				case J_UP:
				joystick_Up ();
				break;
				
				case J_DOWN:
				joystick_Down ();
				break;
				
				default:
				low++;
				break;
			}

		}
		
if (Stimmgeraet_Auswahl < 2 && Cursor_Richtung > 7 && Cursor_Richtung < 16 )
{
	if (fftStatus == FFT_CALC_FFT)
	FFT();
	else if (fftStatus == FFT_CALC_TONE)
	{
		main_Judge();
		
		if (stringIndex >= 0)						// Wenn ein Ton erkannt wurde...
		{
			if (Stimmgeraet_Auswahl == 0)			// Nach FFT und wenn automatische Aufnahme aktiv ist und ein Ton erkannt wurde.
			{
				LCD_GotoXY(14,2);
				LCD_PutString(GUITAR[stringIndex].note);				// Ton wird über Zeigerdiagramm eingeblendet.
			}
		
		// zwei Fallunterscheidungen bei Ton. Entweder liegt Zeigerausschlag im Zeigerdiagramm oder außerhalb.
		
			//durch diese korrektur gleich am anfang erspart man sich die vielen Fallunterscheidungen
			//ein float oder double auf Gleichheit testen kann schief gehen, weil diese Datentypen haben eine begrenzte Genauigkeit
			
// 			if (angle < -90) angle = -90;
// 			else if (angle > 90) angle = 90;

			
			//if (angle > -90 && angle < 90 )					// Zeiger wird im Diagramm eingezeichnet
			//{
				Stimmgeraet_Zeiger(anglevorher,0);      // Voriger Zeiger wird gelöscht, ebenso wie + und -.
				Stimmgeraet_Zeiger(angle,1);			// Neuer Zeiger wird gezeichnet, ebenso wie + und -.
				anglevorher = angle;					// Jetziger Winkelwert wird in Variable anglevorher übergeben, um beim nächsten Zeigerausschlag den alten zu löschen.
			//}
		
		//das hier brauchen wir dann nicht mehr
// 			if (angle == 90 || angle == -90)				// Zeigerausschlag liegt außerhalb von Darstellungsbereich.
// 			{
// 				Stimmgeraet_Zeiger(anglevorher,0);			// Voriger Zeiger wird gelöscht, ebenso wie + und -.
// 				Stimmgeraet_Zeiger(angle,1);				// + oder - werden geschrieben.
// 				anglevorher = angle;
// 			}

			LCD_Update();
			keinTonCounter = 0;
		}
		else 
		{
			keinTonCounter++;
			if (keinTonCounter >= 3)	//wenn 3 zyklen kein Ton erkannt wurde, zeiger löschen
			{
				if (Stimmgeraet_Auswahl == 0)			// Nach FFT und wenn automatische Aufnahme aktiv ist und ein Ton erkannt wurde.
				{
					LCD_GotoXY(14,2);
					LCD_PutString(PSTR("XX"));				// Ton wird über Zeigerdiagramm eingeblendet.
				}
				Stimmgeraet_Zeiger(anglevorher,0);      // Voriger Zeiger wird gelöscht, ebenso wie + und -.
				anglevorher = 0;
				keinTonCounter = 0;
				LCD_Update();
			}
		}
		
	
	
	}	// else if
   }	// if
  }		// while(1)
}		// main()

void Timer3_Init (void)        // Interrupt Timer mit der Frequenz 1 kHz
{
	TCCR3B |= (1 << CS30) | (1 << CS31) | (1<<WGM32); // Prescaler = 64, CTC Mode an
	//Fequenz am Ausgang ist f = f(Oszillator)/(Prescaler*(OCRnA + 1)).
	//    OCR3A = F_CPU/(64*1000)-1;
	OCR3A = 250-1;                            // Der Wert im Output Compare Register(OCR0A) wird ständig mit dem aktuellen Wert im Datenregister TCNT1H/TCNT1L verglichen. Stimmen die beiden Werte überein, so wird ein sogenannter Output Compare Match ausgelöst.
	TIMSK3 |= (1 << OCIE3A);                // Compare Interrupt erlauben.
	// Gemäß genannter Formel ist die Frequenz f der Interrupts: f = 16 Mhz /(64 * (250 - 1 + 1)) = 1000 Hz. Es wird also jede ms ein Interrupt ausgelöst.
}

ISR (TIMER3_COMPA_vect)       // Compare Interrupt Handler wird jede ms aufgerufen, dadurch werden die Variablen um je 1 erhöht.
{
	timer_1ms++;
	timer_20ms++;
}


void timer_init()
{
	//Time=PRE*(MAX-TCNT0+1) /F_CPU (S), MAX:255
	TCCR0B |= (_BV(CS01) | _BV(CS00));	//Timer0 Prescaler 64  16MHz/64=0.25Mhz
}

ISR(ADC_vect)
{
	TCNT0 = 0x9E;      //Each Tact is 4us, For 1ms will 250 Tacts be counted.
	
	if(fftStatus == FFT_GET_ADC_VALUES && cnt_adc_result < FFT_N)
	{
		int16_t value = ADCW;
		data_adc_result[cnt_adc_result++] = value - 32736;
		
		if (cnt_adc_result>=FFT_N)
			fftStatus = FFT_CALC_FFT;
	}
	else
	{
		ADCW;
	}
	
	if(fftStatus == FFT_DISABLE && cnt_adc_result)
		cnt_adc_result = 0;
	
	TIFR0 |= _BV(TOV0);// Clear Timer0 overflow flag
}
void adc_init()
{
	ADMUX |= _BV(REFS0);		                     // Voltage Reference: AVCC
	ADMUX |= _BV(ADLAR);		                     // Save high 8 bit in ADCH (left-justified)
	ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);  // ADC Sampling Rate Prescaler 128    16MHz/128=125kHz sampling rate
	ADCSRB |= _BV(ADTS2);		                     // ADC Trigger Timer0 Overflow
	ADCSRA |= _BV(ADEN);		                     // ADC Enable
	
	TCNT0 = 0x9E;
	sei();							// Enable all Interrupts
	ADCSRA |= _BV(ADIE);			// Enable ADC Finish Interrupt
	ADCSRA |= _BV(ADATE);			// Enable ADC Auto-Trigger

}
// void adc()
// {
// 	cnt_adc_result = 0;
// 	Backlight_LED(BL_RED_ON);
// 	LCD_GotoXY(0,5);
// 	LCD_PutString("ADC-Start");
// 	LCD_Update();
// 	
// 	
// 	while (cnt_adc_result < FFT_N); //Waiting for finishing sampling
// 	LCD_GotoXY(0,5);
// 	LCD_PutString("ADC-Stop ");
// 	LCD_Update();
// 
// 	cnt_adc_result = 0;				// Clear cnt_adc_result
// }

void FFT()
{
	// adc();
	cnt_adc_result = 0;
	
//	memset( (void*)(bfly_buff) ,0 ,FFT_N*sizeof(bfly_buff[0]) ); // Clear bfly_buff
		
	fft_input(data_adc_result, bfly_buff);
	fft_execute(bfly_buff);

//	memset( (void*)(spektrum) ,0 ,FFT_N/2*sizeof(spektrum[0]) ); // Clear spectrum

	fft_output(bfly_buff, spektrum);
	
	fftStatus = FFT_CALC_TONE;
}


void main_Judge()
{
	uint16_t i;
	uint16_t maxCounter = 0;
	uint16_t maximumAmplitude = 0;
	uint16_t iMax = FFT_N/2;
	//find global maximum Amplitude
	for (i = 0; i < iMax; i++)
	{
		if (maximumAmplitude < spektrum[i])
		{
			maximumAmplitude = spektrum[i];
		}
	}
	
	//find only amplitudes greater than a third of maximum amplitude
	maximumAmplitude /= (uint16_t)(3);
	
	//find frequencies that have an amplitude-peek (up to 20 values) save them in an array maxima[]
	for (i = 1; i < (FFT_N/2 - 1); i++)
	{
		if ( (spektrum[i]>spektrum[i-1]) && (spektrum[i] > spektrum[i+1]) && (spektrum[i] > maximumAmplitude ) && (maxCounter < 20) && (spektrum[i] > (uint16_t)(32)) )
		{
			maxima[maxCounter] = i;
			maxCounter++;
		}
	}
	
	//if an maxima is found (mostly the main Frequence is saved in maxima[0]
	if (maxCounter)
	{
		uint8_t nearestTone = 0;
		int16_t minAbstand = 0x7FFF;
		int16_t distance = 0x7FFF;
		int16_t uSignedAbstand = 0;
	
		//find the nearest corresponding guitar-string
		if (Saite < 0)
		{
			for (i = 0; i < 6; i++)
			{
				distance =  (int16_t)(GUITAR[i].f_max) - (int16_t)(maxima[0]);
		
				if (distance < 0)
				{
					uSignedAbstand = 0 - distance;
				}
				else
				{
					uSignedAbstand = distance;
				}
		
				if (uSignedAbstand < minAbstand)
				{
					minAbstand = uSignedAbstand;
					nearestTone = i;
				}
			}
		}
		else
		{
			nearestTone = Saite;
		}
		//calculate the differenz in cent 		
		int16_t cent = (int16_t)( 1200.0f * log2((float)(maxima[0])/(float)(GUITAR[nearestTone].f_max)) );
		//the minimum cent differenz is about 5 and the minimum degree difference you can display on the LCD is 2 degrees
		//we have a radius +- 90° 
		//minimum is -225 cent and maximum +225 cent
		if (cent > 225)
			cent = 225;
		else if (cent < -225)
			cent = -225;
		
		angle = cent * 90 / 225;
		
		stringIndex = nearestTone;
		
//  		LCD_GotoXY(0,4);
//  		sprintf(s,"F:%i  ",maxima[0]);
// 		LCD_PutString(s);
// 		LCD_GotoXY(0,5);
// 		sprintf(s,"C:%i  ",cent);
// 		LCD_PutString(s);		
// 		LCD_GotoXY(0,6);
// 		sprintf(s,"G:%i I:%i   ",angle,stringIndex);
// 		LCD_PutString(s);
// 		LCD_Update();
		
//		memset( (void*)(data_adc_result), 0, FFT_N*sizeof(data_adc_result[0]) ); // Clear data_adc_result
	}
	else 
	{
		angle = 0;
		stringIndex = -1;
		
	}

	fftStatus = FFT_GET_ADC_VALUES;
}

// Das gesamte Menue besteht aus einem Hauptmenue, einem Stimmgeraetmenue und einem Metronommenue. Jedes einzelne der drei Menues erstreckt sich über eine Displayfläche. Daher gibt es insgesamt 3 "Seiten". Mit Hilfe des Joysticks kann man einerseits die einzelnen Funktionen im jeweiligen Menue auswählen und auch das Menue wechseln (z.B. vom Hauptmenue ins Stimmgeraetmenue).
// Nachfolgend sollen die einzelnen Menues aufgelistet werden, sodass auch die Funktionsweise der Variablen Curser_Richtung veranschaulicht wird.

// Darstellung Hauptmenue auf Display mitsamt Funktionen, Zeile auf Display und Cursor_Richtung

//										      Funktion			                        Zeile Display     Cursor_Richtung
// Hauptemenue						Deklaration des Menues	                                    0			0
//																	                            1			1
// Stimmgeraet						Durch Tastendruck Übergang in Stimmgeraetmenue				2			2
//									Durch Tastendruck Übergang in Metronommenue					3			3
//																								4			4
//																								5			5
//																								6			6
// DMM 2015, Gruppe 5				Gruppenbezeichnung											7			7

										//Stimmgeraetmenue

// Darstellung Stimmgeraetmenue						Funktion							Zeile Display  Cursor_Richtung
//Stimmgeraet										Menuename									0			8
//																								1			9
// Auswahl zwischen automatischer Aufnahme, manueller Aufnahme und Tonausgabe				    2			10
//	Ton: <  >								Wahl des Tones für manuelle Aufnahme/ Tonausgabe	3			11
// Start/Stopp								Start und Stopp der Tonausgabe						4			12
// 																								5			13
//																								6			14
// Zurueck									Zurück ins Hauptmenue								7			15

						//Darstellung Metronommenue

//												Funktion							Zeile Display	Cursor_Richtung
//Metronom																					0			16
//																							1			17
//Start (Stopp)							Starten/ Stoppen der Animation						2			18
//								(Periodische Durchlauf der Zahlen von 1 bis max 8)			3			19
 //BPM: (Wert)								Einstellung der Beats pro Minute				4			20
// Takt: <Art>								Einstellung der Taktart	(3/4,4/4,8/8)		    5			21
//																							6			22
// Zurueck									Zurück ins Hauptmenue							7			23

void Display_Hauptmenue (void)												// Gestaltung des Hauptmenues
{
	
	LCD_GotoXY(0,0);
	LCD_PutString_P(PSTR("     Hauptmenue\r\n\n"));
	LCD_PutString_P(PSTR(" Stimmgeraet \r\n"));
	LCD_PutString_P(PSTR(" Metronom \r\n\n\n\n"));
	LCD_PutString_P(PSTR(" DMM 2015, Gruppe 5 \r\n"));

	LCD_DrawLine (30, 8, 90, 8, 1);
	Toggle_Buchstabe (6,71,2);												// Bedienungscursor auf Option Stimmgeraet (zum Übergang ins Stimmgeraetmenue) gelegt, weshalb das Feld schwarz hinterlegt wird.
	LCD_Update();
}

void Display_Stimmgeraet (void)												// Gestaltung des Stimmgeraetmenues
{
	LCD_GotoXY(0,0);
	LCD_PutString_P(PSTR("     Stimmgeraet\r\n\n"));
	LCD_PutString_P(PSTR(" Auto.\r\n\n\n\n\n"));
	LCD_PutString_P(PSTR(" Zurueck \r\n"));
	LCD_DrawLine (30, 8, 96, 8, 1);
		
	LCD_GotoXY(0,2);											// Pfeile links und rechts von Auto., um anzuzeigen, dass man weitere Optionen auswaehlen kann.
	LCD_Zahlen(13);
	LCD_GotoXY(36,2);
	LCD_Zahlen(12);			
								
	LCD_DrawLine (87, 24, 93, 24, 1);							// Pfeil oben in Zeigerdiagramm als Referenz für Idealwert des Tones.	
	LCD_DrawLine (88, 25, 92, 25, 1);
	LCD_DrawLine (89, 26, 91, 26, 1);
	LCD_DrawLine (90, 27, 90, 27, 1);
	
	LCD_DrawCircle(90,62,34,1);
	LCD_DrawPixel(56,63,0);
	LCD_DrawPixel(124,63,0);
    LCD_DrawLine (56, 62, 124, 62, 1);							// Linien als Grenzen (Mininum/Maximum) der Zeigerdarstellung.
		
	//for (uint8_t i = 0 ; i <= 90; i = i + 2)
	//{
// 		Stimmgeraet_Zeiger (M_PI/180*82, 1);
// 		Stimmgeraet_Zeiger (M_PI/180*84, 1);
// 		Stimmgeraet_Zeiger (M_PI/180*81, 1);
// 		Stimmgeraet_Zeiger (M_PI/180*80, 1);
// 		Stimmgeraet_Zeiger (M_PI/180*30, 1);
	//}
	
	LCD_DrawCircle(114,19,5,1);
// 	LCD_DrawLine (114, 16, 114,22, 1);
// 	LCD_DrawLine (111, 19, 117, 19, 1);
	
	LCD_Update();
}

void Display_Metronom (void)															// Gestaltung des Metronommenues
{
	LCD_GotoXY(0,0);
	LCD_PutString_P(PSTR("       Metronom\r\n\n"));
	LCD_PutString_P(PSTR(" Start \r\n\n"));
	LCD_PutString_P(PSTR(" BPM: 60 \r\n"));												// Anfangswert: 60 BPM.	
	LCD_PutString_P(PSTR(" Takt: 3/4 \r\n\n"));											// Anfangswert für Takt: 3/4-Takt.
	LCD_PutString_P(PSTR(" Zurueck \r\n"));
	
	LCD_DrawLine (42, 8, 90, 8, 1);
	Toggle_Buchstabe (6,35,2);															// Bedienungscursor auf Option Start gelegt, weshalb das Feld schwarz hinterlegt wird.		

	LCD_Update();
}

int8_t joystick(void)									// Es wird ausgewertet, welcher Eingang auf low geschaltet ist und damit, wie der Joystick bewegt wurde (gedrückt, oben, unten, rechts, links).													
{
	uint8_t position;									// Taster ist als low-active geschaltet. 
	
	position = ~(PINA) & 0xF8;							// Über PINA wird der Status der Eingänge des PORTA abgefragt. Mit der Maske 11111000 liefert die Variable position also einen für eine Tasterbetätigung (gedrückt, oben, unten, rechts, links) charakteristischen Wert.
	
	switch (position)
	{
		case 8:											// Die Werte werden hier abgerufen und entsprechend wird eine Konstante zurückgegeben.
		return J_PUSH;
		case 16:
		return J_RIGHT;
		case 32:
		return J_LEFT;
		case 64:
		return J_DOWN;
		case 128:
		return J_UP;
		default:
		return -1;
	}
}

void joystick_Push (void)				// Funktion, was passiert, wenn Joystick gedrückt wird.					
{
	if (low>= 2)						// Taster (= Joystick) ist als low active geschaltet. Über die Variablen low und zaehlen wird der Taster entprellt. Es wird der Übergang von High-Pegel auf Low-Pegel registriert und als Tastendruck interpretiert. 
	{									// Der Taster muss mindestens 40 ms	(Variable low wird alle 20 ms um erhöht, siehe main unter if(timer_20ms)) auf high sein. Die Variable zaehlen wird jedes Mal erhöht, wenn einerseits am PIN mindestens 40 ms High-Pegel angelegt ist und andererseits Low-Pegel anliegt. 
		zaehlen++;		
														
		if (zaehlen >= 2)				// Ist zaehlen um mindestens 2 hochgezaehlt worden (= Eingang 40 ms auf low), gilt ein Tastendruck (Übergang von high auf low) als bestätigt und es werden die entsprechenden Funktionen ausgeführt. 
		{
			switch (Cursor_Richtung)					
			{								            
			case 2:					// Geht ins Stimmgeraetmenue.			
				LCD_Clear ();
				Display_Stimmgeraet ();
				Cursor_Richtung = 10;
				fftStatus = FFT_GET_ADC_VALUES;
				Saite = -1;
				break;
				
			case 3:					// Geht ins Metronommenue.
				LCD_Clear ();
				Display_Metronom ();
				Cursor_Richtung = 18;
				break;
				
			case 12:				// Tonausgabe starten und stoppen.
				LCD_GotoXY(0,4);														
				
				if (Stimmgeraet_TonausgabeStartStopp)
				{
					LCD_PutString_P(PSTR(" Start \r\n"));							// Stoppen der Tonausgabe, also wird Start angezeigt.
					Stimmgeraet_TonausgabeStartStopp = 0;
					Tonstoppen();													// Stoppt die Tonausgabe
											
				}else {									
					LCD_PutString_P(PSTR(" Stopp \r\n"));							// Starten der Tonausgabe, also wird Stopp angezeigt.
					Stimmgeraet_TonausgabeStartStopp = 1;
					Tonstarten(GUITAR[Saite].f_ton);								// Startet die Tonausgabe mit der Frequenz der aktuell ausgewählten Saite
				}	
				Toggle_Buchstabe (6,35,4);											// Start bzw. Stopp wird schwarz hinterlegt.		
				LCD_Update();
				break;
				
				//Stimmgeraetmenue						
			case 15:
				Tonstoppen();								// Stoppt die Tonausgabe								
				LCD_Clear ();							
				Display_Hauptmenue ();
				Stimmgeraet_Auswahl = 0;// Wieder auf Auto gesetzt.			
				Saite = 0;              // Gitarrensaite auf E4 zurückgesetzt.    
				Cursor_Richtung = 2;
				fftStatus = FFT_DISABLE;			
				break;									
					
				// Metronommenue						
			case 18:																							
																																					
				LCD_GotoXY(0,2);																	
				
				if (Metronom_StartStopp)					
				{																								
					LCD_PutString_P(PSTR(" Start \r\n"));																			
					Metronom_StartStopp = 0;																									
															
					}else {																	
					LCD_PutString_P(PSTR(" Stopp \r\n"));    
					Metronom_StartStopp = 1;								
				}												
																
				Toggle_Buchstabe (6,35,2);					
				LCD_Update();
				break;

			case 23:
				Tonstoppen();								// Stoppt die Tonausgabe
				LCD_Clear ();
				Display_Hauptmenue ();
				Cursor_Richtung = 2;						// Rückkehr zum Hauptmenue
				Metronom_StartStopp = 0;					// Metronomanimation wird gestoppt.
				Takt = 1;									//Takt und Beats pro Minute werden auf Anfangswerte bei Verlassen von Metronommenue zurückgesetzt.
				BeatsPerMinute = 60;
				PORTB =0xFF;									//LEDs ausschalten
				break;
				
				default:
				break;
			}
			
			zaehlen =0;										// Nachdem Eingangssignal von PINA3 von high auf low geschaltet wurde, werden die Werte wieder auf null gesetzt.	
			low=0;
		}
	}
}

void joystick_Right (void)
{
	static uint8_t a=0;
											// Durch die Variable a ist es möglich, bei gedrücktem Taster nach Rechts die Werte von z.B. Beats pro Minute alle 200 ms zu erhöhen (ohne loslassen des Tasters). 
// 	if (low>= 2)						// Taster (= Joystick) ist als low active geschaltet. Über die Variablen low und zaehlen wird der Taster entprellt. Es wird der Übergang von High-Pegel auf Low-Pegel registriert und als Tastendruck interpretiert.
// 	{									// Der Taster muss mindestens 40 ms	(Variable low wird alle 20 ms um erhöht, siehe main unter if(timer_20ms)) auf high sein. Die Variable zaehlen wird jedes Mal erhöht, wenn einerseits am PIN mindestens 40 ms High-Pegel angelegt ist und andererseits Low-Pegel anliegt.
// 		zaehlen++;
// 		if (zaehlen >= 2)
		//den Teil habe auskommentiert, weil die Menu Wahl im Stimmgeraet zu schnell geht
	if (low>= 2 || a>= 1)
	{
		if (low >=2 )
		zaehlen++;
		
		if (zaehlen >= 2 || a >=10)
		//bis hier
		{
			switch (Cursor_Richtung)						// Je nach dem, in welchem Menue und in welcher Zeile man sich befindet (beide Informationen erhält man aus der Variablen Cursor_Richtung), wird eine entsprechende Funktion ausgeführt, welche mit einer Switch-Abfrage ermittelt wird.
			{
				case 10:
				
					switch (Stimmgeraet_Auswahl)
					{
						case 0:	
						Saite = 0;													// Stimmgeraet_Auswahl = 1 (Manuelle Aufnahme)
						LCD_GotoXY(1,2);
						LCD_PutString_P(PSTR("Manu."));
						LCD_GotoXY(0,3);
						LCD_PutString_P(PSTR(" Ton:\r\n"));
						LCD_GotoXY(14,2);										// Anzeige über Pfeil in Zeigerdiagramm, welcher Ton als Referenz dient / zu stimmen ist.	
						LCD_PutString(GUITAR[Saite].note);
						LCD_GotoXY(6,3);
						LCD_PutString(GUITAR[Saite].note);						// Auswahl des Tones.
						Stimmgeraet_Zeiger(anglevorher,0);
						Stimmgeraet_Auswahl = 1;								// Auf 1 gesetzt, da Menueeinstellung Manuelle Aufnahme gewählt wurde.
						fftStatus = FFT_GET_ADC_VALUES;
						break;
						
						case 1:
						Saite = 0;												// Stimmgeraet_Auswahl = 2 (Tonausgabe)
						LCD_GotoXY(1,2);
						LCD_PutString_P(PSTR("Tona."));
						LCD_GotoXY(14,2);										
						LCD_PutString_P(PSTR("  "));							// Löschen des Tones (z.B. E4) über Pfeil des Zeigers.
						LCD_GotoXY(0,3);
						LCD_PutString_P(PSTR(" Ton:\r\n"));
						LCD_PutString_P(PSTR(" Start\r\n"));
						LCD_GotoXY(6,3);										// Auswahl des Tones.
						LCD_PutString(GUITAR[Saite].note);
						Stimmgeraet_Zeiger(anglevorher,0);							
					    Stimmgeraet_Auswahl = 2;								// Auf 2 gesetzt, da Menueeinstellung Tonausgabe gewählt wurde.
						fftStatus = FFT_DISABLE;
						break;
						
						case 2:
						Saite = -1;												// Stimmgeraet_Auswahl = 0 (Automatische Aufnahme)
						LCD_GotoXY(1,2);
						LCD_PutString_P(PSTR("Auto."));
						LCD_GotoXY(0,3);
						LCD_PutString_P(PSTR("         \r\n"));
						LCD_PutString_P(PSTR("         \r\n"));	
						Stimmgeraet_Auswahl = 0;							// Auf 0 gesetzt, da Menueeinstellung automatische Aufnahme gewählt wurde.
						fftStatus = FFT_GET_ADC_VALUES;
						Tonstoppen();													// Stoppt die Tonausgabe	
						break;
					}
					
					LCD_Update();

					break;	
					
				case 11:														// Einstellung des Tones. Über Variable Saite wird Ton durchlaufen.
				if (Saite < 5)	
				{			
					Saite ++;
				
					if (Saite == 5)	
					{
						LCD_GotoXY(48,3);
						LCD_Zahlen(10);
					}
					
					else if ( Saite == 1)
					{
						LCD_GotoXY(30,3);
						LCD_Zahlen(13);
					}
		    	
				LCD_GotoXY(6,3);										
				LCD_PutString(GUITAR[Saite].note);								// Ton wird auf Display ausgegeben.
				Tonstoppen();													// Stoppt die Tonausgabe
				if (Stimmgeraet_TonausgabeStartStopp)
				{
					Tonstarten(GUITAR[Saite].f_ton);							// Startet die Tonausgabe mit der Frequenz der aktuell ausgewählten Saite				
				}
				if (Stimmgeraet_Auswahl == 1)									// Falls die Einstellung des manuell zu stimmenden Tones vorliegt, wird der ausgewaehlte Ton über Pfeil des Zeigerdiagrammes angegeben.
					{	
						LCD_GotoXY(14,2);							
						LCD_PutString(GUITAR[Saite].note);
						Stimmgeraet_Zeiger(anglevorher,0);
					}
				LCD_Update();
				}
				break;
				
				// Metronommenue
				case 20:
				if (BeatsPerMinute < 180)					// Maximalwert von Beats pro Minute sind 300. 
				{
					BeatsPerMinute = BeatsPerMinute + 1;   // Über globale Variable BeatsPerMinute und einem Rechtsdruck des Tasters in entsprechender Zeile kann BPM variiert werden.
					
					if (BeatsPerMinute == 180)				
					{
						LCD_GotoXY(54,4);					// Bewegung des nicht sichtbaren Cursors für Displayausgabe.
						LCD_Zahlen(10);						// Durch dise Funktion (erläutert in lcd.c) wird ein Pfeil gelöscht.
					}
					else if(BeatsPerMinute == 41)
					{
						LCD_GotoXY(30,4);
						LCD_Zahlen(13);						// Durch dise Funktion wird ein Pfeil (links rum) hinzugefügt.
					}
					
					Werteausgabe ();						// Durch diese Funktion (erläutert weiter unten) wird der neue Wert von BPM ausgegeben.
					LCD_Update();
				}
				break;
				
				case 21:
				
				if (Takt < 3)
				{
					Takt ++;								// Takt wird um eins erhöht.
					
					if (Takt == 3)							
					{
						LCD_GotoXY(7,5);
						LCD_PutString_P(PSTR("8/8"));
						LCD_GotoXY(60,5);					
						LCD_Zahlen(10);						// Pfeil wird gelöscht.
					}
					else if(Takt == 2)
					{	
						LCD_GotoXY(7,5);
						LCD_PutString_P(PSTR("4/4"));
						LCD_GotoXY(36,5);					
						LCD_Zahlen(13);						// Pfeil wird hinzugefügt.
					}

					LCD_Update();
				}
				break;
				
				default:
				break;
			}
			
			zaehlen =0;
			low=0;
			a = 1;
		}
		a++;
		if (a >= 11)
		a = 0;
	}
}

void joystick_Left (void)				// Funktioniert analog zur Funktion "joystick_Right".
{
	static uint8_t a=0;

// 	if (low>= 2)						// Taster (= Joystick) ist als low active geschaltet. Über die Variablen low und zaehlen wird der Taster entprellt. Es wird der Übergang von High-Pegel auf Low-Pegel registriert und als Tastendruck interpretiert.
// 	{									// Der Taster muss mindestens 40 ms	(Variable low wird alle 20 ms um erhöht, siehe main unter if(timer_20ms)) auf high sein. Die Variable zaehlen wird jedes Mal erhöht, wenn einerseits am PIN mindestens 40 ms High-Pegel angelegt ist und andererseits Low-Pegel anliegt.
// 		zaehlen++;	
// 		if (zaehlen >= 2)
		//ab hier mit schnelllauf
	if (low>= 2 || a>= 1)
	{
		if (low >=2 )
		zaehlen++;
		if (zaehlen >= 2 || a >=10 )
		//Bis hier
		{
			switch (Cursor_Richtung)
			{
			case 10:
								
				switch (Stimmgeraet_Auswahl)
				{
				case 0:
					Saite = 0;	
					//tonanalyse deaktivieren
					LCD_GotoXY(1,2);
					LCD_PutString_P(PSTR("Tona."));
					LCD_GotoXY(0,3);
					LCD_PutString_P(PSTR(" Ton:\r\n"));
					LCD_PutString_P(PSTR(" Start\r\n"));
					LCD_GotoXY(6,3);
					LCD_PutString(GUITAR[Saite].note);
					Stimmgeraet_Zeiger(anglevorher,0);
					LCD_GotoXY(14,2);
					LCD_PutString_P(PSTR("  "));
					Stimmgeraet_Auswahl = 2;	
					fftStatus = FFT_DISABLE;						
					break;
									
				case 1:
					Saite = -1;	
					//tonanalyse aktiv
					LCD_GotoXY(1,2);
					LCD_PutString_P(PSTR("Auto."));
					LCD_GotoXY(0,3);
					LCD_PutString_P(PSTR("         \r\n"));
					LCD_PutString_P(PSTR("         \r\n"));
					LCD_GotoXY(14,2);											
					LCD_PutString_P(PSTR("  "));
					Stimmgeraet_Zeiger(anglevorher,0);
					Stimmgeraet_Auswahl = 0;
					fftStatus = FFT_GET_ADC_VALUES;	
									
					break;
									
				case 2:
					//tonanalyse aktiv
					Saite = 0;
					LCD_GotoXY(1,2);
					LCD_PutString_P(PSTR("Manu."));
					LCD_GotoXY(0,3);
					LCD_PutString_P(PSTR(" Ton:\r\n"));
					LCD_PutString_P(PSTR("         \r\n"));
					LCD_GotoXY(6,3);
					LCD_PutString(GUITAR[Saite].note);
					LCD_GotoXY(14,2);
					LCD_PutString(GUITAR[Saite].note);
					Stimmgeraet_Auswahl = 1;		
					Tonstoppen();													// Stoppt die Tonausgabe									
					fftStatus = FFT_GET_ADC_VALUES;
					
					break;
									
				}
								
				LCD_Update();

				break;
			case 11:														// Einstellung des Tones. Über Variable Saite wird Ton durchlaufen.
				if (Saite > 0)
				{
					Saite --;
				
				if (Saite == 4)
				{
					LCD_GotoXY(48,3);
					LCD_Zahlen(12);

				}
				
				else if ( Saite == 0)
				{
					LCD_GotoXY(30,3);
					LCD_Zahlen(10);
				}
				
				LCD_GotoXY(6,3);
				LCD_PutString(GUITAR[Saite].note);								// Ton wird auf Display ausgegeben.
				Tonstoppen();													// Stoppt die Tonausgabe
				if (Stimmgeraet_TonausgabeStartStopp)
				{
					Tonstarten(GUITAR[Saite].f_ton);							// Startet die Tonausgabe mit der Frequenz der aktuell ausgewählten Saite				
				}
				
				if (Stimmgeraet_Auswahl == 1)									// Falls die Einstellung des manuell zu stimmenden Tones vorliegt, wird der ausgewaehlte Ton über Pfeil des Zeigerdiagrammes angegeben.
				{
					LCD_GotoXY(14,2);
					LCD_PutString(GUITAR[Saite].note);
					Stimmgeraet_Zeiger(anglevorher,0);
				}
				
				LCD_Update();
				}
				break;
				
			case 20:
				if (BeatsPerMinute > 40)
				
				{
					BeatsPerMinute = BeatsPerMinute - 1;
					
					if (BeatsPerMinute == 40)
					{
						LCD_GotoXY(30,4);
						LCD_Zahlen(10);
					}
					else if(BeatsPerMinute == 179)
					{
						LCD_GotoXY(54,4);
						LCD_Zahlen(12);
					}
					
					Werteausgabe ();
					LCD_Update();
				}
				break;
				
			case 21:
				if (Takt > 1)
				{
					Takt --;
					
					if (Takt == 1)
					{	
						LCD_GotoXY(7,5);
						LCD_PutString_P(PSTR("3/4"));
						LCD_GotoXY(36,5);
						LCD_Zahlen(10);
					}
					else if(Takt == 2)
					{	
						LCD_GotoXY(7,5);
						LCD_PutString_P(PSTR("4/4"));
						LCD_GotoXY(60,5);
						LCD_Zahlen(12);
					}
					
					LCD_Update();
				}
				break;
				
				default:
				break;
			}
			
			zaehlen =0;
			low=0;
			a = 1;
		}

		a++;
		if (a >= 11)
		a = 0;
	}
}

void joystick_Down (void)											// Funktion, in der steht, was passiert, wenn Joystick nach unten gedrückt wird.
{
	if (low>= 2)
	{
		zaehlen++;
		
		if (zaehlen >= 2)
		{
			switch (Cursor_Richtung)
			{
				//Hauptmenue
			case 2:												
				Toggle_Buchstabe (6,71,2);							// Es wird das Wort Stimmgeraet nicht mehr schwarz hinterlegt angezeigt.
				Toggle_Buchstabe (6,53,3);							// Es wird das Wort Metronom schwarz hinterlegt angezeigt.
				LCD_Update();
				Cursor_Richtung = 3;								// Cursor auf Display wird um eine Zeile nach unten geschoben, deshalb wird Cursor_Richtung um 1 erhöht.
				break;
				//Menue_Stimmgeraet
			case 10:											// Auswahlmenue im Stimmgeraetmenue.
				LCD_GotoXY(0,2);
				LCD_Zahlen(10);
				LCD_GotoXY(36,2);
				LCD_Zahlen(10);
				switch (Stimmgeraet_Auswahl)
				{
				case 0:											// Falls Autom. ausgewähl wurde (also Stimmgeraet_Auswahl = 0), Cursor wird auf zurueck gesetzt.
					Toggle_Buchstabe (6,47,7);
					Cursor_Richtung = 15;
					break;
					
					default:										// Ansonsten kann man einen Ton auswaehlen mit linkem oder rechten Tastendruck.
					if (Saite == 0)
					{
						LCD_GotoXY(48,3);
						LCD_Zahlen(12);
					}
					
					if (Saite >= 1 && Saite <= 4)
					{
						LCD_GotoXY(48,3);
						LCD_Zahlen(12);
						LCD_GotoXY(30,3);
						LCD_Zahlen(13);
				    }
					
				    if (Saite == 5)
					{
						LCD_GotoXY(30,3);
						LCD_Zahlen(13);
					}
					Cursor_Richtung = 11;
					break;
				}											
				
				LCD_Update();
				
				break;
				
			case 11:												
				LCD_GotoXY(30,3);									// Pfeiltasten, um ausgewaehlten Ton werden geloescht.	
				LCD_Zahlen(10);
				LCD_GotoXY(48,3);
				LCD_Zahlen(10);
				
				switch (Stimmgeraet_Auswahl)
				{
				case 1:											// Falls Manu. Einstellung, wird Cursor auf zurueck gesetzt.
					Toggle_Buchstabe (6,47,7);
					Cursor_Richtung = 15;
					break;
					
				case 2:											// Für Tona. wird Cursor auf Start/ Stopp gesetzt.
					Toggle_Buchstabe (6,35,4);
					Cursor_Richtung = 12;
					break;
				}
				LCD_Update();
				break;
				
			case 12:											// Cursor wird auf zurueck gesetzt und Start/Stopp enttoggelt.
				Toggle_Buchstabe (6,35,4);
				Toggle_Buchstabe (6,47,7);
				LCD_Update();
				Cursor_Richtung = 15;
				break;
				
				//Menue_Metronom
			case 18:
				
				Toggle_Buchstabe (6,35,2);
				
				if(BeatsPerMinute > 40 && BeatsPerMinute < 300)		// Cursor bewegt sich in Zeile, in der man BPM einstellen kann. Der Wert wird von Pfeilen umgeben, um anzuzeigen, dass man den Joystick nach links und/oder rechts bewegen soll, um den Wert zu ändern.		
				{
					LCD_GotoXY(30,4);								// Pfeile werden in beide Richtungen angezeigt, wenn BPM zwischen 50 - 290 liegt (also z.B. < 90>).
					LCD_Zahlen(13);									// Pfeil nach links <.
					LCD_GotoXY(54,4);
					LCD_Zahlen(12);									// Pfeil nach rechts >.		
				}
				else if(BeatsPerMinute == 40)						// Nur Pfeil nach rechts > soll angezeigt werden, da minimaler Wert von BPM = 40.
				{
					LCD_GotoXY(54,4);
					LCD_Zahlen(12);
				}
				else if(BeatsPerMinute == 300)						// Nur Pfeil nach links > soll angezeigt werden, da maximaler Wert von BPM = 300.
				{
					LCD_GotoXY(30,4);
					LCD_Zahlen(13);
				}
				
				LCD_Update();
				
				Cursor_Richtung = 20;
				
				break;
				
			case 20:
				
				LCD_GotoXY(30,4);								// Die Pfeile neben dem Wert von BPM verschwinden, da man mit dem Cursor eine Zeile nach unten gewandert ist.		
				LCD_Zahlen(10);
				LCD_GotoXY(54,4);
				LCD_Zahlen(10);
				
				if( Takt == 2)						// Zwei Pfeile neben Taktart (<4/4>) entspricht Variable Takt =2.
				{
					LCD_GotoXY(36,5);
					LCD_Zahlen(13);
					LCD_GotoXY(60,5);
					LCD_Zahlen(12);
				}
				else if(Takt == 1)							// Nur Pfeil nach rechts > soll angezeigt werden, da "minimaler Wert" von  3/4-Takt (entspricht Variable Takt = 1).			
				{
					LCD_GotoXY(60,5);
					LCD_Zahlen(12);
				}
				else if(Takt == 3)							// Nur Pfeil nach links < soll angezeigt werden, da "maximaler Wert" von 8/8-Takt (entspricht Variable Takt = 3).
				{
					LCD_GotoXY(36,5);
					LCD_Zahlen(13);
				}
				LCD_Update();
				Cursor_Richtung = 21;
				break;
				
			case 21:
				LCD_GotoXY(36,5);							// Pfeile um Taktart werden gelöscht, da man mit dem Cursor auf Zeile mit Zurueck springt.	
				LCD_Zahlen(10);
				LCD_GotoXY(60,5);
				LCD_Zahlen(10);
				
				Toggle_Buchstabe (6,47,7);
				LCD_Update();
				Cursor_Richtung = 23;
				break;
				default:
				break;
			}		
			zaehlen =0;
			low=0;
		}
	}
}

void joystick_Up (void)																	// Funktioniert analog zur Funktion "joystick_Down"
{
	
	if (low>= 2)
	{
		zaehlen++;
		
		if (zaehlen >= 2)
		{
			switch (Cursor_Richtung)
			{
				
				//Hauptmenue
			case 3:
				Toggle_Buchstabe (6,53,3);
				Toggle_Buchstabe (6,71,2);
				LCD_Update();
				Cursor_Richtung = 2;
				break;
				
				
				
				//Menue_Stimmgeraet

								
			case 11:
				LCD_GotoXY(0,2);
				LCD_Zahlen(13);
				LCD_GotoXY(36,2);
				LCD_Zahlen(12);
				LCD_GotoXY(30,3);
				LCD_Zahlen(10);
				LCD_GotoXY(48,3);
				LCD_Zahlen(10);
								
				Cursor_Richtung = 10;
				LCD_Update();
				break;
								
			case 12:											// Cursor wird auf zurueck gesetzt und Start/Stopp enttoggelt.
				if (Saite == 0)
				{
					LCD_GotoXY(48,3);
					LCD_Zahlen(12);
				}
					
				if (Saite >= 1 && Saite <= 4)
				{
					LCD_GotoXY(48,3);
					LCD_Zahlen(12);
					LCD_GotoXY(30,3);
					LCD_Zahlen(13);
				}
					
				if (Saite == 5)
				{
					LCD_GotoXY(30,3);
					LCD_Zahlen(13);
				}
				
				Toggle_Buchstabe (6,35,4);
				Cursor_Richtung = 11;
				LCD_Update();
				break;			
								
			case 15:
				switch (Stimmgeraet_Auswahl)
				{	
				case 0:											// Falls Autom. ausgewähl wurde (also Stimmgeraet_Auswahl = 0), Cursor wird auf zurueck gesetzt.
					LCD_GotoXY(0,2);
					LCD_Zahlen(13);
					LCD_GotoXY(36,2);
					LCD_Zahlen(12);
					Cursor_Richtung = 10;
					break;
					
				case 1:								 // Manu. ausgewählt, daher springt Cursor auf Ton.
					if (Saite == 0)
					{
						LCD_GotoXY(48,3);
						LCD_Zahlen(12);
					}
					
					if (Saite >= 1 && Saite <= 4)
					{
						LCD_GotoXY(48,3);
						LCD_Zahlen(12);
						LCD_GotoXY(30,3);
						LCD_Zahlen(13);
					}
					
					if (Saite == 5)
					{
						LCD_GotoXY(30,3);
						LCD_Zahlen(13);
					}										

			
					Cursor_Richtung = 11;
					break;
					
				case 2:									   // Tonausgabe gewählt.	
					Toggle_Buchstabe (6,35,4);
					Cursor_Richtung = 12;
					break;
				}
			
				Toggle_Buchstabe (6,47,7);
				LCD_Update();
				break;

				
				// Menue_Metronom
			case 20:
				Toggle_Buchstabe (6,35,2);
				LCD_GotoXY(30,4);
				LCD_Zahlen(10);
				LCD_GotoXY(54,4);
				LCD_Zahlen(10);
				LCD_Update();
				Cursor_Richtung = 18;
				break;
				
			case 21:

				LCD_GotoXY(36,5);
				LCD_Zahlen(10);
				LCD_GotoXY(60,5);
				LCD_Zahlen(10);
				
				if(BeatsPerMinute > 40 && BeatsPerMinute < 300)
				{
					LCD_GotoXY(30,4);
					LCD_Zahlen(13);
					LCD_GotoXY(54,4);
					LCD_Zahlen(12);
				}
				else if(BeatsPerMinute == 40)
				{
					LCD_GotoXY(54,4);
					LCD_Zahlen(12);
				}
				else if(BeatsPerMinute == 300)
				{
					LCD_GotoXY(30,4);
					LCD_Zahlen(13);
				}


				LCD_Update();
				Cursor_Richtung = 20;
				break;
				
			case 23:
				if(Takt == 2)
				{
					LCD_GotoXY(36,5);
					LCD_Zahlen(13);
					LCD_GotoXY(60,5);
					LCD_Zahlen(12);
				}
				else if(Takt == 1)
				{
					LCD_GotoXY(60,5);
					LCD_Zahlen(12);
				}
				else if(Takt == 3)
				{
					LCD_GotoXY(36,5);
					LCD_Zahlen(13);
				}
				
				Toggle_Buchstabe (6,47,7);
				
				LCD_Update();
				Cursor_Richtung = 21;
				break;
				
				default:
				break;
			}		
			zaehlen =0;
			low=0;
		}
	}
}


void Toggle_Buchstabe (uint8_t x0, uint8_t x1,uint8_t Page)				// Funktion, um Zeile in Menue schwarz zu hinterlegen, um aufzuzeigen, wo man sich derzeit auf dem Menue mit dem Cursor befindet.
{
	uint8_t x;
	uint8_t y;
	for (y = 8* Page; y < 8*Page + 8; y++)								// Page beschreibt die Zeile (0-7), y den Pixel (0-63). Eine Zeile besteht in Y-Richtung aus 8 Pixeln.
		for (x = x0; x <= x1; x++)
			LCD_DrawPixel (x, y, 2);									// Funktion aus Demo.
}

void Werteausgabe (void)												// Funktion für Werteausgabe im Metronommenue, also Takt und BPM.
{
	volatile uint16_t a;	
	uint8_t b;
	uint8_t c[3];
	volatile int d;
												
	a = BeatsPerMinute;												// a besitzt Wert von BeatsPerMinute.
		
	if(BeatsPerMinute > 99)											// Für BPM > 98, ist d=2. Ist BPM zweistellig, ist d =1. Über d erfolgt Zugriff auf Matrix c[3] und deren Werte.
	{
		d = 2;
	}
	else
	{
		d = 1;
	}
		
	c[2] = 10;														// Anfangswert. Später wird die Funktion LCD_Zahlen aufgerufen, in der die Matrix Extrazeichen verwendet wird, in welcher ein Wert von 10 dem Löschen bzw. einer Nullmatrix entspricht.
		
	while(a)															// Während a ungleich 0 ist, wird Schleife ausgeführt.	
	{
		b=a%10;															// Damit erhält man die letzte Ziffer von BPM
		a=a/10;															// Wird benötigt, damit im nächsten Schleifendurchlauf die nächste Ziffer ermittelt werden kann.
		c[d]=b;															// In Matrix c wird an geeigneter Stelle der Wert der Ziffer geschrieben.	
		d--;															// d wird erniedrigt, sodass maximal die Schleife dreimal durchlaufen wird. Wenn d = 2, handelt es sich um eine dreistellige Zahl (Beats pro Minute).
	}
	
	if (Cursor_Richtung == 20)											// Damit folgende Schleife einfacher ist.
			d = 1;
	
	for(int i = 0; i <= d +1; i++)										// For-Schleife wird 1-3 dreimal durchlaufen entsprechend der Anzahl der Ziffern des BPM- oder Taktwertes.	
	{
		
		LCD_GotoXY(12 + 6*((Cursor_Richtung - 16) + i),Cursor_Richtung - 16);	// Darüber wird die geeignete Stelle ermittelt, an der die Ziffer auf dem Display ausgegeben werden soll.
		LCD_Zahlen(c[i]);														
	}
}

void Stimmgeraet_Zeiger (double Winkel, uint8_t mode)
{

// Mehrere Fallunterscheidungen sind zu beachten. 
// 1) Winkel: Ist Winkel im Zeigerdarstellungsbereich, also -90 < Winkel < 90 Grad?
// 2) In welchem mode befindet man sich? Möchte man Zeiger und +/- löschen (mode 0) oder Zeiger und +/- schreiben (mode 1) 	

//diese Abfrage fällt auch weg
//if( Winkel > -90 && Winkel < 90)								// Winkel im Darstellungsbereich					
//{
	double x;
	double y;
	uint8_t r;
	
		if (Winkel > 80 || Winkel < - 80)						// Groeßer oder kleiner 80 Grad.
		r = 34;													// Dann ist Radius 34 Pixel, damit Zeiger bis zum Halbkreis vollständig eingetragen wird.
		else {r=33;}											// Ansonsten 33 Pixel.
		
		x = r*sin(M_PI/180*Winkel);								// Berechnung für x-Wert der Zeigerdarstellung (Relativwert, Nullpunkt ist x = 90)
		y = r*cos(M_PI/180*Winkel);								// Berechnung für y-Wert der Zeigerdarstellung (Relativwert, Nullpunkt ist x = 62)
		
		LCD_DrawLine(90,62,90 + x,62 - y,mode);					// Zeiger in Abhängigkeit des Winkels zeichnen (modi= 1) /löschen (modi= 0).
		
		
		// 2 Fälle werden unterschieden: löschen oder Zeichnen
		switch (mode)
		{
			case 0:										// Löschen
			LCD_DrawCircle(90,62,34,1);					// Halbkreis zeichnen.
			LCD_DrawPixel(56,63,0);						// Zwei Pixel löschen, damit letzte Pixelreihe frei ist.
			LCD_DrawPixel(124,63,0);
			LCD_DrawLine (56, 62, 124, 62, 1);			// Untere waagerechte zeichnen für Abschluss des Halbkreises.
			LCD_DrawLine (114, 16, 114,22, 0);			// +/- löschen
			LCD_DrawLine (111, 19, 117, 19, 0);
			break;
			
			case 1:										// +/- zeichnen
			if (Winkel < 0 )
			{
				LCD_DrawLine (111, 19, 117, 19, 1);		// - zeichnen
			}
						
			if (Winkel > 0)								// + zeichnen
			{
				LCD_DrawLine (114, 16, 114,22, 1);
				LCD_DrawLine (111, 19, 117, 19, 1);
			}
			break;
		} 
	
// und der bereich außerhalb brauch auch nicht mehr abgefangen werden	
//	}
// 	else
//	{												// Falls Zeiger außerhalb von Zeigerdiagramm
// 		
// 		switch (mode)								
// 		{
// 			case 0:									// Erstmal +/- löschen, als Vorbereitung für neuen Eintrag.
// 			LCD_DrawLine (114, 16, 114,22, 0);
// 			LCD_DrawLine (111, 19, 117, 19, 0);
// 			break;
// 				
// 			case 1:	
// 			if (Winkel == 90)						// Frequenz zu groß, also wird + gezeichnet.
// 			{
// 			LCD_DrawLine (114, 16, 114,22, 1);
// 			LCD_DrawLine (111, 19, 117, 19, 1);	
// 			}
// 			if (Winkel == -90)						// Frequenz zu klein, also - zeichnen.
// 				LCD_DrawLine (111, 19, 117, 19, 1);
// 			break;
// 		}
// 	}
}
     

void Tonabspielen(uint16_t frequenz,uint16_t dauer){      //frequenz in Hz, dauer in ms
	uint16_t i;
	//Umschaltschwelle berechnen für gewünschte Frequenz
	OCR1A = F_CPU/(2*8*frequenz)-1;
	ctc_on();
	// der Wert von _delay_ms() muss fest sein, daher wird _delay_ms(1) so oft aufgerufen, bis die gewünschte Dauer erreicht wird 
	for (i=0; i<dauer; i++) _delay_ms(1);
	ctc_off();
}
            
void Tonstarten(uint16_t frequenz){		//frequenz in Hz
	//Umschaltschwelle berechnen für gewünschte Frequenz
	OCR1A = F_CPU/(2*8*frequenz)-1;
	ctc_on();
}
void Tonstoppen(void){
	ctc_off();
}

void ctc_on(void){						// Startet den Timer1 im CTC-Modus
	TCCR1A |= (1<<COM1A0);				// Toggle OC1A on Compare Match
	TCCR1B |= (1<<CS11) | (1<<WGM12);	// Prescaler = 8, CTC Mode aktiv
}
void ctc_off(void){						// Deaktiviert den Timer1
	TCCR1A = 0;
	TCCR1B = 0; 
}