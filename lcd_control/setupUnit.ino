/*
 Name:		lcd_control.ino
 Created:	4/3/2020 11:40:24
 Author:	Helber Carvajal
*/

#include <LiquidCrystal_I2C.h>

//**********VALORES MAXIMOS**********
#define MENU_QUANTITY 3
#define MAX_FREC 40
#define MAX_RIE 4
#define MAX_PRESION 40
#define MAX_FLUJO 40

// UART Configuration
#define RXD1 3      // RX UART 0
#define TXD1 1      // TX UART 0
#define RXD2 16     // RX UART 2
#define TXD2 17     // TX UART 2

// variables para la medicion de milisegundos del programa
unsigned long tiempoInterrupcion = 0;
static unsigned long ultimaInterrupcion = 0;
static unsigned long debounceUltima_A = 0;
unsigned long tiempoDebounce_A = 0;
static unsigned long debounceUltima_B = 0;
unsigned long tiempoDebounce_B = 0;
unsigned int debounceVal = 1;  // valor para el antirrebote

//********ENCODER SETUP***********
#define A     19     //variable A a pin digital 2 (DT en modulo)
#define B     18     //variable B a pin digital 4 (CLK en modulo)
#define SW    5      //sw a pin digital 3 (SW en modulo)  

volatile unsigned int menu = 0;

// Global al ser usada en loop e ISR (encoder)
unsigned long tiempo1 = 0;
float relacionIE = 2;
String relacion_IE = "1:4.1";
float maxPresion = 4;
float maxFlujo = 4;
unsigned int frecRespiratoria = 12;

// Banderas utilizadas en las interrupciones
boolean insideMenuFlag = false;
boolean flagPresion = false;
boolean flagFlujo = false;
boolean flagFrecuencia = false;
boolean flagIE = false;
int flagEncoder = 0;

// variables para la atencion de interrupciones
bool flagTimerInterrupt = false;
volatile bool flagSwInterrupt = false;
volatile bool flagEncoderInterrupt_A = false;
volatile bool flagEncoderInterrupt_B = false;
volatile bool flagDettachInterrupt_A = false;
volatile bool flagDettachInterrupt_B = false;
volatile bool flagDetach = false;
volatile unsigned int contDetach = 0;

float Peep = 0;
float Ppico = 0;
float VT = 0;

LiquidCrystal_I2C lcd(0x27, 20, 4);   // Objeto LCD

// inicializacion del contador del timer
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Interrupt routines
void IRAM_ATTR onTimer();  // funcion de interrupcion
void IRAM_ATTR swInterrupt();
void IRAM_ATTR encoderInterrupt_A();
void IRAM_ATTR encoderInterrupt_B();

// the setup function runs once when you press reset or power the board
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

    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    lcd.init();
    lcd.backlight();
    lcd_show();
}

// the loop function runs over and over again until power down or reset
void loop() {
    //Serial.print("I am ESP2\n");
    //delay(500);
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
    }// Final interrupcion encoder
    
  
}

void IRAM_ATTR onTimer() {
    portENTER_CRITICAL(&timerMux);
    //Serial.println("I am inside onTimer");
    flagTimerInterrupt = true;
    portEXIT_CRITICAL(&timerMux);
}


void lcd_show() {

    if (relacionIE > 0) {
        relacion_IE = "1:" + String(relacionIE, 1);
    }
    else {
        relacion_IE = String(-relacionIE, 1) + ":1";
    }

    switch (menu) {
    case 0:
        // lcd.home();
        lcd.setCursor(0, 0);
        lcd.print("  InnspiraMED UdeA  ");
        lcd.setCursor(0, 1);
        lcd.print("FR        PIP     ");
        lcd.setCursor(4, 1);
        lcd.print(frecRespiratoria);
        lcd.setCursor(14, 1);
        lcd.print(String(Ppico, 1));
        lcd.setCursor(0, 2);
        lcd.print("I:E       PEEP      ");
        lcd.setCursor(4, 2);
        lcd.print(relacion_IE);
        lcd.setCursor(15, 2);
        lcd.print(String(Peep, 1));
        lcd.setCursor(0, 3);
        lcd.print("VT                  ");
        lcd.setCursor(4, 3);
        lcd.print(String(VT, 1));
        break;
    case 1:

        lcd.setCursor(0, 0);
        lcd.print("   Configuracion    ");
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        if (flagFrecuencia) {

            lcd.print(" ");
            lcd.write(126);
            lcd.print("Frec:            ");
        }
        else {
            lcd.print("  Frec:             ");
        }
        lcd.setCursor(10, 2);
        lcd.print(frecRespiratoria);
        lcd.setCursor(0, 3);
        if (flagIE) {
            lcd.print(" ");
            lcd.write(126);
            lcd.print("I:E:              ");
        }
        else {
            lcd.print("  I:E:              ");
        }
        lcd.setCursor(10, 3);
        lcd.print(relacion_IE);

        break;
    case 2:
        lcd.setCursor(0, 0);
        lcd.print("      Alarmas       ");
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        if (flagPresion) {
            lcd.print(" ");
            lcd.write(126);
            lcd.print("Presion:            ");
        }
        else {
            lcd.print("  Presion:            ");
        }
        lcd.setCursor(12, 2);
        lcd.print(maxPresion);
        lcd.setCursor(0, 3);
        if (flagFlujo) {
            lcd.print(" ");
            lcd.write(126);
            lcd.print("Flujo:            ");
            Serial.println("Flujo:  ");
        }
        else {
            lcd.print("  Flujo:            ");
        }
        lcd.setCursor(12, 3);
        lcd.print(maxFlujo);
        break;
    default:
        lcd.setCursor(0, 0);
        lcd.print("                    ");
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("Default: ");
        lcd.print(12);                      // Escribimos el numero de segundos trascurridos
        lcd.print("           ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        break;
    }
    //Serial.println("I am in lcd_show()");
}


void IRAM_ATTR swInterrupt() {
    portENTER_CRITICAL_ISR(&mux);
    flagSwInterrupt = true;
    tiempoInterrupcion = millis();
    portEXIT_CRITICAL_ISR(&mux);
}

void swInterruptAttention() {
    if (tiempoInterrupcion - ultimaInterrupcion > 100) {
        // portENTER_CRITICAL_ISR(&mux);
        switchRoutine();
        // Serial.println(tiempoInterrupcion - ultimaInterrupcion);
        // portEXIT_CRITICAL_ISR(&mux);
        ultimaInterrupcion = tiempoInterrupcion;
    }
}

// Interrupcion por encoder
void IRAM_ATTR encoderInterrupt_A() {
    portENTER_CRITICAL_ISR(&mux);
    flagEncoderInterrupt_A = true;
    tiempoDebounce_A = millis();
    portEXIT_CRITICAL_ISR(&mux);
}
// Interrupcion por encoder
void IRAM_ATTR encoderInterrupt_B() {
    portENTER_CRITICAL_ISR(&mux);
    flagEncoderInterrupt_B = true;
    tiempoDebounce_B = millis();
    portEXIT_CRITICAL_ISR(&mux);
}

void encoderInterruptAttention_A() {
    if (tiempoDebounce_A - debounceUltima_A > debounceVal) {
        // Serial.print("A = ");
        // Serial.println(tiempoDebounce_A - debounceUltima_A);
        if ((digitalRead(B) == HIGH) && (digitalRead(A) == LOW)) {
            // portENTER_CRITICAL_ISR(&mux);
            flagEncoder = 0;
            flagDettachInterrupt_B = true;
            contDetach = 0;
            encoderRoutine();
            // portEXIT_CRITICAL_ISR(&mux);
        }
        debounceUltima_A = tiempoDebounce_A;
    }
}

void encoderInterruptAttention_B() {
    if (tiempoDebounce_B - debounceUltima_B > debounceVal) {
        // Serial.print("B = ");
        // Serial.println(tiempoDebounce_B - debounceUltima_B);
        if ((digitalRead(A) == HIGH) && (digitalRead(B) == LOW)) {
            // portENTER_CRITICAL_ISR(&mux);
            flagEncoder = 1;
            flagDettachInterrupt_A = true;
            contDetach = 0;
            encoderRoutine();
            // portEXIT_CRITICAL_ISR(&mux);
        }
        debounceUltima_B = tiempoDebounce_B;
    }
}

void encoderRoutine() {
    static int cont = 0;
    // Incremento
    if (flagEncoder == 1) {
        if (insideMenuFlag == false) {
            menu++;
            if (menu < 0 || menu > MENU_QUANTITY - 1)
                menu = 0;
            //Serial.println("menu = " + String(menu));
        }
        else {
            switch (menu) {
            case 1:
                if (flagFrecuencia) {
                    frecRespiratoria++;
                    if (frecRespiratoria > MAX_FREC) {
                        frecRespiratoria = MAX_FREC;
                    }
                }
                else if (flagIE) {

                    relacionIE = relacionIE + 0.1;
                    if (relacionIE >= MAX_RIE) {
                        relacionIE = MAX_RIE;;
                    }
                    if (relacionIE > -1.0 && relacionIE < 0) {
                        relacionIE = 1;
                    }
                }
                break;
            case 2:
                if (flagPresion) {
                    maxPresion++;
                    if (maxPresion > MAX_PRESION) {
                        maxPresion = MAX_PRESION;
                    }
                }
                else if (flagFlujo) {
                    maxFlujo++;
                    if (maxFlujo > MAX_FLUJO) {
                        maxFlujo = MAX_FLUJO;
                    }
                }
                break;
            }
        }
    }
    // Decremento
    else if (flagEncoder == 0) {
        if (insideMenuFlag == false) {
            menu--;
            if (menu < 0 || menu > MENU_QUANTITY - 1)
                menu = MENU_QUANTITY - 1;
        }
        else {
            switch (menu) {
            case 1:
                if (flagFrecuencia) {
                    frecRespiratoria--;
                    if (frecRespiratoria > MAX_FREC) {
                        frecRespiratoria = 0;
                    }
                }
                else if (flagIE) {
                    relacionIE = relacionIE - 0.1;
                    if (relacionIE <= -MAX_RIE) {
                        relacionIE = -MAX_RIE;
                    }
                    if (relacionIE > 0 && relacionIE < 1) {
                        relacionIE = -1;
                    }
                }
                break;
            case 2:
                if (flagPresion) {
                    maxPresion--;
                    if (maxPresion > MAX_PRESION) {
                        maxPresion = MAX_PRESION;
                    }
                }
                else if (flagFlujo) {
                    maxFlujo--;
                    if (maxFlujo > MAX_FLUJO) {
                        maxFlujo = MAX_FLUJO;
                    }
                }
                break;
            }
        }
    }
    // portENTER_CRITICAL_ISR(&mux);
    flagEncoder = 3;
    lcd_show();
    // portEXIT_CRITICAL_ISR(&mux);
}

void switchRoutine() {
    // Serial.println("SW MENU");
    if (menu == 1 && !insideMenuFlag) {
        insideMenuFlag = !insideMenuFlag;
        flagFrecuencia = true;
        // Serial.println("SW MENU 1_1");
    }
    else if (menu == 1 && flagFrecuencia) {
        flagFrecuencia = false;
        flagIE = true;
        // Serial.println("SW MENU 1_2");
    }
    else if (menu == 1 && flagIE) {
        insideMenuFlag = !insideMenuFlag;
        flagIE = false;
        Serial.println(String(frecRespiratoria) + ";" + relacion_IE);
        // Serial.println("SW MENU 1_3");
    }
    else if (menu == 2 && !insideMenuFlag) {
        insideMenuFlag = !insideMenuFlag;
        flagPresion = true;
        //Serial.println("Config maxPres");
    }
    else if (menu == 2 && flagPresion) {
        flagPresion = false;
        flagFlujo = true;
        //Serial.println("Config maxFlujo");
    }
    else if (menu == 2 && flagFlujo) {
        insideMenuFlag = !insideMenuFlag;
        flagFlujo = false;
    }
    lcd_show();
}
