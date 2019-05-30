#include "SamsungVFD.h"

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
// constructor is called).

// commands
#define CMD_CLEARDISPLAY       		0b00000001
#define CMD_RETURNHOME          	0b00000010
#define CMD_ENTRYMODESET        	0b00000100	// 0000 01IS
	#define ENTRY_RIGHT          	0b00000000
	#define ENTRY_LEFT           	0b00000010
	#define ENTRY_INCREMENT 		0b00000001
	#define ENTRY_DECREMENT 		0b00000000

#define CMD_DISPLAYCONTROL      	0b00001000	// 00001DCB
	#define CONTROL_DISPLAYON       0b00000100
	#define CONTROL_DISPLAYOFF      0b00000000
	#define CONTROL_CURSORON        0b00000010
	#define CONTROL_CURSOROFF       0b00000000
	#define CONTROL_BLINKON         0b00000001
	#define CONTROL_BLINKOFF        0b00000000

#define CMD_CURSORSHIFT         	0b00010000	// 0001SRxx
	#define SHIFT_DISPLAYMOVE       0b00001000
	#define SHIFT_CURSORMOVE        0b00000000
	#define	SHIFT_RIGHT         	0b00000100
	#define SHIFT_LEFT          	0b00000000
#define CMD_FUNCTIONSET         	0b00100000
	#define FUNCTION_DATA8			0b00010000
	#define FUNCTION_2LINE          0b00001000 
	#define FUNCTION_1LINE          0b00000000
	#define FUNCTION_BRIGHTNESS25   0b00000011
	#define FUNCTION_BRIGHTNESS50   0b00000010
	#define FUNCTION_BRIGHTNESS75   0b00000001
	#define FUNCTION_BRIGHTNESS100  0b00000000
#define CMD_SETCGRAMADDR        	0b01000000 // 01aaaaaa 
#define CMD_SETDDRAMADDR        	0b10000000 // 1aaaaaaa

// flags for display entry mode

// flags for display on/off control

// flags for display/cursor shift

// flags for function set

// SPI commmand/data select, will be or-ed with value
#define VFD_SPIDATA            		0xFA   	// RW=0, RS=1
#define VFD_SPICOMMAND				0xF8	// RW=0, RS=1
#define VFD_SPICOMMAND16        	(VFD_SPICOMMAND << 8) 
#define VFD_SPIDATA16           	(VFD_SPIDATA << 8)

void SamsungVFD::begin()
{
    // set the brightness(=0) and push the linecount with VFD_SETFUNCTION
	_displayfunction = FUNCTION_2LINE | FUNCTION_DATA8| FUNCTION_BRIGHTNESS100;
	command(CMD_FUNCTIONSET | _displayfunction);

    // Initialize to default text direction (for roman languages)
    _displaymode = ENTRY_LEFT | ENTRY_DECREMENT;
    command(CMD_ENTRYMODESET | _displaymode);

    // turn the display on with no cursor or blinking default
    _displaycontrol = CONTROL_DISPLAYON;
    command(CMD_DISPLAYCONTROL | _displaycontrol);

	// go to address 0
    command(CMD_SETDDRAMADDR); 
}

/********** high level commands, for the user! */
void SamsungVFD::setBrightness(uint8_t brightness)
{
    // set the brightness (only if a valid value is passed
    if (brightness <= FUNCTION_BRIGHTNESS25)
    {
        _displayfunction &= ~FUNCTION_BRIGHTNESS25;
        _displayfunction |= brightness;

        command(CMD_FUNCTIONSET | _displayfunction);
    }
}

uint8_t SamsungVFD::getBrightness()
{
    // get the brightness
    return _displayfunction & FUNCTION_BRIGHTNESS25;
}

void SamsungVFD::clear()
{
    command(CMD_CLEARDISPLAY); // clear display, set cursor position to zero
    delayMicroseconds(2000);   // this command takes a long time!
}

void SamsungVFD::home()
{
    command(CMD_RETURNHOME); // set cursor position to zero
    delayMicroseconds(2000); // this command takes a long time!
}

void SamsungVFD::setCursor(uint8_t col, uint8_t row)
{
    int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 3)
        return;
    command(CMD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void SamsungVFD::displayOn(bool on)
{
    if (on)
        _displaycontrol |= CONTROL_DISPLAYON;
    else
        _displaycontrol &= ~CONTROL_DISPLAYON;
    command(CMD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void SamsungVFD::cursor(bool on)
{
    if (on)
        _displaycontrol |= CONTROL_CURSORON;
    else
        _displaycontrol &= ~CONTROL_DISPLAYON;
    command(CMD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void SamsungVFD::blink(bool on)
{
    if (on)
        _displaycontrol |= CONTROL_BLINKON;
    else
        _displaycontrol &= ~CONTROL_BLINKON;
    command(CMD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void SamsungVFD::scrollDisplayLeft(void)
{
    command(CMD_CURSORSHIFT | SHIFT_DISPLAYMOVE | SHIFT_LEFT);
}

void SamsungVFD::scrollDisplayRight(void)
{
    command(CMD_CURSORSHIFT | SHIFT_DISPLAYMOVE | SHIFT_RIGHT);
}

// This is for text that flows Left to Right
void SamsungVFD::leftToRight(void)
{
    _displaymode |= ENTRY_LEFT;
    command(CMD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void SamsungVFD::rightToLeft(void)
{
    _displaymode &= ~ENTRY_LEFT;
    command(CMD_ENTRYMODESET | _displaymode);
}

// This will left/right 'justify' text from the cursor
void SamsungVFD::autoscroll(bool on)
{
    if (on)
        _displaycontrol |= ENTRY_INCREMENT;
    else
        _displaycontrol &= ~ENTRY_INCREMENT;
    command(CMD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void SamsungVFD::createChar(uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    command(CMD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++)
    {
        write(charmap[i]);
    }
}

/*********** mid level commands, for sending data/cmds, init */
#ifdef SAMSUNGVFD_SPI_SWCS
	#define	SAMSUNGVFD_CS_ON		   	if(_cs != 0xFF) digitalWrite(_cs, LOW);
	#define	SAMSUNGVFD_CS_OFF		   	if(_cs != 0xFF) digitalWrite(_cs, HIGH);
#else
	#define SAMSUNGVFD_CS_ON
	#define SAMSUNGVFD_CS_OFF
#endif

void SamsungVFD_SPI::command(uint8_t value)
{
#ifdef SAMSUNGVFD_SPI_TRANSACTION
    SPI.beginTransaction(_spisettings);
#endif
	SAMSUNGVFD_CS_ON;
   	_spi.transfer16(VFD_SPICOMMAND16 | value);
	SAMSUNGVFD_CS_OFF;
#ifdef SAMSUNGVFD_SPI_TRANSACTION
    SPI.endTransaction(); // release the SPI bus
#endif
};

size_t SamsungVFD_SPI::write(uint8_t value)
{
#ifdef SAMSUNGVFD_SPI_TRANSACTION
    SPI.beginTransaction(_spisettings);
#endif
	SAMSUNGVFD_CS_ON;
    _spi.transfer16(VFD_SPIDATA16 | value);
	SAMSUNGVFD_CS_OFF;

#ifdef SAMSUNGVFD_SPI_TRANSACTION
    SPI.endTransaction(); // release the SPI bus
#endif
    return 1;
}

// Unsure if this loop would work on a HW CS platform like ESP32
#ifdef SAMSUNGVFD_SPI_SWCS
size_t SamsungVFD_SPI::write(const uint8_t *buffer, size_t size)
{
    uint8_t cnt = 0;
#ifdef SAMSUNGVFD_SPI_TRANSACTION
    SPI.beginTransaction(_spisettings);
#endif

	SAMSUNGVFD_CS_ON;
	_spi.transfer(VFD_SPIDATA);
	while (size--)
	{
		_spi.transfer(buffer[cnt++]);
	};
	SAMSUNGVFD_CS_OFF;

#ifdef SAMSUNGVFD_SPI_TRANSACTION
    SPI.endTransaction(); // release the SPI bus
#endif
    return cnt;
}
#endif // SAMSUNGVFD_SPI_SWCS

#undef VFD_CHIPSELECT_ON
#undef VFD_CHIPSELECT_OFF


void SamsungVFD_I2C::command(uint8_t cmd)
{
	_wire.beginTransmission(_i2caddr);
	_wire.write(cmd);
	_wire.endTransmission();
};

size_t SamsungVFD_I2C::write(uint8_t value)
{
	_wire.beginTransmission(_i2caddr+1);
	_wire.write(value);
	_wire.endTransmission();

    return 1;
}

size_t SamsungVFD_I2C::write(const uint8_t *buffer, size_t size)
{
    uint8_t cnt = 0;

	_wire.beginTransmission(_i2caddr+1);
	while (size--)
	{
		_wire.write(buffer[cnt++]);
	};
	_wire.endTransmission();

    return cnt;
}
