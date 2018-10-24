#include "SPI_VFD.h"

#include <Arduino.h>

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <SPI.h>

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    N = 1; 2-line display
//    BR1=BR0=0; (100% brightness)
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// SPI_VFD constructor is called).

// commands
#define CMD_CLEARDISPLAY        0x01
#define CMD_RETURNHOME          0x02
#define CMD_ENTRYMODESET        0x04
#define CMD_DISPLAYCONTROL      0x08
#define CMD_CURSORSHIFT         0x10
#define CMD_FUNCTIONSET         0x30
#define CMD_SETCGRAMADDR        0x40
#define CMD_SETDDRAMADDR        0x80

// flags for display entry mode
#define VFD_ENTRYRIGHT          0x00
#define VFD_ENTRYLEFT           0x02
#define VFD_ENTRYSHIFTINCREMENT 0x01
#define VFD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define VFD_DISPLAYON           0x04
#define VFD_DISPLAYOFF          0x00
#define VFD_CURSORON            0x02
#define VFD_CURSOROFF           0x00
#define VFD_BLINKON             0x01
#define VFD_BLINKOFF            0x00

// flags for display/cursor shift
#define VFD_DISPLAYMOVE         0x08
#define VFD_CURSORMOVE          0x00
#define VFD_MOVERIGHT           0x04
#define VFD_MOVELEFT            0x00

// flags for function set
#define VFD_2LINE               0x08
#define VFD_1LINE               0x00
#define VFD_BRIGHTNESS25        0x03
#define VFD_BRIGHTNESS50        0x02
#define VFD_BRIGHTNESS75        0x01
#define VFD_BRIGHTNESS100       0x00

// SPI commmand/data select, will be or-ed with value
#define VFD_SPICOMMAND          0xF800
#define VFD_SPIDATA             0xFA00

SPI_VFD::SPI_VFD(SPIClass &spi, uint8_t cols, uint8_t lines)
 : _spi(spi), _numlines(lines)
{
}

void SPI_VFD::begin() 
{
    _spi.begin();

    // set number of lines
    if (_numlines > 1)
        _displayfunction = VFD_2LINE;
    else
        _displayfunction = VFD_1LINE;

    // set the brightness and push the linecount with VFD_SETFUNCTION
    setBrightness(VFD_BRIGHTNESS100);

    // Initialize to default text direction (for roman languages)
    _displaymode = VFD_ENTRYLEFT | VFD_ENTRYSHIFTDECREMENT;

    // set the entry mode
    command(CMD_ENTRYMODESET | _displaymode);

    command(CMD_SETDDRAMADDR);  // go to address 0

    // turn the display on with no cursor or blinking default
    _displaycontrol = VFD_DISPLAYON;
    displayOn(true);

    clear();
    home();
}

/********** high level commands, for the user! */
void SPI_VFD::setBrightness(uint8_t brightness){
  // set the brightness (only if a valid value is passed
  if (brightness <= VFD_BRIGHTNESS25) {
    _displayfunction &= ~VFD_BRIGHTNESS25;
    _displayfunction |= brightness;

	command(CMD_FUNCTIONSET | _displayfunction);
  }
}

uint8_t SPI_VFD::getBrightness()
{
  // get the brightness
  return _displayfunction & VFD_BRIGHTNESS25;
}

void SPI_VFD::clear()
{
  command(CMD_CLEARDISPLAY);  // clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void SPI_VFD::home()
{
  command(CMD_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void SPI_VFD::setCursor(uint8_t col, uint8_t row)
{
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if(row > 4)
      return; 
    command(CMD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void SPI_VFD::displayOn(bool on) 
{
  if(on)
    _displaycontrol |= VFD_DISPLAYON;
  else
    _displaycontrol &= ~VFD_DISPLAYON;
  command(CMD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void SPI_VFD::cursor(bool on) 
{
  if(on)
    _displaycontrol |= VFD_CURSORON;
  else
    _displaycontrol &= ~VFD_DISPLAYON;
  command(CMD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void SPI_VFD::blink(bool on) 
{
  if(on)
    _displaycontrol |= VFD_BLINKON;
  else
    _displaycontrol &= ~VFD_BLINKON;
  command(CMD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void SPI_VFD::scrollDisplayLeft(void) 
{
  command(CMD_CURSORSHIFT | VFD_DISPLAYMOVE | VFD_MOVELEFT);
}
void SPI_VFD::scrollDisplayRight(void) 
{
  command(CMD_CURSORSHIFT | VFD_DISPLAYMOVE | VFD_MOVERIGHT);
}

// This is for text that flows Left to Right
void SPI_VFD::leftToRight(void) 
{
  _displaymode |= VFD_ENTRYLEFT;
  command(CMD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void SPI_VFD::rightToLeft(void) 
{
  _displaymode &= ~VFD_ENTRYLEFT;
  command(CMD_ENTRYMODESET | _displaymode);
}

// This will left/right 'justify' text from the cursor
void SPI_VFD::autoscroll(bool on) 
{
  if(on)
    _displaycontrol |= VFD_ENTRYSHIFTINCREMENT;
  else
    _displaycontrol &= ~VFD_ENTRYSHIFTINCREMENT;
  command(CMD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void SPI_VFD::createChar(uint8_t location, uint8_t charmap[]) 
{
  location &= 0x7; // we only have 8 locations 0-7
  command(CMD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) 
  {
    write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds, init */
inline void SPI_VFD::command(uint8_t value) 
{
    //SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    _spi.transfer16(VFD_SPICOMMAND | value);
    // SPI.endTransaction(); // release the SPI bus
};

size_t SPI_VFD::write(uint8_t value) 
{
    // SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    _spi.transfer16(VFD_SPIDATA | value);
    // SPI.endTransaction(); // release the SPI bus
    return 1;
}
