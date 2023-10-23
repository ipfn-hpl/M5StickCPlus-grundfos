/*
Description: Adjust the speed of FAN Unit through PWM.
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
unsigned int pwmMotor = 800;

unsigned long lastRising = 0;
long pulseWidth = 0;
long pulsePeriod = 0;

unsigned long countRising = 0;
unsigned long countFalling = 0;
bool speed = false;
void IRAM_ATTR pulse_width_fall_isr(void) {
    detachInterrupt(pulse_width_pin);
    pulseWidth =  micros() - lastRising;
    countFalling++;
}

void IRAM_ATTR pulse_width_ris_isr(void) {
    pulsePeriod =  micros() - lastRising;
    lastRising = micros();
    detachInterrupt(pulse_width_pin);
    pulseWidth = 0;
    countRising++;

    attachInterrupt(pulse_width_pin, pulse_width_fall_isr, FALLING);
}

void setup() {
    // put your setup code here, to run once:
    M5.begin();
    Serial.begin(115200);
    M5.Lcd.setRotation(3);
    //setCursor(int16_t x0, int16_t y0, uint8_t font)
    M5.Lcd.setCursor(25, 0, 2);
    M5.Lcd.println("Grundfos ALPHA1L");
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
    float pwmFloat;
    // put your main code here, to run repeatedly:
    //ledcWrite(ledChannel, 512);
    //ledcWrite(ledChannel, 10);
    unsigned long now =  millis();  // Obtain the host startup duration.
    if (now - lastLed > 53) {
        lastLed = now;
        pwmLed++;
        if(pwmLed > 900)
            pwmLed = 100;
        ledcWrite(ledChannel, pwmLed);
        pwmMotor++;
        if(pwmMotor > 950)
            pwmMotor = 800;
        ledcWrite(fanChannel, pwmMotor);
    }
    if (now - lastMsg > 1000) {
        lastMsg = now;
        detachInterrupt(pulse_width_pin);
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
        
        pwmFloat = pulseWidth;
        pwmFloat = pwmFloat / pulsePeriod;
        Serial.print(" %:");
        Serial.println(pwmFloat);

        sensors.requestTemperatures();
        float temperatureC = sensors.getTempCByIndex(0);
        Serial.print(temperatureC);
        Serial.println("ºC");

        M5.Lcd.setCursor(10, 20);
        M5.Lcd.print("PWM: ");
        M5.Lcd.print(pwmMotor);
        M5.Lcd.setCursor(10, 40);
        M5.Lcd.print("Pulse: ");
        M5.Lcd.println(pulseWidth);
        M5.Lcd.setCursor(10, 60);
        M5.Lcd.print("pF: ");
        M5.Lcd.println(pwmFloat);
        /*
        if (speed)
            ledcWrite(fanChannel, 600);
        else
            ledcWrite(fanChannel, 300);
            */
        speed = not speed;

    }
}
