/*
* Description: Adjust the speed of FAN Unit through PWM.
* https://docs.m5stack.com/en/core/m5stickc_plus
* https://github.com/m5stack/m5-docs/blob/master/docs/en/api/button.md
* LCD screen 	1.14 inch, 135*240 Colorful TFT LCD, ST7789v2
*/

#include <M5StickCPlus.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define GPIO_DS18B20_0     GPIO_NUM_32 // SDA Yellow  (CONFIG_ONE_WIRE_GPIO)
//#define MAX_DEVICES          (8)
//#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
//#define SAMPLE_PERIOD        (1000)   // milliseconds
const int oneWireBus = 33; //GPIO_DS18B20_0;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);


//const int pulse_width_pin = 36;
const int pulse_width_pin = GPIO_NUM_0;
//const int motor_pin = 32; // GROVE PORT GPIO32 (SDA) : G/5V/G32/G33, GPIO_NUM_32
const int motor_pin = 26;
int freq            = 200;
int ledChannel      = 0;
int fanChannel      = 2;
int resolution      = 10;
unsigned long lastMsg = 0;
unsigned long lastLed = 0;
unsigned int pwmLed = 0;
unsigned int pwmMotor = 110;

unsigned long lastRising = 0;
long pulseWidth = 0;
long pulsePeriod = 0;
const long max_period = 13333;

/* PWM Levels
Descrição
Subida: 1 W / % PWM
0-70 % <9333 Potência [W]
25% 3333 Potência 25 W (Max)
2: 95% 12666 Reserva (paragem)
3: 90% 12000 Paragem de alarme: avaria, circulador bloqueado
4: 85 % 11333 Paragem de alarme: avaria elétrica
5: 80% 10666 Aviso
6: 70 % 9333 Saturação
*/

long countRising = 0;
long countFalling = 0;
//bool speed = false;
void IRAM_ATTR pulse_width_ris_isr(void);
void IRAM_ATTR pulse_width_fall_isr(void) {
    long tmp;
    detachInterrupt(pulse_width_pin);
    tmp =  micros() - lastRising;
   // if ((tmp > 100) && (tmp < 3000))
        pulseWidth =  tmp;
    if(countRising > 0)
        attachInterrupt(pulse_width_pin, pulse_width_ris_isr, RISING);
    countFalling++;
}

void IRAM_ATTR pulse_width_ris_isr(void) {
    pulsePeriod =  micros() - lastRising;
    lastRising = micros();
    detachInterrupt(pulse_width_pin);
    //pulseWidth = 0;
    countRising--;
    if(countRising > 0)
        attachInterrupt(pulse_width_pin, pulse_width_fall_isr, FALLING);
}

void setup() {
    // put your setup code here, to run once:
    M5.begin();
    Serial.begin(115200);
    M5.Lcd.setRotation(3);
    //setCursor(int16_t x0, int16_t y0, uint8_t font)
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(25, 0, 2);
    M5.Lcd.println("Grundfos ALPHA1L");
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.print("PWM: ");
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("Pulse: ");
    ledcSetup(fanChannel, freq, resolution);
    ledcAttachPin(motor_pin, fanChannel);
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(M5_LED, ledChannel);

    //https://docs.m5stack.com/en/core/m5stickc_plus
    //G36/G25 share the same port, when one of the pins is used, 
    //the other pin should be set as a floating input
    //
    pinMode(pulse_width_pin, INPUT_PULLUP);
    pinMode(GPIO_NUM_36, INPUT_PULLUP);
    gpio_pulldown_dis(GPIO_NUM_25);
    gpio_pullup_dis(GPIO_NUM_25);

    delay(100);
    //delay(100);
     // Create a 1-Wire bus, using the RMT timeslot driver

    // Start the DS18B20 sensor
    sensors.begin();

    while(!Serial && millis() < 5000);
    Serial.println("\nStarting!");
}

void loop() {
    float motorPower;
    M5.update();
    if (M5.BtnA.wasReleased()) {  // If the button A is pressed. 
        pwmMotor = 112;
        Serial.print("A, PWM:");
        Serial.println(pwmMotor);
        M5.Lcd.setCursor(80, 20);
        //M5.Lcd.print("PWM: ");
        M5.Lcd.print(pwmMotor);
        M5.Lcd.setCursor(120, 20);
        M5.Lcd.print('A');
    } else if (M5.BtnB.wasReleased()) {  // If the button B is pressed. 
        pwmMotor = 800;
        Serial.print("B, PWM:");
        Serial.println(pwmMotor);
        M5.Lcd.setCursor(80, 20);
        //M5.Lcd.print("PWM: ");
        M5.Lcd.print(pwmMotor);
        M5.Lcd.setCursor(120, 20);
        M5.Lcd.print('B');
    }

    //ledcWrite(ledChannel, 512);
    //ledcWrite(ledChannel, 10);
    unsigned long now =  millis();  // Obtain the host startup duration.
    if (now - lastLed > 53) {
        lastLed = now;
        pwmLed++;
        if(pwmLed > 900)
            pwmLed = 100;
        ledcWrite(ledChannel, pwmLed);
    //    pwmMotor++;
        //if(pwmMotor > 950)
        //    pwmMotor = 800;
        ledcWrite(fanChannel, pwmMotor);
    }
    if (now - lastMsg > 1000) {
        lastMsg = now;
        detachInterrupt(pulse_width_pin);
        countRising = 2;
        attachInterrupt(pulse_width_pin, pulse_width_ris_isr, RISING);
        Serial.print("Time:");
        Serial.print(now/1000);
        Serial.print(", PWM:");
        Serial.print(pwmMotor);
        Serial.print(" D:");
        Serial.print(pulsePeriod);
        Serial.print(" W:");
        Serial.print(pulseWidth);
        Serial.print(" C:");
        Serial.print(countRising);
        Serial.print(" F:");
        Serial.print(countFalling);
        
        motorPower = pulseWidth;
        motorPower = motorPower / 3333.0 * 25.0; // in W
        //if(pulsePeriod > 0.01)
        //pwmFloat = pwmFloat / pulsePeriod;
        Serial.print(" %:");
        Serial.println(motorPower);

        sensors.requestTemperatures();
        float temperatureC = sensors.getTempCByIndex(0);
        Serial.print(temperatureC);
        Serial.println("ºC");
        M5.Lcd.setCursor(80, 20);
        M5.Lcd.print("      ");
        M5.Lcd.setCursor(80, 20);
        M5.Lcd.print(pwmMotor);
        M5.Lcd.setCursor(80, 40);
        M5.Lcd.print(pulseWidth);
        if((pulseWidth > 10) && (pulseWidth < 3333)) {
            M5.Lcd.setCursor(10, 60);
            M5.Lcd.print("      ");
            M5.Lcd.setCursor(10, 60);
            M5.Lcd.print(motorPower,2);
            M5.Lcd.print(" W");
        }
        else {
            M5.Lcd.setCursor(120, 60);
            M5.Lcd.print("NV");
        }
        M5.Lcd.setCursor(10, 80);
        M5.Lcd.print("Period: ");
        M5.Lcd.print(pulsePeriod);
        /*
        if (speed)
            ledcWrite(fanChannel, 600);
        else
            ledcWrite(fanChannel, 300);
            */
        //speed = not speed;

    }
}
