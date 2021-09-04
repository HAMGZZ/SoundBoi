/*
 * Exhaust controller using hc-05 and elm327 module with an atmega328p
 */

#include <Arduino.h>
#include "BluetoothSerial.h"
#include "ESP32Servo.h"
#include "ELMduino.h"

#define DONGLE_NAME             "OBDII"     //Name of Bluetooth OBDII dongle

#define DEBUG_PORT              Serial      //Serial port for debug (USB)
#define ELM_PORT                SerialBT    //Serial port for bluetooth

#define ONBOARD_LED             D9          //Onboard LED pin

#define BUTTON                  D3          //Pin for button in
#define MOTOR1                  D2          //Pin for motor 1 out
#define MOTOR2                  D4          //Pin for motor 2 out

#define ENGINE_WARMUP_TIME      10          //Number of seconds before valves close after the engine starts

BluetoothSerial SerialBT;

Servo servo1;
Servo servo2;

ELM327 elm;

int state = 0;
bool connected = false;

uint32_t rpm = 0;
uint32_t speed = 0;
uint32_t throttle = 0;
uint32_t load = 0;
uint32_t coolantTemp = 0;
uint32_t engineOnTime = 0;

uint32_t ledIndicatorTimer = 0;
uint32_t blinkrate = 0;

bool rules();
void open();
void close();
void buttonHandler();
void blinkLed();
int mapRange(int a1,int a2,int b1,int b2,int s);

void setup()
{
    DEBUG_PORT.begin(9600);
    ELM_PORT.begin("ESP32", true);
    DEBUG_PORT.println("Exhaust valve controlled... \n\rOpening valves....");
    servo1.attach(MOTOR1);
    servo2.attach(MOTOR2);
    pinMode(BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonHandler, FALLING);
    open();
    DEBUG_PORT.println("Connecting to ELM327...");
    if(!ELM_PORT.connect(DONGLE_NAME))
    {
        DEBUG_PORT.println("Could not connect to ELM327 - Phase 1");
        DEBUG_PORT.println("Entering manual mode...");
        close();
        connected = false;
    }
    else if(!elm.begin(ELM_PORT, true, 2000))
    {
        DEBUG_PORT.println("Could not connect to ELM327 - Phase 2");
        DEBUG_PORT.println("Entering manual mode...");
        close();
        connected = false;
    }
    else
    {
        DEBUG_PORT.println("Connected!");
        connected = true;
    }
}

void loop()
{
    delay(100);
    float tempRPM = elm.rpm();
    float tempSpeed = elm.kph();
    float tempThrottle = elm.throttle();
    float tempLoad = elm.engineLoad();
    float tempCoolantTemp = elm.engineCoolantTemp();
    float tempEngineOnTime = elm.runTime();


    if(elm.status == ELM_SUCCESS)
    {
        rpm = (uint32_t)tempRPM;
        speed = (uint32_t)tempSpeed;
        throttle = (uint32_t)tempThrottle;
        load = (uint32_t)tempLoad;
        coolantTemp = (uint32_t)tempCoolantTemp;
        engineOnTime = (uint32_t)tempEngineOnTime;
        DEBUG_PORT.printf("RPM: %d\tSPEED: %d\t THROTTLE: %d\t LOAD: %d\t coolantTemp: %d\r", rpm, speed, throttle, load, coolantTemp);
    }

    rules();
    blinkLed();
    ledIndicatorTimer++;
    
}

bool rules()
{
    if(state == 0 && engineOnTime > ENGINE_WARMUP_TIME)
    {
        if((rpm > 3000 || load > 50 || throttle > 50) && speed < 90)
            open();
        else if((rpm >  4000 || load > 70 || throttle > 80) && speed >= 90)
            open();
        else if(speed < 6 && coolantTemp > 50)
            open();
        else
            close();
    }


    else if(state == 1 && engineOnTime > ENGINE_WARMUP_TIME)
    {
        if((rpm > 1900 || load > 30 || throttle > 30) && speed < 90)
            open();
        else if(speed < 15)
            open();
        else if(speed >= 90)
        {
            state = 0;
        }
        else
            close();
    }


    else if(state == 2)
    {
        open();
    }


    else if(state == 3)
    {
        close();
    }

    
    else if(state == 4 && engineOnTime > ENGINE_WARMUP_TIME)
    {
        int openVal = mapRange(0, 100, 0, 90, throttle);
        servo1.write(openVal);
        servo2.write(openVal);

    }

    if(rpm < 50 || engineOnTime < ENGINE_WARMUP_TIME)
    {
        open();
    }
    return true;
}

void open()
{
    servo1.write(0);
    servo2.write(0);
}

void close()
{
    servo1.write(90);
    servo2.write(90);
}

void blinkLed()
{
    if(ledIndicatorTimer > blinkrate)
    {
        if(state == 0)
        {
            blinkrate = 10;
            ledIndicatorTimer = 0;
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
        else if(state == 1)
        {
            blinkrate = 2;
            ledIndicatorTimer = 0;
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
        else if(state == 2)
        {
            blinkrate = 0;
            ledIndicatorTimer = 0;
            digitalWrite(LED_BUILTIN, HIGH);
        }
        else if(state == 3)
        {
            blinkrate = 0;
            ledIndicatorTimer = 0;
            digitalWrite(LED_BUILTIN, LOW);
        }
        else if(state == 4)
        {
            blinkrate = 5;
            ledIndicatorTimer = 0;
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }

    }
}

int mapRange(int a1,int a2,int b1,int b2,int s)
{
	return b1 + (s-a1)*(b2-b1)/(a2-a1);
}

void buttonHandler()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
      state++;
      if(state > 4)
      {
          state = 0;
      }
  }
  last_interrupt_time = interrupt_time;
}