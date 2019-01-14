# MS5611
MS5611 - barometric pressure sensor library for Arduino (SPI version)

Rewrite of Rob Tillaart's work of the library.
- Modified for SPI instead of I2C
- Using only integers (for MCU's that don't do floating point operations)
- Added function to read PROM parameters
- Added PROM CRC4 code (in exaple sketch)
