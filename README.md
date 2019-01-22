# MS5611
MS5611 - barometric pressure sensor library for Arduino (SPI version)

Rewrite of Rob Tillaart's work of the library.
- Modified for SPI instead of I2C
- Using only integers (for MCU's that don't do floating point operations)
- Added function to get ROM parameters in sketch
- Added PROM parameter CRC4 check (in example sketch)

**Warning:** *the IO pins of the MS5611 are not 5V tolerant. The popular GY-63
break out board has a voltage regulator but it does not provide protection
from overvoltage on the SPI/I2C pins. The sensor breaks easily when more than
3.5V is applied to the IO pins. Typical sign of defective sensor is a temperature
reading of 30+ degrees at room temperature.*

To do:
- ~~properly implement second order compensation~~
- include CRC4 check in library
- combine SPI and I2C version in one library
- measure speed difference between float and RPN integer calculations
