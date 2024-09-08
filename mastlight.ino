#include <EEPROM.h>
#include <DMXSerial.h>

#include "gpio.h"

// Bits in gpiopData used for mastlight LEDs
#define MIN_LED_BIT    0
#define MAX_LED_BIT    9

// Bits on PIO ports - Port A is 0-7, Port B is 8-15
#define HB_LED_BIT     10     // heartbeat LED
#define SEQ_BUTTON_BIT 14     // sequence button

// Arduino pins
#define SPEED_PIN      A0     // Speed setting pot

// System tick period
#define TICK_TIME      10     // 10 milliseconds

// On time = "speed" * TICK_TIME 
#define MIN_SPEED      100    // 1s
#define MAX_SPEED      1      // 10ms

#define HB_TICKS       10     // heartbeat every (TICK_TIME * HB_TICKS) = 100ms

#define EEPROM_WRITE_TICKS 500 // Number of ticks for deferred eeprom write

#define FLASH_ON_MASK  0x0124

#define DMX_ADDR       1      // Base DMX address  DMX_ADDR > 127, light on  DMX_ADDR + 1, speed  DMX_ADDR + 2 > 127, sequence button pressed
#define DMX_TIMEOUT    5000   // in milliseconds

typedef struct
{
    char *seqName;
    void (*fn)(void);
} sequenceType;

typedef struct
{
    size_t configSize;
    int sequence;
} configType;

int seqBit;
int seqMaxBit;
int seqTickCount;
int seqSetTickCount;
boolean seqInc;
boolean seqOn;

void seqRotate(void);
void seqKnightRider(void);
void seqRandom(void);
void seqRotate2(void);
void seqFlash(void);

sequenceType sequenceDefs[] =
{
    { "ROTATE",      seqRotate },
    { "KNIGHTRIDER", seqKnightRider },
    { "RANDOM",      seqRandom },
    { "ROTATE2",     seqRotate2 },
    { "FLASH",       seqFlash },
    { "",            NULL }
};

boolean firstTime;

boolean seqButtonPressed;
unsigned short int seqButtonMask;

int heartbeatTicks;

unsigned long lastMillis;

configType sysConfig;
int eepromWriteTicks;

int dmxRed;
int dmxGreen;
int dmxBlue;
boolean gotDmxData;

boolean lightsOn;

void writeConfig()
{
    EEPROM.put(0, sysConfig);
}

void defaultConfig()
{
    sysConfig.configSize = sizeof(sysConfig);
    sysConfig.sequence = 0;
}

void readConfig()
{
    EEPROM.get(0, sysConfig);
    if(sysConfig.configSize != sizeof(sysConfig))
    {
        defaultConfig();
        writeConfig();
    }
}

boolean gpioSequenceButtonPressed()
{
    unsigned short int inData;
  
    inData = getGpioData();
  
    if((inData & seqButtonMask) != seqButtonMask)
    {
        return true;
    }
    else
    {
        return false;
    }
}

boolean dmxSequenceButtonPressed()
{
    boolean rtn;

    rtn = false;
    
    if(gotDmxData == true && DMXSerial.read(dmxBlue) > 127)
    {
        rtn = true;
    }

    return rtn;
}

boolean seqButtonPressedAndReleased()
{
    boolean rtn;

    rtn = false;
    
    if(gpioSequenceButtonPressed() == true || dmxSequenceButtonPressed())
    {
        seqButtonPressed = true;
    }
    else
    {
        if(seqButtonPressed == true)
        {
            rtn = true;
        }

        seqButtonPressed = false;
    }

    return rtn;
}

void newSequence()
{
    firstTime = true;
    seqTickCount = 0;

    clearGpio();
}

void handleSequenceButton()
{
    int x;
    
    if(seqButtonPressedAndReleased() == true)
    {
        // button was pressed

        sysConfig.sequence++;
        if(sequenceDefs[sysConfig.sequence].seqName[0] == '\0')
        {
            sysConfig.sequence = 0;
        }

        eepromWriteTicks = EEPROM_WRITE_TICKS;
        newSequence();
    }
}

void readSpeedPot()
{
    seqSetTickCount = map(analogRead(SPEED_PIN), 0, 1024, MAX_SPEED, MIN_SPEED + 1);
    if(seqTickCount > seqSetTickCount)
    {
        seqTickCount = seqSetTickCount - 2;
    }
}

void seqRandom()
{
    if(firstTime == true)
    {
        firstTime = false;
        randomSeed(millis());
        seqMaxBit = MAX_LED_BIT + 1;
        seqBit = random(MIN_LED_BIT, seqMaxBit);
        setGpioBit(seqBit);
    }
    else
    {
        seqTickCount++;
        if(seqTickCount == seqSetTickCount)
        {
            seqTickCount = 0;
            
            clearGpioBit(seqBit);
            seqBit = random(MIN_LED_BIT, seqMaxBit);
            setGpioBit(seqBit);
        }
    }
}

void seqFlash()
{
    if(firstTime == true)
    {
        firstTime = false;
        seqOn = true;
        gpioData  = gpioData | FLASH_ON_MASK;
        writeGpio();
    }
    else
    {
        seqTickCount++;
        if(seqTickCount == seqSetTickCount)
        {
            seqTickCount = 0;
            if(seqOn == true)
            {
                seqOn = false;
                clearGpio();
            }
            else
            {
                seqOn = true;
                gpioData = gpioData | FLASH_ON_MASK;
                writeGpio();
            }
        }
    }
}

void seqKnightRider()
{
    if(firstTime == true)
    {
        firstTime = false;
        seqBit = MIN_LED_BIT;
        seqMaxBit = MAX_LED_BIT;
        seqInc = true;
        setGpioBit(seqBit);
    }
    else
    {
        seqTickCount++;
        if(seqTickCount == seqSetTickCount)
        {
            seqTickCount = 0;
            
            clearGpioBit(seqBit);

            if(seqInc == true)
            {
                if(seqBit == seqMaxBit)
                {
                    seqInc = false;
                    seqBit--;
                }
                else
                {
                    seqBit++;
                }
            }
            else
            {
                if(seqBit == MIN_LED_BIT)
                {
                    seqInc = true;
                    seqBit++;
                }
                else
                {
                    seqBit--;
                }
            }
            setGpioBit(seqBit);
        }
    }
}

void seqRotate()
{
    if(firstTime == true)
    {
        firstTime = false;
        seqBit = MIN_LED_BIT;
        seqMaxBit = MAX_LED_BIT;
        setGpioBit(seqBit);
    }
    else
    {
        seqTickCount++;
        if(seqTickCount == seqSetTickCount)
        {
            seqTickCount = 0;

            clearGpioBit(seqBit);

            if(seqBit == seqMaxBit)
            {
                seqBit = MIN_LED_BIT;
            }
            else
            {
                seqBit++;
            }
            setGpioBit(seqBit);
        }
    }
}

void seqRotate2()
{
    if(firstTime == true)
    {
        firstTime = false;
        seqBit = MIN_LED_BIT;
        seqMaxBit = MAX_LED_BIT / 2;
        setGpioBit(seqBit);
    }
    else
    {
        seqTickCount++;
        if(seqTickCount == seqSetTickCount)
        {
            seqTickCount = 0;

            clearGpioBit(seqBit);
            clearGpioBit(seqBit + 5);

            if(seqBit == seqMaxBit)
            {
                seqBit = MIN_LED_BIT;
            }
            else
            {
                seqBit++;
            }

            setGpioBit(seqBit);
            setGpioBit(seqBit + 5);
        }
    }
}

void heartbeatLed()
{
    heartbeatTicks--;
    if(heartbeatTicks == 0)
    {
        heartbeatTicks = HB_TICKS;

        if(isSet(HB_LED_BIT) == true)
        {
            clearGpioBit(HB_LED_BIT);
        }
        else
        {
            setGpioBit(HB_LED_BIT);
        }
    }
}

void eepromWrite()
{
    if(eepromWriteTicks)
    {
        eepromWriteTicks--;
        if(eepromWriteTicks == 0)
        {
            // save config data
            writeConfig();
        }
    }
}

void handleDmx()
{
    if(DMXSerial.noDataSince() > DMX_TIMEOUT)
    {
        lightsOn = true;
        gotDmxData = false;
    }
    else
    {   
        gotDmxData = true;
        if(DMXSerial.read(dmxRed) > 127)
        {    
            if(lightsOn == false)
            {
                lightsOn = true;
                newSequence();
            }
        }
        else
        {
            if(lightsOn == true)
            {
                lightsOn = false;
                clearGpio();
            }
        }

        seqSetTickCount = map(DMXSerial.read(dmxGreen), 0, 255, MIN_SPEED +1, MAX_SPEED);
        if(seqTickCount > seqSetTickCount)
        {
            seqTickCount = seqSetTickCount - 2;
        }
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    initPio();

    lastMillis = millis();

    heartbeatTicks = HB_TICKS;

    seqButtonPressed = false;
    seqButtonMask = 1 << SEQ_BUTTON_BIT;

    readConfig();

    dmxRed = DMX_ADDR;
    dmxGreen = DMX_ADDR + 1;
    dmxBlue = DMX_ADDR + 2;
    gotDmxData = false;
    DMXSerial.init(DMXReceiver);

    newSequence();
}

void loop()
{
    unsigned long int millisNow;

    millisNow = millis();
    if(millisNow - lastMillis >= TICK_TIME)
    {
        lastMillis = millisNow;

        eepromWrite();

        heartbeatLed();

        if(lightsOn == true)
        {
            sequenceDefs[sysConfig.sequence].fn();
        }
    }

    // Deal with DMX data
    handleDmx();

    // Check sequence button
    handleSequenceButton();

    // Look at speed pot if there wasn't any DMX
    if(gotDmxData == false)
    {
        // Check speed pot
        readSpeedPot();    
    }
}
