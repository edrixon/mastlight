
#include "gpio.h"

// Bits in gpiopData used for mastlight LEDs
#define MIN_BIT       0
#define MAX_BIT       9

// Bits on PIO ports - Port A is 0-7, Port B is 8-15
#define SEQ_BUTTON_BIT 14    // sequence button
#define HB_LED_BIT    10     // heartbeat LED

// Arduino pins
#define SPEED_PIN     A0     // Speed setting pot

// System tick period
#define TICK_TIME     10     // 10 milliseconds

// On time = "speed" * TICK_TIME 
#define MIN_SPEED     100    // 1s
#define MAX_SPEED     1      // 10ms

#define HB_TICKS      10     // heartbeat every (TICK_TIME * HB_TICKS) = 100ms

typedef struct
{
    char *name;
    void fn(void);
} sequenceType;

int seqBit;
int seqMaxBit;
int seqTickCount;
int seqSetTickCount;
boolean seqInc;

void seqRotate();
void seqKnightRider();
void seqRandom();
void seqRotate2();

sequenceType sequenceDefs[] =
{
    { "ROTATE",      seqRotate },
    { "KNIGHTRIDER", seqKnightRider },
    { "RANDOM",      seqRandom },
    { "ROTATE2",     seqRotate2 },
    { "",            NULL }
};
int sequence;
boolean firstTime;

boolean seqButtonPressed;
unsigned short int seqButtonMask;

int heartbeatTicks;

unsigned long lastMillis;

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

boolean seqButtonPressedAndReleased()
{
    boolean rtn;

    rtn = false;
    
    if(gpioSequenceButtonPressed() == true)
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
    Serial.print("Starting ");
    Serial.print(sequenceDefs[sequence].name);
    Serial.println(" sequence");

    Serial.print("  Interval = ");
    Serial.print(seqSetTickCount * TICK_TIME);
    Serial.println(" milliseconds");
 
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

        sequence++;
        if(sequenceDefs[sequence].fn == NULL)
        {
            sequence = 0;
        }

        newSequence();
   }
}

void readSpeedPot()
{
    seqSetTickCount =
                 map(analogRead(SPEED_PIN), 0, 1024, MAX_SPEED, MIN_SPEED + 1);
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
        seqMaxBit = MAX_BIT + 1;
        seqBit = random(MIN_BIT, seqMaxBit);
        setGpioBit(seqBit);
    }
    else
    {
        seqTickCount++;
        if(seqTickCount == seqSetTickCount)
        {
            seqTickCount = 0;
            
            clearGpioBit(seqBit);
            seqBit = random(MIN_BIT, seqMaxBit);
            setGpioBit(seqBit);
        }
    }
}

void seqKnightRider()
{
    if(firstTime == true)
    {
        firstTime = false;
        seqBit = MIN_BIT;
        seqMaxBit = MAX_BIT;
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
                if(seqBit == MIN_BIT)
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
        seqBit = MIN_BIT;
        seqMaxBit = MAX_BIT;
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
                seqBit = MIN_BIT;
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
        seqBit = MIN_BIT;
        seqMaxBit = MAX_BIT / 2;
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
                seqBit = MIN_BIT;
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

void setup()
{
    Serial.begin(9600);
    Serial.println("Mastlight controller");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    initPio();

    lastMillis = millis();

    heartbeatTicks = HB_TICKS;

    seqButtonPressed = false;
    seqButtonMask = 1 << SEQ_BUTTON_BIT;

    readSpeedPot();

    Serial.println("");

    sequence = 0
    newSequence();

}

void loop()
{
    unsigned long int millisNow;

    millisNow = millis();
    if(millisNow - lastMillis >= TICK_TIME)
    {
        lastMillis = millisNow;

        heartbeatLed();

        sequenceDefs[sequence].fn();

    }

    // Check sequence button
    handleSequenceButton();

    // Check speed pot
    readSpeedPot();
}
