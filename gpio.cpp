#include <Arduino.h>
#include <Wire.h>

#define __IN_GPIO

#define PIO_I2C_ADDR  0x20

#define IODIR         0x00
#define GPPU          0x0c
#define GPIO          0x12
#define OLAT          0x14

unsigned short int gpioData;

void checkPioConnected()
{
    Serial.print("  I2C . . . . . ");
    Wire.begin();
    Serial.println("OK");

    Serial.print("  PIO . . . . . ");
    Wire.beginTransmission(PIO_I2C_ADDR);
    if(Wire.endTransmission() != 0)
    {
        Serial.println("not found!!!");
        while(1)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(50);
            digitalWrite(LED_BUILTIN, LOW);
            delay(1000);
        }
    }
    Serial.println("OK");
}

void wireBeginTransmission(unsigned char busAddr)
{
    digitalWrite(LED_BUILTIN, HIGH);
    Wire.beginTransmission(busAddr);
}

int wireEndTransmission()
{
    int rtn;
  
    rtn = Wire.endTransmission();
    if(rtn != 0)
    {
        Serial.print("I2C error - ");
        Serial.println(rtn);
    }

    digitalWrite(LED_BUILTIN, LOW);
  
    return rtn;
}

void writePioByte(unsigned char regAddr, unsigned char regData)
{
    wireBeginTransmission(PIO_I2C_ADDR);
    Wire.write(regAddr);
    Wire.write(regData);
    wireEndTransmission();
}

void writePioWord(unsigned char regAddr, unsigned short int regData)
{
    unsigned char lo;
    unsigned char hi;
  
    lo = regData & 0x00ff;
    hi = (unsigned char)((unsigned short int)(regData & 0xff00) >> 8);
  
    wireBeginTransmission(PIO_I2C_ADDR);
    Wire.write(regAddr);
    Wire.write(lo);
    Wire.write(hi);
    wireEndTransmission();
}

unsigned short int readPioWord(unsigned char regAddr)
{
    int bc;
    unsigned short int rtn;
    unsigned short int x;
  
    bc = 0;
    rtn = 0;
  
    wireBeginTransmission(PIO_I2C_ADDR);
    Wire.write(regAddr);
    wireEndTransmission();
    Wire.requestFrom(PIO_I2C_ADDR, 2);
    while(Wire.available())
    {
        x = (unsigned short int)Wire.read();
        if(bc == 0)
        {
            rtn = x;
            bc++;
        }
        else
        {
            rtn = (x << 8) | rtn;
        }
    }
  
    return rtn;
}

void writeGpio()
{
    writePioWord(OLAT, gpioData);
}

void clearGpio()
{
    // clear led outputs
    // write data back
    gpioData = 0;
    writeGpio();
}

void initPio()
{
    // Make sure GPIO board is plugged in
    checkPioConnected();

    // direction registers IODIRA = 0x00 IODIRB = 0xc0
    writePioWord(IODIR, 0xc000);

    // input pullups GPPUB = 0xc0
    writePioWord(GPPU, 0xc000);

    // data registers
    clearGpio();
}

void clearGpioBit(int b)
{
    unsigned short int mask;

    mask = ~(1 << b);
    gpioData = gpioData & mask;
    writeGpio();
}

void setGpioBit(int b)
{
    unsigned short int mask;

    mask = 1 << b;
    gpioData = gpioData | mask;
    writeGpio();
}

unsigned short int getGpioData()
{
    return readPioWord(GPIO);
}

boolean isSet(int b)
{
    unsigned short int mask;
  
    mask = 1 << b;
    if((gpioData & mask) == mask)
    {
        return true;
    }
    else
    {
        return false;
    }
}
