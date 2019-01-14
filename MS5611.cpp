//
//    FILE: MS5611.cpp
//  AUTHOR: Rob Tillaart
//          Erni - testing/fixes
// VERSION: 0.1.8a
// PURPOSE: MS5611 Temperature & Atmospheric Pressure library for Arduino
//     URL:
//
// POST FORK HISTOTY
// 0.1.8a 2019-01-11 Modified for SPI
//
// PRE-FORK HISTORY:
// 0.1.8  fix #109 incorrect constants (thanks to flauth)
// 0.1.7  revert double to float (issue 33)
// 0.1.6  2015-07-12 refactor
// 0.1.05 moved 6 float multiplies to init  [adds ~70 bytes !!!]
//        moved the MS5611_LIB_VERSION to PROGMEM
// 0.1.04 changed float to double (for platforms which support it)
//        changed divisions in multiplications
//        fixed uint32_t readADC()
//        reduced size of C array by 1 float
//        added second order temperature compensation
// 0.1.03 changed math to float [test version]
// 0.1.02 fixed bug return value read()
//        fixed bug #bits D2
//        added MS5611_READ_OK
//        added inline getters for temp & pres & lastresult.
//        adjusted delay's based on datasheet
//        merged convert functions
//        fixed offset in readProm()
// 0.1.01 small refactoring
// 0.1.00 added temperature and Pressure code
// 0.0.00 initial version by Rob Tillaart (15-okt-2014)
//
// Released to the public domain
//

#include "MS5611.h"

#include <SPI.h>

/////////////////////////////////////////////////////
//
// PUBLIC
//
MS5611::MS5611(uint8_t CSn)
{
  _cspin = CSn;
  pinMode(_cspin, OUTPUT);
  digitalWrite(_cspin, HIGH);
  //SPI.begin();
  _temperature = -999;
  _pressure = -999;
  //init();
}

void MS5611::init()
{
  reset();

  // Default values (C1 t/m C86 taken from example column in datasheet to test calculations):
  C[0] = 1;
  C[1] = 40127;
  C[2] = 36924;
  C[3] = 23317;
  C[4] = 23282;
  C[5] = 33464;
  C[6] = 28312;
  C[7] = 0xF0F0;
 
  SPISettings settingsA(1000000, MSBFIRST, SPI_MODE0);  // define SPI settings; limit SPI communication to 1MHz
  SPI.beginTransaction(settingsA);                      // start SPI transaction
  for (uint8_t reg = 0; reg < 8; reg++)
  {
    C[reg] = readProm(reg);
  }
  SPI.endTransaction();                                 // end SPI transaction
}

int MS5611::read(uint8_t bits)
{
  // VARIABLES NAMES BASED ON DATASHEET  <- Nice!
  convert(0x40, bits);
  uint32_t D1 = readADC();

  convert(0x50, bits);
  uint32_t D2 = readADC();

  // TODO the multiplications of these constants can be done in init()
  // but first they need to be verified.

  // TEMP & PRESS MATH - PAGE 7/20
  //  - avoiding float type to make running on Tiny's not-impossible
  //  - running into issues with uint64_t so using uint32_t with adjustments
  //      (at the cost of reduced resolution for temperature).
  uint32_t Tref, dT, D2_;
  uint32_t dTC6;
  int32_t  TEMP;
  Tref = C[5];
  Tref = Tref << 3;           // not multiplying by 2^8: keep   16 bits
  D2_  = D2 >> 5 ;            // dividing by 2^8       :  24 -> 16 bits

  if (D2_ < Tref ) {          // to avoid signed int so we can bit-shift for divisioning
    dT   = Tref - D2_;        // 1/256th ; less resolution but  16 bits
    dTC6 = dT * C[6];         // 16 x 16 bits                   32 bits
    dTC6 = dTC6 >> 18;        // divide by 2^15 instead of 2^23
    TEMP = 2000 - dTC6;
  } else {                    // same for D2 >= Tref ...
    dT   = D2_ - Tref;
    dTC6 = dT * C[6];
    dTC6 = dTC6 >> 18;  
    TEMP = 2000 + dTC6;
  }
//
//  // OFF = offT1 + TCO * dT = C2 * 2^16 + (C4 * dT ) / 2^7
//  uint64_t offT1  =  (uint64_t)C[2] << 16; // multiply by 2^16
//  int64_t  TCOdT  =  C[4] * dT;
//           TCOdT  = TCOdT / 128; // not sure if I can bit-shift signed integers for division
//                                 //  otherwise I would do: TCOdT = TCOdT >> 7
//  int64_t  OFF    =  offT1 + TCOdT;
//
//  // SENSE = sensT1 + TCS * dT = C1 * 2^15 + (C3 * dT ) / 2^8
//  uint64_t sensT1 = (uint64_t)C[1] << 15; // multiply by 2^15
//  int64_t  TCSdT  = C[3] * dT;
//           TCSdT  = TCSdT / 256; // not sure if I can bit-shift signed integers for division
//                                 //  otherwise I would do: TCSdT = TCSdT >> 8
//  int64_t  SENS   = sensT1 + TCSdT;

  _temperature = TEMP; 

  // SECOND ORDER COMPENSATION - PAGE 8/20
  // COMMENT OUT < 2000 CORRECTION IF NOT NEEDED
  // NOTE TEMPERATURE IS IN 0.01 C
//  uint64_t T2    = 0;
//  uint64_t OFF2  = 0;
//  uint64_t SENS2 = 0;
//  if (TEMP < 2000)
//  {
//    uint64_t tSQ;
//    T2    = dT * dT;
//    T2    = dT >> 31;
//    tSQ   = TEMP - 2000;
//    tSQ   = tSQ * tSQ;
//    OFF2  = 5 * tSQ;
//    OFF2  = OFF2 >> 1;
//    SENS2 = 5 * tSQ;
//    SENS2 = SENS2 >> 2;
//    // COMMENT OUT < -1500 CORRECTION IF NOT NEEDED
//    if (TEMP < -1500)
//    {
//      tSQ   = TEMP + 1500;
//      tSQ   = tSQ * tSQ;
//      OFF2 += 7 * tSQ;
//      tSQ   = tSQ >> 1;
//      OFF2 += 11 * tSQ;
//    }
//  }
//  _temperature -= T2;
//  OFF  -= OFF2;
//  SENS -= SENS2;
  // END SECOND ORDER COMPENSATION


  //_pressure = (D1 * sens * 4.76837158205E-7 - offset) * 3.051757813E-5;
  _pressure = C[6];

  return 0;
}


/////////////////////////////////////////////////////
//
// PRIVATE
//
void MS5611::reset()
{
  SPISettings settingsA(1000000, MSBFIRST, SPI_MODE0);  // define SPI settings; limit SPI communication to 1MHz
  SPI.beginTransaction(settingsA);                      // start SPI transaction
  digitalWrite(_cspin, LOW);                            // pull CS line low
  SPI.transfer(0x1E);                                   // send reset command
  delay(4);
  digitalWrite(_cspin,HIGH);                            // pull CS line high
  SPI.endTransaction();                                 // end SPI transaction
}

void MS5611::convert(const uint8_t addr, uint8_t bits)
{
  uint8_t del[5] = {1, 2, 3, 5, 10};                    // array of MS5611 conversion time (in ms)
  bits = constrain(bits, 8, 12);
  uint8_t offset = (bits - 8) * 2;
  SPISettings settingsA(1000000, MSBFIRST, SPI_MODE0);  // define SPI settings; limit SPI communication to 1MHz
  SPI.beginTransaction(settingsA);                      // start SPI transaction
  digitalWrite(_cspin, LOW);                            // pull CS line low
  SPI.transfer(addr + offset);                          // send command
  delay(del[offset/2]);                                 // MS5611 needs some time for conversion; wait for this...
  digitalWrite(_cspin,HIGH);                            // pull CS line high
  SPI.endTransaction();                                 // end SPI transaction
}

uint16_t MS5611::readProm(uint8_t reg) {
  // read two bytes from SPI and eventually return accumulated value
  reg = min(reg, 7);
  uint8_t offset = reg * 2;
  uint16_t val = 0;
  digitalWrite(_cspin, LOW);                            // pull CS line low
  SPI.transfer(0xA0 + offset);                          // send command
  val  = SPI.transfer(0x00);                            // read 8 bits of data (MSB)
  val  = val << 8;                                      // shift left 8 bits
  val |= SPI.transfer(0x00);                            // read 8 bits of data (LSB)
  digitalWrite(_cspin,HIGH);                            // pull CS line high
  return val;
}

uint32_t MS5611::readADC() {
  // read two bytes from SPI and eventually return accumulated value
  uint8_t  v;
  uint32_t val = 0;
  SPISettings settingsA(1000000, MSBFIRST, SPI_MODE0);  // define SPI settings; limit SPI communication to 1MHz
  SPI.beginTransaction(settingsA);                      // start SPI transaction
  digitalWrite(_cspin, LOW);                            // pull CS line low
  SPI.transfer(0x00);                                   // send command
  val  = SPI.transfer(0x00);                            // read 8 bits of data (MSB)
  val  = val << 8;                                      // shift left 8 bits
  val |= SPI.transfer(0x00);                            // read 8 bits of data
  val  = val << 8;                                      // shift left 8 bits
  val |= SPI.transfer(0x00);                            // read 8 bits of data (LSB)
  digitalWrite(_cspin,HIGH);                            // pull CS line high
  SPI.endTransaction();                                 // end SPI transaction
  return val;
}

// END OF FILE
