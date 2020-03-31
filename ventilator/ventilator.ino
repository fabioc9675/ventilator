/*
  Name:		main.ino
  Created:	3/22/2020 19:13:49
  Author:	Helber Carvajal
*/

// Definiciones para controlar el shiel DFRobot quad motor driver
#define EV_01_P1        25  // equivale al pin 3 del shield, velocidad motor 1
// #define EV_01_P2        26  // equivale al pin 4 del shield, direccion motor 1 // movido del 17 para prueba
#define EV_02_P1        27  // equivale al pin 6 del shield, velocidad motor 4
// #define EV_02_P2        14  // equivale al pin 7 del shield, direccion motor 4
#define EV_03_P1        23  // equivale al pin 11 del shield, velocidad motor 2
// #define EV_03_P2        13  // equivale al pin 12 del shield, direccion motor 2

// Definiciones para el manejo del ADC
#define ADC_PRESS_1     39  // pin ADC para presion 1
#define ADC_PRESS_2     36  // pin ADC para presion 2
#define ADC_PRESS_3     34  // pin ADC para presion 3

//********DEFINICION DE PINES***********
#define A     19     //variable A a pin digital 2 (DT en modulo)
#define B     18     //variable B a pin digital 4 (CLK en modulo)
#define SW    5      //sw a pin digital 3 (SW en modulo)  

// Definiciones para controlar el sensor de flujo
#define FLANCO          18  // pin digital numero 2 para deteccion de flujo IRQ

#define AMP1       0.0308
#define OFFS1      -22.673
#define AMP2       0.0309
#define OFFS2      -32.354

// Calibracion de los sensores de presion
#define AMP1       0.0308
#define OFFS1      -22.673
#define AMP2       0.0309
#define OFFS2      -32.354
#define AMP3       0.0309
#define OFFS3      -32.354

// Variables de control del protocolo
#define RXD2 16
#define TXD2 17


// variables y parametros de impresion
String SERIALEQU;
String patientPress;
String circuitPress;
String patientFlow;
String circuitFlow;
String inspTime;
String expiTime;
String frequency;
String volume;

String RaspberryChain = "";

// estructura para el conteo de los pulsos
struct Fan {
	const uint8_t PIN;
	uint32_t numberOfPulses;
	bool aspa;
};

Fan fan1 = { FLANCO, 0, false };

// Definiciones del ADC
volatile int contADC = 0;
bool fl_ADC = false;
bool flagTimerInterrupt = false;
int ADC1_Value = 0;     //
int ADC2_Value = 0;
int ADC3_Value = 0;
float Pressure1 = 0;
float Pressure2 = 0;
float Pressure3 = 0;
int numeroPulsos = 0;

// variables para la atencion de interrupciones
volatile bool flagSwInterrupt = false;
volatile bool flagEncoderInterrupt_A = false;
volatile bool flagEncoderInterrupt_B = false;
volatile bool flagDettachInterrupt_A = false;
volatile bool flagDettachInterrupt_B = false;
volatile bool flagDetach = false;
volatile unsigned int contDetach = 0;


//***********************************
// datos para prueba de transmision
int pruebaDato = 0;
int second = 0;
int milisecond = 0;
// *********************************

// definiciones del ADC
float Pin[10] = { 0,0,0,0,0,0,0,0,0,0 };
float Pout[10] = { 0,0,0,0,0,0,0,0,0,0 };
float SPin = 0;
float SPout = 0;
float Peep = 0;
float Ppico = 0;
float UmbralPeep = 100;
float UmbralPpico = -100;

float Pin_max = 0;
float Pout_max = 0;
float Pin_min = 0;
float Pout_min = 0;
float dpmin = 0;
float dpmax = 0;
float Comp = 0;
float C1 = 1.3836;
float C2 = 38.2549;
float C3 = 291.7069;
float VT = 0;
float V_Comp[10] = { 0,0,0,0,0,0,0,0,0,0 };
float V_dpmax[10] = { 0,0,0,0,0,0,0,0,0,0 };
float V_dpmin[10] = { 0,0,0,0,0,0,0,0,0,0 };
float SComp = 0;
float Sdpmax = 0;
float Sdpmin = 0;

// Some global variables available anywhere in the program
volatile unsigned long startMillis;
volatile unsigned long currentMillis;
volatile float inspirationTime = 1.5;
volatile float expirationTime = 3;

// definiciones del timer
volatile int interruptCounter = 0;


// inicializacion del contador del timer
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// definicion de interrupciones
void IRAM_ATTR onTimer();  // funcion de interrupcion

void IRAM_ATTR swInterrupt();
void IRAM_ATTR encoderInterrupt_A();
void IRAM_ATTR encoderInterrupt_B();


void setup() {
	// Configuracion del timer a 1 kHz
	timer = timerBegin(0, 80, true);               // Frecuencia de reloj 80 MHz, prescaler de 80, frec 1 MHz
	timerAttachInterrupt(timer, &onTimer, true);
	timerAlarmWrite(timer, 1000, true);             // Interrupcion cada 1000 conteos del timer, es decir 100 Hz
	timerAlarmEnable(timer);                        // Habilita interrupcion por timer


	//Encoder setup
	pinMode(A, INPUT_PULLUP);    // A como entrada
	pinMode(B, INPUT_PULLUP);    // B como entrada
	pinMode(SW, INPUT_PULLUP);   // SW como entrada

	attachInterrupt(digitalPinToInterrupt(A), encoderInterrupt_A, FALLING);
	attachInterrupt(digitalPinToInterrupt(B), encoderInterrupt_B, FALLING);
	attachInterrupt(digitalPinToInterrupt(SW), swInterrupt, RISING);

	// configuracion del pin de interrupcion
	pinMode(fan1.PIN, INPUT_PULLUP);
	//attachInterrupt(fan1.PIN, isr, RISING);

	// inicializacion de los pines controladores de las EV como salidas
	pinMode(EV_01_P1, OUTPUT);  // PIN 3   velocidad
	//  pinMode(EV_01_P2, OUTPUT);  // PIN 4   direccion
	pinMode(EV_02_P1, OUTPUT);  // PIN 6   velocidad
	//  pinMode(EV_02_P2, OUTPUT);  // PIN 7   direccion
	pinMode(EV_03_P1, OUTPUT);  // PIN 12  velocidad
	//  pinMode(EV_03_P2, OUTPUT);  // PIN 11  direccion

	Serial.begin(115200);   // incializacion de comunicacion serie a 9600 bps
	Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
	lcd_setup();

	// inicializacion de los strings
	SERIALEQU = String("1234567890ABCD");
	patientPress = String("");
	circuitPress = String("");
	patientFlow = String("234.67");
	circuitFlow = String("245.67");
	inspTime = String("2.1");
	expiTime = String("3.5");
	frequency = String("20");
	volume = String("240");

	RaspberryChain = String("");
	lcd_show();

}

void loop() {
	//*******refresh display every 0.5 s************
	static unsigned long timeToShowLcd = 0;
	if ((millis() - timeToShowLcd) > 300) {
		timeToShowLcd = millis();
		// lcd_show();
	}
	//********************************************

	// switchRoutine();
	// encoderRoutine();
	currentMillis = millis();


	// *************************************************
	// **** Atencion a rutina de interrupcion por timer
	// *************************************************
	if (flagTimerInterrupt) {
		flagTimerInterrupt = false;
		interruptCounter++;
		contADC++;

		if (flagDettachInterrupt_A) {
			if (contDetach == 0 && flagDetach == false) {
				detachInterrupt(A);  // desactiva la interrupcion A
				flagDetach = true;
			}
			contDetach++;
			if (contDetach >= 150) {
				attachInterrupt(digitalPinToInterrupt(A), encoderInterrupt_A, FALLING);
				flagDettachInterrupt_A = false;
				flagDetach = false;
				contDetach = 0;
			}
		}

		if (flagDettachInterrupt_B) {
			if (contDetach == 0 && flagDetach == false) {
				detachInterrupt(B);  // desactiva la interrupcion A
				flagDetach = true;
			}
			contDetach++;
			if (contDetach >= 150) {
				attachInterrupt(digitalPinToInterrupt(B), encoderInterrupt_B, FALLING);
				flagDettachInterrupt_B = false;
				flagDetach = false;
				contDetach = 0;
			}
		}

		milisecond++;
		if (milisecond == 1000) {
			milisecond = 0;
			second++;
			if (second == 60) {
				second = 0;
			}
		}

		if (contADC == 50) {
			fl_ADC = true;
			contADC = 0;
		}
		if (interruptCounter == 1) {        // Inicia el ciclado abriendo electrovalvula de entrada y cerrando electrovalvula de salida
		  //  digitalWrite(EV_01_P2, HIGH);   // turn the LED on (HIGH is the voltage level)
			digitalWrite(EV_01_P1, HIGH);   // turn the LED on (HIGH is the voltage level)

			digitalWrite(EV_02_P1, LOW);   // turn the LED on (HIGH is the voltage level)
			//  digitalWrite(EV_02_P2, LOW);   // turn the LED on (HIGH is the voltage level)
			digitalWrite(EV_03_P1, LOW);   // turn the LED on (HIGH is the voltage level)
			//  digitalWrite(EV_03_P2, LOW);   // turn the LED on (HIGH is the voltage level)
			
			//Calculos
			Pin_min = SPin;
			Pout_min = SPout;

		}
		else if (interruptCounter == int(inspirationTime * 1000)) { // espera 1 segundo y cierra electrovalvula de entrada y abre electrovalvula de salida
		  //  digitalWrite(EV_01_P2, LOW);    // turn the LED off by making the voltage LOW
			digitalWrite(EV_01_P1, LOW);   // turn the LED on (HIGH is the voltage level)

			digitalWrite(EV_02_P1, HIGH);    // turn the LED off by making the voltage LOW
			//  digitalWrite(EV_02_P2, HIGH);   // turn the LED on (HIGH is the voltage level)
			digitalWrite(EV_03_P1, HIGH);    // turn the LED off by making the voltage LOW
			//  digitalWrite(EV_03_P2, HIGH);   // turn the LED on (HIGH is the voltage level)

			//Calculos
			Pin_max = SPin;
			Pout_max = SPout;
			dpmax = Pin_max - Pin_min;
			if (dpmax < 0) {
				dpmax = -dpmax;
			}
			dpmin = Pout_max - Pout_min;
			if (dpmin < 0) {
				dpmin = -dpmin;
			}
		}
		else if (interruptCounter == int(((inspirationTime + expirationTime) * 1000))) {
			interruptCounter = 0;
			Peep = UmbralPeep;
			UmbralPeep = 100;
			Ppico = UmbralPpico - 2;
			UmbralPpico = -100;
			Comp = (C1 * Peep * Peep) - (C2 * Peep) + C3;
			//Comp=(-14.52*Peep)+(198.75);

			//Filtrado media movil VT y COMP
			//-Almacenamiento de valores
			V_Comp[9] = Comp;
			V_dpmax[9] = dpmax;
			V_dpmin[9] = dpmin;
			//-Corrimiento inicial
			for (char i = 9; i >= 1; i--) {
				V_Comp[9 - i] = V_Comp[9 - i + 1];
				V_dpmax[9 - i] = V_dpmax[9 - i + 1];
				V_dpmin[9 - i] = V_dpmin[9 - i + 1];
			}
			//-Calculo promedio
			SComp = 0;
			Sdpmax = 0;
			Sdpmin = 0;
			for (char i = 0; i <= 9; i++) {
				SComp = SComp + V_Comp[i];
				Sdpmax = Sdpmax + V_dpmax[i];
				Sdpmin = Sdpmin + V_dpmin[i];
			}
			SComp = SComp / 10;
			Sdpmax = Sdpmax / 10;
			Sdpmin = Sdpmin / 10;
			VT = SComp * (Sdpmax - Sdpmin);
		}	
	}
	// Final interrupcion timer

	// *************************************************
	// **** Atencion a rutina de adquisicion ADC
	// *************************************************
	if (fl_ADC) {
		fl_ADC = false;
		ADC1_Value = analogRead(ADC_PRESS_1);
		ADC2_Value = analogRead(ADC_PRESS_2);
		ADC3_Value = analogRead(ADC_PRESS_3);
		// falta realizatr la conversion de adc a presion
		Pressure1 = AMP1 * float(ADC1_Value) + OFFS1;
		Pressure2 = AMP2 * float(ADC2_Value) + OFFS2;
		Pressure3 = AMP3 * float(ADC3_Value) + OFFS3;

		//Filtrado media movil
	//-Almacenamiento de valores
		Pin[9] = Pressure1;
		Pout[9] = Pressure2;
		//-Corrimiento inicial
		for (char i = 9; i >= 1; i--) {
			Pin[9 - i] = Pin[9 - i + 1];
			Pout[9 - i] = Pout[9 - i + 1];
		}
		//-Calculo promedio
		SPin = 0;
		SPout = 0;
		for (char i = 0; i <= 9; i++) {
			SPin = SPin + Pin[i];
			SPout = SPout + Pout[i];
		}
		SPin = SPin / 10;
		SPout = SPout / 10;
		//-Calculo Peep
		if (UmbralPeep > SPin) {
			UmbralPeep = SPin;
		}
		//-Calculo Ppico
		if (UmbralPpico < SPin) {
			UmbralPpico = SPin;
		}

		// conteo de los pulsos
		numeroPulsos = fan1.numberOfPulses;  //Numero de pulsos en 50ms
		fan1.numberOfPulses = 0;
		numeroPulsos = (numeroPulsos * 2000 / 75); //(#pulsos/1 s)=7.5*Q [L/min] (considera un decimal)

		patientPress = String(Pressure1);
		circuitPress = String(Pressure2);


		if (pruebaDato == 20) {
			pruebaDato = 0;
		}
		frequency = String(pruebaDato);
		volume = String(second);
		expiTime = String(inspirationTime * 1000);
		inspTime = String(expirationTime * 1000);

		pruebaDato++;

		// composicion de cadena
		RaspberryChain = SERIALEQU + ',' + patientPress + ',' + circuitPress + ',' + patientFlow + ',' + circuitFlow + ',' + inspTime + ',' + expiTime + ',' + frequency + ',' + volume;

		// Envio de la cadena de datos
		//Serial.println(RaspberryChain);
		//Serial2.println(RaspberryChain);

		/*Serial.print("Pres1 = ");
		  Serial.print(ADC1_Value);
		  Serial.print(", Pres2 = ");
		  Serial.print(ADC2_Value + 180);
		  Serial.print(", Puls = ");
		  Serial.println(numeroPulsos);*/
	}
	// Final muestreo ADC


	// *************************************************
	// **** Atencion a rutina de interrupcion por switch
	// *************************************************
	if (flagSwInterrupt) {
		flagSwInterrupt = false;
		// Atencion a la interrupcion
		swInterruptAttention();
	}
	// Final interrupcion Switch


	// *************************************************
	// **** Atencion a rutina de interrupcion por encoder
	// *************************************************
	if (flagEncoderInterrupt_A) {
		flagEncoderInterrupt_A = false;
		// Atencion a la interrupcion
		encoderInterruptAttention_A();
	}
	// Final interrupcion encoder


	// *************************************************
	// **** Atencion a rutina de interrupcion por encoder
	// *************************************************
	if (flagEncoderInterrupt_B) {
		flagEncoderInterrupt_B = false;
		// Atencion a la interrupcion
		encoderInterruptAttention_B();
	}
	// Final interrupcion encoder

}

void IRAM_ATTR onTimer() {
	portENTER_CRITICAL(&timerMux);
	//Serial.println("I am inside onTimer");
	flagTimerInterrupt = true;
	portEXIT_CRITICAL(&timerMux);
}
