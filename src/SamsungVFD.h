#ifndef SPI_VFD_h
#define SPI_VFD_h

#include <Arduino.h>
#include <SPI.h>
#include <inttypes.h>
#include <Print.h>
#include <Wire.h>

// VFD Module 20T202DA2JA
// Controller: µ PD16314

// Config, do not enable here but use a build define
//#define SAMSUNGVFD_SPI_SWCS				// Enable Software CS control, with UseHwCs() its faster without
//#define SAMSUNGVFD_SPI_TRANSACTION		// Enable using SPI.begin/endTransaction if multiple configs are used

#define SAMSUNGVFD_CS_NONE			    0xFF
#define SAMSUNGVFD_ADDRESS_DEFAULT		0x44	// and +1, so 0x97 for data

class SamsungVFD : public Print 
{
    public:
		SamsungVFD() {};

        void begin();

        void clear();
        void home();

        void setBrightness(uint8_t brightness);
        uint8_t getBrightness();

        void displayOn(bool = true);
        void blink(bool = true);
        void cursor(bool = true);
        void scrollDisplayLeft();
        void scrollDisplayRight();
        void leftToRight();
        void rightToLeft();
        void autoscroll(bool on = true);

        void createChar(uint8_t location, uint8_t charmap[]);
        void setCursor(uint8_t col, uint8_t row);

        // Implement thsese for Print 
        virtual size_t write(uint8_t) = 0;
#ifdef SAMSUNGVFD_SPI_SWCS
		//Needs testing on HWCS platform first
       virtual size_t write(const uint8_t *buffer, size_t size) = 0;
#endif
    protected:
        virtual void command(uint8_t) = 0;

        uint8_t _displayfunction = 0x00;
        uint8_t _displaycontrol = 0x00;
        uint8_t _displaymode = 0x00;

        // uint8_t _numlines = 2;
};

class SamsungVFD_SPI : public SamsungVFD
{
    public:
#ifdef SAMSUNGVFD_SPI_SWCS
        SamsungVFD_SPI(SPIClass& spi, uint8_t cs = SAMSUNGVFD_CS_NONE) : _spi(spi), _cs(cs) {};
#else
        SamsungVFD_SPI(SPIClass& spi) : _spi(spi) {};
#endif

        virtual size_t write(uint8_t);
        virtual size_t write(const uint8_t *buffer, size_t size);

	protected:
        SPIClass& _spi;
		uint8_t _cs = -1;
        SPISettings _spisettings = SPISettings(25000000, MSBFIRST, SPI_MODE3);
        virtual void command(uint8_t);
};

class SamsungVFD_I2C : public SamsungVFD
{
    public:
        SamsungVFD_I2C(TwoWire& wire, const uint8_t addr = SAMSUNGVFD_ADDRESS_DEFAULT) : _i2caddr(addr), _wire(wire) {};
        SamsungVFD_I2C(const uint8_t addr = SAMSUNGVFD_ADDRESS_DEFAULT) : _i2caddr(addr), _wire(Wire) {};

        virtual size_t write(uint8_t);
        virtual size_t write(const uint8_t *buffer, size_t size);

	protected:
        virtual void command(uint8_t);

        uint8_t _i2caddr;
        TwoWire& _wire = Wire;
};

#endif