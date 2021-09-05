/*
 * SoundBoi Exahust controller attiny1614 to interface with BLAT!
 * Version 1.0.0
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ELMduino.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define BUTTON                  2          //Pin for button in
#define OUTPUT_PIN              1          //PIN FOR OUTPUT HIGH = OPEN

#define ENGINE_WARMUP_TIME      10          //Number of seconds before valves close after the engine starts

#define IND_LED                 6
#define CON_LED                 7


ELM327 elm;
SoftwareSerial debug(9, 10);

int state = 0;

uint32_t rpm = 0;
uint32_t speed = 0;
uint32_t throttle = 0;
uint32_t load = 0;
uint32_t coolantTemp = 0;
uint32_t engineOnTime = 0;

uint32_t ledIndicatorTimer = 0;
uint32_t blinkrate = 0;

bool connected = false;

bool rules();
void buttonHandler();
void blinkLed();
void open();
void close();

int main()
{
    delay(1000);
    Serial.begin(115200);
    debug.begin(9600);
    debug.printf("SoundBoi for BLAT! - Lewis Hamilton September 2021\r\n");
    debug.printf("VERSION: %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    pinMode(BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonHandler, FALLING);
    open();
    if(!elm.begin(Serial, true, 2000))
    {
        connected = false;
        debug.printf("COULD NOT CONNECT TO ELM327...");
        digitalWrite(CON_LED, LOW);
    }
    else
    {
        connected = true;
        debug.printf("CONNECTED TO ELM327");
        digitalWrite(CON_LED, HIGH);
    }

    while(true)
    {
        if(connected)
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
                debug.printf("RPM: %lf SPD: %lf THTL: %lf\% LOAD: %lf\% TEMP: %lfC ON-TIME: %lfs\r\n");
            }

            rules();
            blinkLed();
            ledIndicatorTimer++;
        }
    }
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

    if(rpm < 50 || engineOnTime < ENGINE_WARMUP_TIME)
    {
        open();
    }
    return true;
}

void open()
{
    digitalWrite(OUTPUT_PIN, HIGH);
}

void close()
{
    digitalWrite(OUTPUT_PIN, LOW);
}

void blinkLed()
{
    if(ledIndicatorTimer > blinkrate)
    {
        if(state == 0)
        {
            blinkrate = 10;
            ledIndicatorTimer = 0;
            digitalWrite(IND_LED, !digitalRead(IND_LED));
        }
        else if(state == 1)
        {
            blinkrate = 2;
            ledIndicatorTimer = 0;
            digitalWrite(IND_LED, !digitalRead(IND_LED));
        }
        else if(state == 2)
        {
            blinkrate = 0;
            ledIndicatorTimer = 0;
            digitalWrite(IND_LED, HIGH);
        }
        else if(state == 3)
        {
            blinkrate = 0;
            ledIndicatorTimer = 0;
            digitalWrite(IND_LED, LOW);
        }

    }
}


void buttonHandler()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
      state++;
      if(state > 3)
      {
          state = 0;
      }
  }
  last_interrupt_time = interrupt_time;
}