# MS5611
MS5611 - barometric pressure sensor library for Arduino (SPI version)

Rewrite of Rob Tillaart's work of the library.
- Modified for SPI instead of I2C
- Using only integers (for MCU's that don't do floating point operations)
- Added function to get ROM parameters in sketch
- Added PROM parameter CRC4 check (in example sketch)

To do:
- ~~properly implement second order compensation~~
- include CRC4 check in library
- combine SPI and I2C version in one library
- measure speed difference between float and RPN integer calculations
