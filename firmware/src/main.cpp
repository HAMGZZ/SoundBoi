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
int min_state = 0;
int max_state = 3;          //GLOBAL VARS for interupt


struct EngineData
{
    float rpm;
    float speed;
    float throttle;
    float load;
    float coolantTemp;
    float engineOnTime;
};

void rules(EngineData data);
void blinkLed(int *ledIndicatorTimer, int *blinkrate);
void buttonHandler();
void open();
void close();



int main()
{
    delay(1000);
    Serial.begin(115200);
    debug.begin(9600);
    debug.printf("SoundBoi for BLAT! - Lewis Hamilton September 2021\r\n");
    debug.printf("VERSION: %d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    debug.printf("Loading variables...");
    pinMode(BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonHandler, FALLING);
    
    EngineData data;
    data.rpm = 0;
    data.coolantTemp = 0;
    data.load = 0;
    data.speed = 0;
    data.throttle = 0;
    data.engineOnTime = 0;
    int ledIndicatorTimer = 0;
    int blinkrate = 0;
    bool connected = false;

    open();
    if(!elm.begin(Serial, true, 2000))
    {
        connected = false;
        debug.printf("COULD NOT CONNECT TO ELM327...");
        digitalWrite(CON_LED, LOW);
        min_state = 2;
        max_state = 3;
    }
    else
    {
        connected = true;
        debug.printf("CONNECTED TO ELM327");
        digitalWrite(CON_LED, HIGH);
        min_state = 0;
        max_state = 3;
    }

    while(true)
    {
        if(connected)
        {
            delay(100);
            EngineData tempData;
            tempData.rpm = elm.rpm();
            tempData.speed = elm.kph();
            tempData.throttle = elm.throttle();
            tempData.load = elm.engineLoad();
            tempData.coolantTemp = elm.engineCoolantTemp();
            tempData.engineOnTime = elm.runTime();

            if(elm.status == ELM_SUCCESS)
            {
                tempData = data;
                debug.printf("RPM: %lf SPD: %lf THTL: %lf\% LOAD: %lf\% TEMP: %lfC ON-TIME: %lfs\r\n", data.rpm, data.speed, data.throttle, data.load, data.coolantTemp, data.engineOnTime);
            }

            rules(data);
            blinkLed(&ledIndicatorTimer, &blinkrate);
            ledIndicatorTimer++;
        }
        else
        {
            rules(data);
            blinkLed(&ledIndicatorTimer, &blinkrate);
            ledIndicatorTimer++;
        }
    }
}

void rules(EngineData data)
{
    if(state == 0 && data.engineOnTime > ENGINE_WARMUP_TIME)
    {
        if((data.rpm > 3000 || data.load > 50 || data.throttle > 50) && data.speed < 90)
            open();
        else if((data.rpm >  4000 || data.load > 70 || data.throttle > 80) && data.speed >= 90)
            open();
        else if(data.speed < 6 && data.coolantTemp > 50)
            open();
        else
            close();
    }

    else if(state == 1 && data.engineOnTime > ENGINE_WARMUP_TIME)
    {
        if((data.rpm > 2200 || data.load > 30 || data.throttle > 30) && data.speed < 90)
            open();
        else if(data.speed < 15)
            open();
        else if(data.speed >= 90)
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

    if(data.rpm < 50 || data.engineOnTime < ENGINE_WARMUP_TIME)
    {
        open();
    }
}

void open()
{
    digitalWrite(OUTPUT_PIN, HIGH);
    digitalWrite(IND_LED, HIGH);
}

void close()
{
    digitalWrite(OUTPUT_PIN, LOW);
}

void blinkLed(int *ledIndicatorTimer, int *blinkrate)
{
    if(*ledIndicatorTimer > *blinkrate)
    {
        if(state == 0)
        {
            *blinkrate = 10;
            *ledIndicatorTimer = 0;
            digitalWrite(IND_LED, !digitalRead(IND_LED));
        }
        else if(state == 1)
        {
            *blinkrate = 2;
            *ledIndicatorTimer = 0;
            digitalWrite(IND_LED, !digitalRead(IND_LED));
        }
        else if(state == 2)
        {
            *blinkrate = 0;
            *ledIndicatorTimer = 0;
            digitalWrite(IND_LED, HIGH);
        }
        else if(state == 3)
        {
            *blinkrate = 0;
            *ledIndicatorTimer = 0;
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
      if(state > max_state)
      {
          state = 0;
      }
      if(state < min_state)
      {
          state = min_state;
      }
  }
  last_interrupt_time = interrupt_time;
}