/*
 * SoundBoi Exahust controller attiny1614 to interface with BLAT!
 * Version 1.0.1
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ELMduino.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 1

#define BUTTON                  2           //Pin for button in
#define OUTPUT_PIN              1           //PIN FOR OUTPUT HIGH = OPEN

#define ENGINE_WARMUP_TIME      1           //Number of seconds before valves close after the engine starts

#define IND_LED                 6           //LED pin for displaying what state
#define CON_LED                 7           //Connected to elm327 LED


ELM327 elm;         
SoftwareSerial debug(9, 10);                //Serial line for debugging

int state = 0;                              //State of SoundBoi
int min_state = 0;      
int max_state = 3;                          //GLOBAL VARS for interupt (when connected and not)


//Engine Data Struct
float rpm;
float speed;
float throttle;
float load;
float coolantTemp;
float engineOnTime;


bool rules(bool connected);            //Returns true or false depending on whether the valve should open or close
void blinkLed();  //Blink LED routine for dipiciting what state we are in.
void buttonHandler();                                   //Button ISR
void open();                                            //Open Valve         
void close();                                           //Close Valve



void setup(){}

void loop()
{
    


    Serial.begin(38400);
    debug.begin(9600);

    debug.printf("SoundBoi for BLAT! - Lewis Hamilton September 2021\r\n");
    debug.printf("VERSION: %d.%d.%d\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    debug.printf("<<%ld>> Loading variables...", millis());
    

    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonHandler, FALLING);
    
    pinMode(BUTTON, INPUT_PULLUP);
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(IND_LED, OUTPUT);
    pinMode(CON_LED, OUTPUT);

    bool connected = false;

    debug.printf("[OK]\r\n");  

    // Open valves for engine start.
    open();

    digitalWrite(IND_LED, HIGH);
    debug.printf("<<%ld>> CONNECTING TO ELM327...", millis());

    if(!elm.begin(Serial,false,1000))
    {
        connected = false;
        debug.printf("[FAIL]\r\n");
        digitalWrite(CON_LED, LOW);
        min_state = 2;
        max_state = 3;
        state = 2;
    }
    else
    {
        connected = true;
        debug.printf("[OK]\r\n");
        digitalWrite(CON_LED, HIGH);
        min_state = 0;
        max_state = 3;
    }
    
    digitalWrite(IND_LED, LOW);

    while(true)
    {
        if(connected)
        {
            float temprpm = elm.rpm();
            float tempspeed = elm.kph();
            float tempthrottle = elm.throttle();
            float tempload = elm.engineLoad();
            float tempcoolantTemp = elm.engineCoolantTemp();
            float tempengineOnTime = elm.runTime();
            if(elm.status == ELM_SUCCESS)
            {
                rpm = temprpm;
                speed = tempspeed;
                throttle = tempthrottle;
                load = tempload;
                coolantTemp = tempcoolantTemp;
                engineOnTime = tempengineOnTime;
                
                debug.printf("<<%lf>> RPM: %lf SPD: %lf THTL: %lf LOAD: %lf TEMP: %lfC ON-TIME: %lfs\r\n", millis(), rpm, speed, throttle, load, coolantTemp, engineOnTime);
            }
            else
            {
                debug.printf("<<%lf>> ELM FAIL!", millis());
            }
        }
        
        if(rules(connected))
        {
            open();
        }
        else
        {
            close();
        }

        blinkLed();
    }
}

bool rules(bool connected)
{
    bool openValve = true;
    if(state == 0 && engineOnTime > ENGINE_WARMUP_TIME)
    {
        if((rpm > 3000 || throttle > 70) && speed < 80)
            openValve = true;
        else if((rpm >  4000 || throttle > 80) && speed >= 80)
            openValve = true;
        else
            openValve = false;
    }

    else if(state == 1 && engineOnTime > ENGINE_WARMUP_TIME)
    {
        if((rpm > 2500 || load > 80 || throttle > 50) && speed < 90)
            openValve = true;
        else if(speed < 40 && coolantTemp > 50)
            openValve = true;
        else if(speed >= 110)
        {
            state = 0;
        }
        else
            openValve = false;
    }

    else if(state == 2)
    {
        openValve = false;
    }

    else if(state == 3)
    {
        openValve = true;
    }

    if(((rpm < 50 || engineOnTime < ENGINE_WARMUP_TIME) && connected) && state != 2)
    {
        if(coolantTemp > 50)
        {
            delay(10000);
        }
        openValve = true;
    }


    return openValve;
}

void open()
{
    debug.printf("<<%ld>> Opening valves!\r\n", millis());
    digitalWrite(OUTPUT_PIN, HIGH);
    delay(1000);
}

void close()
{
    debug.printf("<<%ld>> Closing valves!\r\n", millis());
    digitalWrite(OUTPUT_PIN, LOW);
    delay(1000);
}

void blinkLed()
{
    if(state == 0)
    {
        analogWrite(IND_LED, 10);
        delay(200);
        digitalWrite(IND_LED, LOW);
    }
    else if(state == 1)
    {
        analogWrite(IND_LED, 10);
        delay(10);
        digitalWrite(IND_LED, LOW);
    }
    else if(state == 2)
    {
        digitalWrite(IND_LED, LOW);
    }
    else if(state == 3)
    {
        analogWrite(IND_LED, 10);
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
  debug.printf("<<%ld>> Button pushed >> STATE %d", millis(), state);
  last_interrupt_time = interrupt_time;
}