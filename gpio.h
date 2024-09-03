void checkPioConnected();
void wireBeginTransmission(unsigned char busAddr);
int wireEndTransmission();
void writePioByte(unsigned char regAddr, unsigned char regData);
void writePioWord(unsigned char regAddr, unsigned short int regData);
unsigned short int readPioWord(unsigned char regAddr);
void writeGpio();
void clearGpio();
void initPio();
void clearGpioBit(int b);
void setGpioBit(int b);
unsigned short int getGpioData();
boolean isSet(int b);

#ifndef __IN_GPIO

extern unsigned short int gpioData;

#endif
